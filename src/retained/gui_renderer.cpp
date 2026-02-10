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

void GuiRenderer::setDragDropManager(DragDropManager* manager) {
    dndManager_ = manager;
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
        // Phase 7
        case WidgetNode::Type::ListBox:          renderListBox(node); break;
        case WidgetNode::Type::Popup:            renderPopup(node); break;
        case WidgetNode::Type::Modal:            renderModal(node); break;
        // Phase 8
        case WidgetNode::Type::Canvas:           renderCanvas(node); break;
        case WidgetNode::Type::Tooltip:          renderTooltip(node); break;
        // Phase 9
        case WidgetNode::Type::RadioButton:       renderRadioButton(node); break;
        case WidgetNode::Type::Selectable:        renderSelectable(node); break;
        case WidgetNode::Type::InputTextMultiline:renderInputTextMultiline(node); break;
        case WidgetNode::Type::BulletText:        renderBulletText(node); break;
        case WidgetNode::Type::SeparatorText:     renderSeparatorText(node); break;
        case WidgetNode::Type::Indent:            renderIndent(node); break;
        default:
            ImGui::TextColored({1, 0, 0, 1}, "[TODO: %s]", widgetTypeName(node.type));
            break;
    }

    // DnD handling (after widget is rendered so ImGui has the item rect)
    handleDragDrop(node);

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
    if (ImGui::Begin(node.label.c_str(), &open,
                     static_cast<ImGuiWindowFlags>(node.windowFlags))) {
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

// -- Phase 7: Misc ------------------------------------------------------------

void GuiRenderer::renderListBox(WidgetNode& node) {
    // Calculate size based on heightInItems (-1 = auto)
    int heightItems = node.heightInItems;
    float heightPx = 0.0f;
    if (heightItems > 0) {
        heightPx = ImGui::GetTextLineHeightWithSpacing() * heightItems
                   + ImGui::GetStyle().FramePadding.y * 2.0f;
    }

    if (ImGui::BeginListBox(node.label.c_str(), {0.0f, heightPx})) {
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
        ImGui::EndListBox();
    }
}

void GuiRenderer::renderPopup(WidgetNode& node) {
    const char* id = node.id.empty() ? "##popup" : node.id.c_str();

    // boolValue = true means "request open this frame"
    if (node.boolValue) {
        ImGui::OpenPopup(id);
        node.boolValue = false;
    }

    if (ImGui::BeginPopup(id)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndPopup();
    }
}

void GuiRenderer::renderModal(WidgetNode& node) {
    const char* title = node.label.empty() ? "##modal" : node.label.c_str();

    // boolValue = true means "request open this frame"
    if (node.boolValue) {
        ImGui::OpenPopup(title);
        node.boolValue = false;
    }

    bool open = true;
    if (ImGui::BeginPopupModal(title, &open)) {
        for (auto& child : node.children) {
            renderNode(child);
        }
        ImGui::EndPopup();
    }

    if (!open) {
        if (node.onClose) node.onClose(node);
    }
}

// -- Phase 8: Custom ----------------------------------------------------------

void GuiRenderer::renderCanvas(WidgetNode& node) {
    const char* id = node.id.empty() ? "##canvas" : node.id.c_str();
    float w = node.width > 0 ? node.width : 200.0f;
    float h = node.height > 0 ? node.height : 200.0f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize{w, h};

    // Reserve the canvas area
    ImGui::InvisibleButton(id, canvasSize);

    bool isClicked = ImGui::IsItemClicked();
    bool isHovered = ImGui::IsItemHovered();

    // Draw background if color is not the default white
    if (node.colorR < 1.0f || node.colorG < 1.0f || node.colorB < 1.0f || node.colorA < 1.0f) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(
            {node.colorR, node.colorG, node.colorB, node.colorA});
        drawList->AddRectFilled(canvasPos,
            {canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y}, bgCol);
    }

    // Draw border
    if (node.border) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 borderCol = ImGui::ColorConvertFloat4ToU32({0.5f, 0.5f, 0.5f, 1.0f});
        drawList->AddRect(canvasPos,
            {canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y}, borderCol);
    }

    // Custom draw callback
    if (node.onDraw) {
        node.onDraw(node);
    }

    if (isClicked && node.onClick) {
        node.onClick(node);
    }

    (void)isHovered; // available for future tooltip support
}

void GuiRenderer::renderTooltip(WidgetNode& node) {
    // Tooltip applies to the previous widget
    if (!ImGui::IsItemHovered()) return;

    if (!node.textContent.empty() && node.children.empty()) {
        // Simple text tooltip
        ImGui::SetItemTooltip("%s", node.textContent.c_str());
    } else if (!node.children.empty()) {
        // Rich tooltip with child widgets
        if (ImGui::BeginTooltip()) {
            if (!node.textContent.empty()) {
                ImGui::TextUnformatted(node.textContent.c_str());
            }
            for (auto& child : node.children) {
                renderNode(child);
            }
            ImGui::EndTooltip();
        }
    }
}

// -- Phase 9: New Widgets -----------------------------------------------------

