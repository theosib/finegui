#include <finegui/gui_renderer.hpp>
#include <finegui/gui_system.hpp>
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace finegui {

// -- InputText resize callback ------------------------------------------------

struct InputTextCallbackData {
    std::string* str;
};

static int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* userData = static_cast<InputTextCallbackData*>(data->UserData);
        userData->str->resize(static_cast<size_t>(data->BufTextLen));
        data->Buf = userData->str->data();
    }
    return 0;
}

// -- GuiRenderer --------------------------------------------------------------

GuiRenderer::GuiRenderer(GuiSystem& gui)
    : gui_(gui) {
    (void)gui_;  // Reserved for future use (e.g., querying display size)
}

int GuiRenderer::show(WidgetNode tree) {
    int id = nextId_++;
    trees_.emplace(id, std::move(tree));
    return id;
}

void GuiRenderer::update(int guiId, WidgetNode tree) {
    auto it = trees_.find(guiId);
    if (it != trees_.end()) {
        it->second = std::move(tree);
    }
}

void GuiRenderer::hide(int guiId) {
    trees_.erase(guiId);
}

void GuiRenderer::hideAll() {
    trees_.clear();
}

WidgetNode* GuiRenderer::get(int guiId) {
    auto it = trees_.find(guiId);
    return it != trees_.end() ? &it->second : nullptr;
}

void GuiRenderer::renderAll() {
    for (auto& [id, tree] : trees_) {
        renderNode(tree);
    }
}

// -- Dispatch -----------------------------------------------------------------

void GuiRenderer::renderNode(WidgetNode& node) {
    if (!node.visible) return;

    bool wasDisabled = false;
    if (!node.enabled) {
        ImGui::BeginDisabled(true);
        wasDisabled = true;
    }

    bool pushId = !node.id.empty();
    if (pushId) {
        ImGui::PushID(node.id.c_str());
    }

    switch (node.type) {
        case WidgetNode::Type::Window:     renderWindow(node); break;
        case WidgetNode::Type::Text:       renderText(node); break;
        case WidgetNode::Type::Button:     renderButton(node); break;
        case WidgetNode::Type::Checkbox:   renderCheckbox(node); break;
        case WidgetNode::Type::Slider:     renderSlider(node); break;
        case WidgetNode::Type::SliderInt:  renderSliderInt(node); break;
        case WidgetNode::Type::InputText:  renderInputText(node); break;
        case WidgetNode::Type::InputInt:   renderInputInt(node); break;
        case WidgetNode::Type::InputFloat: renderInputFloat(node); break;
        case WidgetNode::Type::Combo:      renderCombo(node); break;
        case WidgetNode::Type::Separator:  renderSeparator(node); break;
        case WidgetNode::Type::Group:      renderGroup(node); break;
        case WidgetNode::Type::Columns:    renderColumns(node); break;
        case WidgetNode::Type::Image:      renderImage(node); break;
        default:
            ImGui::TextColored({1, 0, 0, 1}, "[TODO: %s]", widgetTypeName(node.type));
            break;
    }

    if (pushId) {
        ImGui::PopID();
    }

    if (wasDisabled) {
        ImGui::EndDisabled();
    }
}

// -- Per-widget render methods ------------------------------------------------

void GuiRenderer::renderWindow(WidgetNode& node) {
    bool open = true;
    if (ImGui::Begin(node.label.c_str(), &open)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
    }
    ImGui::End();
    if (!open) {
        node.visible = false;
        if (node.onClose) node.onClose(node);
    }
}

void GuiRenderer::renderText(WidgetNode& node) {
    ImGui::TextUnformatted(node.textContent.c_str());
}

void GuiRenderer::renderButton(WidgetNode& node) {
    bool clicked;
    if (node.width > 0 || node.height > 0) {
        clicked = ImGui::Button(node.label.c_str(), {node.width, node.height});
    } else {
        clicked = ImGui::Button(node.label.c_str());
    }
    if (clicked && node.onClick) {
        node.onClick(node);
    }
}

void GuiRenderer::renderCheckbox(WidgetNode& node) {
    if (ImGui::Checkbox(node.label.c_str(), &node.boolValue)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderSlider(WidgetNode& node) {
    if (ImGui::SliderFloat(node.label.c_str(), &node.floatValue,
                           node.minFloat, node.maxFloat)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderSliderInt(WidgetNode& node) {
    if (ImGui::SliderInt(node.label.c_str(), &node.intValue,
                         node.minInt, node.maxInt)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderInputText(WidgetNode& node) {
    // Ensure buffer has enough capacity for editing
    if (node.stringValue.capacity() < 256) {
        node.stringValue.reserve(256);
    }
    // Resize to capacity so ImGui can write into the full buffer
    size_t cap = node.stringValue.capacity();
    node.stringValue.resize(cap);

    InputTextCallbackData cbData{&node.stringValue};

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;
    if (node.onSubmit) {
        flags |= ImGuiInputTextFlags_EnterReturnsTrue;
    }

    bool enterPressed = ImGui::InputText(
        node.label.c_str(),
        node.stringValue.data(),
        node.stringValue.size() + 1,  // +1 for null terminator
        flags,
        inputTextResizeCallback,
        &cbData
    );

    // Trim to actual string length (ImGui writes null-terminated)
    node.stringValue.resize(std::strlen(node.stringValue.c_str()));

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (node.onChange) node.onChange(node);
    }

    if (enterPressed && node.onSubmit) {
        node.onSubmit(node);
    }
}

void GuiRenderer::renderInputInt(WidgetNode& node) {
    if (ImGui::InputInt(node.label.c_str(), &node.intValue)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderInputFloat(WidgetNode& node) {
    if (ImGui::InputFloat(node.label.c_str(), &node.floatValue)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderCombo(WidgetNode& node) {
    // Get preview text
    const char* preview = (node.selectedIndex >= 0 &&
                           node.selectedIndex < static_cast<int>(node.items.size()))
                          ? node.items[static_cast<size_t>(node.selectedIndex)].c_str()
                          : "";

    if (ImGui::BeginCombo(node.label.c_str(), preview)) {
        for (int i = 0; i < static_cast<int>(node.items.size()); i++) {
            bool isSelected = (i == node.selectedIndex);
            if (ImGui::Selectable(node.items[static_cast<size_t>(i)].c_str(), isSelected)) {
                node.selectedIndex = i;
                if (node.onChange) node.onChange(node);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void GuiRenderer::renderSeparator(WidgetNode& /*node*/) {
    ImGui::Separator();
}

void GuiRenderer::renderGroup(WidgetNode& node) {
    for (auto& child : node.children) {
        renderNode(child);
    }
}

void GuiRenderer::renderColumns(WidgetNode& node) {
    if (node.columnCount <= 1) {
        // Just render children sequentially
        for (auto& child : node.children) {
            renderNode(child);
        }
        return;
    }

    ImGui::Columns(node.columnCount, nullptr, false);
    for (size_t i = 0; i < node.children.size(); i++) {
        renderNode(node.children[i]);
        if (i + 1 < node.children.size()) {
            ImGui::NextColumn();
        }
    }
    ImGui::Columns(1);
}

void GuiRenderer::renderImage(WidgetNode& node) {
    if (node.texture.valid()) {
        ImGui::Image(static_cast<ImTextureID>(node.texture),
                     {node.imageWidth, node.imageHeight});
        if (node.onClick && ImGui::IsItemClicked()) {
            node.onClick(node);
        }
    }
}

} // namespace finegui
