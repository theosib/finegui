#pragma once

#include "widget_node.hpp"
#include <map>

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

private:
    GuiSystem& gui_;
    int nextId_ = 1;
    std::map<int, WidgetNode> trees_;

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
};

} // namespace finegui