void GuiRenderer::renderRadioButton(WidgetNode& node) {
    // intValue = currently active value in the group
    // minInt = this radio button's value
    if (ImGui::RadioButton(node.label.c_str(), &node.intValue, node.minInt)) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderSelectable(WidgetNode& node) {
    if (ImGui::Selectable(node.label.c_str(), &node.boolValue)) {
        if (node.onClick) node.onClick(node);
    }
}

void GuiRenderer::renderInputTextMultiline(WidgetNode& node) {
    if (node.stringValue.capacity() < 1024) {
        node.stringValue.reserve(1024);
    }
    size_t cap = node.stringValue.capacity();
    node.stringValue.resize(cap);

    InputTextCallbackData cbData{&node.stringValue};

    ImGui::InputTextMultiline(
        node.label.c_str(),
        node.stringValue.data(),
        node.stringValue.size() + 1,
        {node.width, node.height},
        ImGuiInputTextFlags_CallbackResize,
        inputTextResizeCallback,
        &cbData
    );

    node.stringValue.resize(std::strlen(node.stringValue.c_str()));

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (node.onChange) node.onChange(node);
    }
}

void GuiRenderer::renderBulletText(WidgetNode& node) {
    ImGui::BulletText("%s", node.textContent.c_str());
}

void GuiRenderer::renderSeparatorText(WidgetNode& node) {
    ImGui::SeparatorText(node.label.c_str());
}

void GuiRenderer::renderIndent(WidgetNode& node) {
    if (node.width < 0) {
        ImGui::Unindent(-node.width > 0 ? -node.width : 0.0f);
    } else {
        ImGui::Indent(node.width > 0 ? node.width : 0.0f);
    }
}

// -- Drag and Drop ------------------------------------------------------------

void GuiRenderer::handleDragDrop(WidgetNode& node) {
    bool isDragSource = !node.dragType.empty();
    bool isDropTarget = !node.dropAcceptType.empty();
    if (!isDragSource && !isDropTarget) return;

    bool allowTraditional = (node.dragMode == 0 || node.dragMode == 1);
    bool allowClickPickup = (node.dragMode == 0 || node.dragMode == 2);

    // === DRAG SOURCE ===
    if (isDragSource) {
        // Traditional ImGui DnD
        if (allowTraditional) {
            ImGuiDragDropFlags srcFlags = ImGuiDragDropFlags_SourceAllowNullID;
            if (ImGui::BeginDragDropSource(srcFlags)) {
                ImGui::SetDragDropPayload(node.dragType.c_str(),
                    node.dragData.data(), node.dragData.size());

                // Preview: show image if Image widget, else show label/text
                if (node.type == WidgetNode::Type::Image && node.texture.valid()) {
                    ImGui::Image(static_cast<ImTextureID>(node.texture),
                                 {node.imageWidth, node.imageHeight});
                } else if (!node.label.empty()) {
                    ImGui::TextUnformatted(node.label.c_str());
                } else if (!node.textContent.empty()) {
                    ImGui::TextUnformatted(node.textContent.c_str());
                } else {
                    ImGui::TextUnformatted(node.dragData.c_str());
                }

                ImGui::EndDragDropSource();
            }
        }

        // Click-to-pick-up
        if (allowClickPickup && dndManager_ && !dndManager_->isHolding()) {
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                // Only pick up if not in a traditional ImGui drag
                if (!ImGui::GetDragDropPayload()) {
                    DragDropManager::CursorItem item;
                    item.type = node.dragType;
                    item.data = node.dragData;
                    if (node.type == WidgetNode::Type::Image && node.texture.valid()) {
                        item.textureId = static_cast<ImTextureID>(node.texture);
                        item.iconWidth = node.imageWidth;
                        item.iconHeight = node.imageHeight;
                    } else {
                        item.fallbackText = !node.label.empty() ? node.label :
                                            !node.textContent.empty() ? node.textContent :
                                            node.dragData;
                    }
                    dndManager_->pickUp(item);
                    if (node.onDragBegin) node.onDragBegin(node);
                }
            }
        }
    }

    // === DROP TARGET ===
    if (isDropTarget) {
        // Traditional ImGui DnD
        if (allowTraditional) {
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(node.dropAcceptType.c_str());
                if (payload) {
                    node.dragData = std::string(
                        static_cast<const char*>(payload->Data),
                        static_cast<size_t>(payload->DataSize));
                    if (node.onDrop) node.onDrop(node);
                }
                ImGui::EndDragDropTarget();
            }
        }

        // Click-to-pick-up
        if (dndManager_ && dndManager_->isHolding(node.dropAcceptType)) {
            if (ImGui::IsItemHovered()) {
                // Visual highlight: yellow border
                ImVec2 rMin = ImGui::GetItemRectMin();
                ImVec2 rMax = ImGui::GetItemRectMax();
                ImGui::GetForegroundDrawList()->AddRect(
                    rMin, rMax,
                    ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 0.0f, 0.8f}),
                    0.0f, 0, 2.0f);

                // Click to deliver
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    auto delivered = dndManager_->dropItem();
                    node.dragData = std::move(delivered.data);
                    if (node.onDrop) node.onDrop(node);
                }
            }
        }
    }
}

} // namespace finegui
