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
        // Phase 3
        case WidgetNode::Type::SameLine:          renderSameLine(node); break;
        case WidgetNode::Type::Spacing:           renderSpacing(node); break;
        case WidgetNode::Type::TextColored:       renderTextColored(node); break;
        case WidgetNode::Type::TextWrapped:       renderTextWrapped(node); break;
        case WidgetNode::Type::TextDisabled:      renderTextDisabled(node); break;
        case WidgetNode::Type::ProgressBar:       renderProgressBar(node); break;
        case WidgetNode::Type::CollapsingHeader:  renderCollapsingHeader(node); break;
        // Phase 4
        case WidgetNode::Type::TabBar:            renderTabBar(node); break;
        case WidgetNode::Type::TabItem:           renderTabItem(node); break;
        case WidgetNode::Type::TreeNode:          renderTreeNode(node); break;
        case WidgetNode::Type::Child:             renderChild(node); break;
        case WidgetNode::Type::MenuBar:           renderMenuBar(node); break;
        case WidgetNode::Type::Menu:              renderMenu(node); break;
        case WidgetNode::Type::MenuItem:          renderMenuItem(node); break;
        // Phase 5
        case WidgetNode::Type::Table:             renderTable(node); break;
        case WidgetNode::Type::TableRow:          renderTableRow(node); break;
        case WidgetNode::Type::TableColumn:       renderTableColumn(node); break;
        // Phase 6
        case WidgetNode::Type::ColorEdit:         renderColorEdit(node); break;
        case WidgetNode::Type::ColorPicker:       renderColorPicker(node); break;
        case WidgetNode::Type::DragFloat:         renderDragFloat(node); break;
        case WidgetNode::Type::DragInt:           renderDragInt(node); break;
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

// -- Phase 3: Layout & Display ------------------------------------------------

void GuiRenderer::renderSameLine(WidgetNode& node) {
    if (node.offsetX > 0) {
        ImGui::SameLine(node.offsetX);
    } else {
        ImGui::SameLine();
    }
}

void GuiRenderer::renderSpacing(WidgetNode& /*node*/) {
    ImGui::Spacing();
}

void GuiRenderer::renderTextColored(WidgetNode& node) {
    ImGui::TextColored({node.colorR, node.colorG, node.colorB, node.colorA},
                       "%s", node.textContent.c_str());
}

void GuiRenderer::renderTextWrapped(WidgetNode& node) {
    ImGui::TextWrapped("%s", node.textContent.c_str());
}

void GuiRenderer::renderTextDisabled(WidgetNode& node) {
    ImGui::TextDisabled("%s", node.textContent.c_str());
}

void GuiRenderer::renderProgressBar(WidgetNode& node) {
    float w = (node.width > 0) ? node.width : -FLT_MIN;
    float h = node.height;
    const char* overlay = node.overlayText.empty() ? nullptr : node.overlayText.c_str();
    ImGui::ProgressBar(node.floatValue, {w, h}, overlay);
}

void GuiRenderer::renderCollapsingHeader(WidgetNode& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (node.defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::CollapsingHeader(node.label.c_str(), flags)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
    }
}

// -- Phase 4: Containers & Menus ----------------------------------------------

void GuiRenderer::renderTabBar(WidgetNode& node) {
    const char* id = node.id.empty() ? "##tabbar" : node.id.c_str();
    if (ImGui::BeginTabBar(id)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndTabBar();
    }
}

void GuiRenderer::renderTabItem(WidgetNode& node) {
    if (ImGui::BeginTabItem(node.label.c_str())) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndTabItem();
    }
}

void GuiRenderer::renderTreeNode(WidgetNode& node) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (node.leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (node.defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool open = ImGui::TreeNodeEx(node.label.c_str(), flags);

    if (ImGui::IsItemClicked() && node.onClick) {
        node.onClick(node);
    }

    if (open && !node.leaf) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::TreePop();
    }
}

void GuiRenderer::renderChild(WidgetNode& node) {
    const char* id = node.id.empty() ? "##child" : node.id.c_str();

    ImGuiChildFlags childFlags = ImGuiChildFlags_None;
    if (node.border) childFlags |= ImGuiChildFlags_Borders;

    if (ImGui::BeginChild(id, {node.width, node.height}, childFlags)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        if (node.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

void GuiRenderer::renderMenuBar(WidgetNode& node) {
    if (ImGui::BeginMenuBar()) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndMenuBar();
    }
}

void GuiRenderer::renderMenu(WidgetNode& node) {
    if (ImGui::BeginMenu(node.label.c_str())) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndMenu();
    }
}

void GuiRenderer::renderMenuItem(WidgetNode& node) {
    const char* shortcut = node.shortcutText.empty() ? nullptr : node.shortcutText.c_str();

    if (ImGui::MenuItem(node.label.c_str(), shortcut, node.checked)) {
        if (node.onClick) node.onClick(node);
    }
}

// -- Phase 5: Tables ----------------------------------------------------------

void GuiRenderer::renderTable(WidgetNode& node) {
    const char* id = node.id.empty() ? "##table" : node.id.c_str();
    int numCols = node.columnCount > 0 ? node.columnCount : 1;

    if (ImGui::BeginTable(id, numCols, static_cast<ImGuiTableFlags>(node.tableFlags))) {
        // Setup column headers if provided (stored in items)
        if (!node.items.empty()) {
            for (const auto& header : node.items) {
                ImGui::TableSetupColumn(header.c_str());
            }
            ImGui::TableHeadersRow();
        }

        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndTable();
    }
}

void GuiRenderer::renderTableRow(WidgetNode& node) {
    ImGui::TableNextRow();
    if (!node.children.empty()) {
        // Container mode: each child goes into the next column
        for (auto& child : node.children) {
            ImGui::TableNextColumn();
            renderNode(child);
        }
    }
}

void GuiRenderer::renderTableColumn(WidgetNode& /*node*/) {
    ImGui::TableNextColumn();
}

// -- Phase 6: Advanced Input --------------------------------------------------

void GuiRenderer::renderColorEdit(WidgetNode& node) {
    float col[4] = {node.colorR, node.colorG, node.colorB, node.colorA};
    if (ImGui::ColorEdit4(node.label.c_str(), col)) {
        node.colorR = col[0]; node.colorG = col[1];
        node.colorB = col[2]; node.colorA = col[3];
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderColorPicker(WidgetNode& node) {
    float col[4] = {node.colorR, node.colorG, node.colorB, node.colorA};
    if (ImGui::ColorPicker4(node.label.c_str(), col)) {
        node.colorR = col[0]; node.colorG = col[1];
        node.colorB = col[2]; node.colorA = col[3];
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderDragFloat(WidgetNode& node) {
    if (ImGui::DragFloat(node.label.c_str(), &node.floatValue,
                         node.dragSpeed, node.minFloat, node.maxFloat)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderDragInt(WidgetNode& node) {
    if (ImGui::DragInt(node.label.c_str(), &node.intValue,
                       node.dragSpeed, node.minInt, node.maxInt)) {
        if (node.onChange) node.onChange(node);
    }
}

} // namespace finegui
