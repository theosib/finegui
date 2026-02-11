#pragma once

#include "widget_node.hpp"
#include "drag_drop_manager.hpp"
#include <map>
#include <string>

namespace finegui {

class GuiSystem;

/// Retained-mode GUI renderer.
///
/// Manages a collection of widget trees and renders them each frame
/// by walking the tree and issuing the corresponding ImGui calls.
///
/// Usage:
///   GuiRenderer renderer(gui);
///   int id = renderer.show(WidgetNode::window("Settings", { ... }));
///   // Each frame:
///   gui.beginFrame();
///   renderer.renderAll();
///   gui.endFrame();
class GuiRenderer {
public:
    explicit GuiRenderer(GuiSystem& gui);

    /// Register a widget tree to be rendered each frame.
    /// Returns an ID that can be used to update or remove it.
    int show(WidgetNode tree);

    /// Replace an existing widget tree.
    void update(int guiId, WidgetNode tree);

    /// Remove a widget tree.
    void hide(int guiId);

    /// Remove all widget trees.
    void hideAll();

    /// Get a reference to a live widget tree (for direct mutation).
    /// Returns nullptr if the ID is not found.
    WidgetNode* get(int guiId);

    /// Call once per frame, between gui.beginFrame() and gui.endFrame().
    /// Walks all active widget trees and issues ImGui calls.
    void renderAll();

    /// Set the DragDropManager for click-to-pick-up mode.
    /// Pass nullptr to disable click-to-pick-up (traditional DnD still works).
    void setDragDropManager(DragDropManager* manager);

    /// Programmatically focus a widget by its ID string.
    /// The focus will be applied during the next renderAll() call.
    void setFocus(const std::string& widgetId);

    /// Find a widget node by its ID string across all trees.
    /// Returns nullptr if not found. Returns first match.
    WidgetNode* findById(const std::string& widgetId);

private:
    static WidgetNode* findByIdRecursive(WidgetNode& node, const std::string& widgetId);

    DragDropManager* dndManager_ = nullptr;
    GuiSystem& gui_;
    int nextId_ = 1;
    std::map<int, WidgetNode> trees_;

    // Focus tracking
    std::string pendingFocusId_;
    std::string lastFocusedId_;
    std::string currentFocusedId_;

    void renderNode(WidgetNode& node);
    void renderWindow(WidgetNode& node);
    void renderText(WidgetNode& node);
    void renderButton(WidgetNode& node);
    void renderCheckbox(WidgetNode& node);
    void renderSlider(WidgetNode& node);
    void renderSliderInt(WidgetNode& node);
    void renderInputText(WidgetNode& node);
    void renderInputInt(WidgetNode& node);
    void renderInputFloat(WidgetNode& node);
    void renderCombo(WidgetNode& node);
    void renderSeparator(WidgetNode& node);
    void renderGroup(WidgetNode& node);
    void renderColumns(WidgetNode& node);
    void renderImage(WidgetNode& node);

    // Phase 3
    void renderSameLine(WidgetNode& node);
    void renderSpacing(WidgetNode& node);
    void renderTextColored(WidgetNode& node);
    void renderTextWrapped(WidgetNode& node);
    void renderTextDisabled(WidgetNode& node);
    void renderProgressBar(WidgetNode& node);
    void renderCollapsingHeader(WidgetNode& node);

    // Phase 4
    void renderTabBar(WidgetNode& node);
    void renderTabItem(WidgetNode& node);
    void renderTreeNode(WidgetNode& node);
    void renderChild(WidgetNode& node);
    void renderMenuBar(WidgetNode& node);
    void renderMenu(WidgetNode& node);
    void renderMenuItem(WidgetNode& node);

    // Phase 5
    void renderTable(WidgetNode& node);
    void renderTableRow(WidgetNode& node);
    void renderTableColumn(WidgetNode& node);

    // Phase 6
    void renderColorEdit(WidgetNode& node);
    void renderColorPicker(WidgetNode& node);
    void renderDragFloat(WidgetNode& node);
    void renderDragInt(WidgetNode& node);

    // Phase 7
    void renderListBox(WidgetNode& node);
    void renderPopup(WidgetNode& node);
    void renderModal(WidgetNode& node);

    // Phase 8
    void renderCanvas(WidgetNode& node);
    void renderTooltip(WidgetNode& node);

    // Phase 9
    void renderRadioButton(WidgetNode& node);
    void renderSelectable(WidgetNode& node);
    void renderInputTextMultiline(WidgetNode& node);
    void renderBulletText(WidgetNode& node);
    void renderSeparatorText(WidgetNode& node);
    void renderIndent(WidgetNode& node);

    // Phase 10 - Style push/pop
    void renderPushStyleColor(WidgetNode& node);
    void renderPopStyleColor(WidgetNode& node);
    void renderPushStyleVar(WidgetNode& node);
    void renderPopStyleVar(WidgetNode& node);

    // Phase 11 - Layout helpers
    void renderDummy(WidgetNode& node);
    void renderNewLine(WidgetNode& node);

    // Phase 12 - Advanced Input (continued)
    void renderDragFloat3(WidgetNode& node);
    void renderInputTextWithHint(WidgetNode& node);
    void renderSliderAngle(WidgetNode& node);
    void renderSmallButton(WidgetNode& node);
    void renderColorButton(WidgetNode& node);

    // Phase 13 - Menus & Popups (continued)
    void renderContextMenu(WidgetNode& node);
    void renderMainMenuBar(WidgetNode& node);

    // Drag-and-drop
    void handleDragDrop(WidgetNode& node);
};

} // namespace finegui
