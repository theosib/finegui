#pragma once

#include <imgui.h>
#include <string>

namespace finegui {

/// Global state for click-to-pick-up drag-and-drop mode.
///
/// Traditional ImGui DnD (click-drag-release) works automatically because
/// both GuiRenderer and MapRenderer emit ImGui calls in the same frame.
/// Click-to-pick-up mode needs global state to track what the cursor
/// is currently "carrying."
///
/// Usage:
///   DragDropManager dndManager;
///   guiRenderer.setDragDropManager(&dndManager);
///   mapRenderer.setDragDropManager(&dndManager);
///   // Each frame, after all renderers:
///   dndManager.renderCursorItem();
class DragDropManager {
public:
    /// What the cursor is currently carrying (empty if nothing).
    struct CursorItem {
        std::string type;           // DnD type string (e.g., "item")
        std::string data;           // payload data string
        ImTextureID textureId = 0;  // icon texture (0 = use text fallback)
        float iconWidth = 32.0f;    // icon display width
        float iconHeight = 32.0f;   // icon display height
        std::string fallbackText;   // text shown if no texture

        bool empty() const { return type.empty(); }
        void clear() { type.clear(); data.clear(); textureId = 0; fallbackText.clear(); }
    };

    /// Pick up an item (click-to-pick-up: first click).
    void pickUp(const CursorItem& item);

    /// Drop the currently held item. Returns the item that was being held.
    CursorItem dropItem();

    /// Cancel (e.g., right-click or Escape).
    void cancel();

    /// Check if carrying an item.
    bool isHolding() const;

    /// Check if carrying an item of the given type.
    bool isHolding(const std::string& type) const;

    /// Read-only access to the current cursor item.
    const CursorItem& cursorItem() const;

    /// Render the floating icon/text at the cursor position.
    /// Call once per frame, after all widget rendering is done,
    /// but before gui.endFrame().
    void renderCursorItem();

private:
    CursorItem cursorItem_;
};

} // namespace finegui
