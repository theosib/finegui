#include <finegui/drag_drop_manager.hpp>
#include <imgui.h>

namespace finegui {

void DragDropManager::pickUp(const CursorItem& item) {
    cursorItem_ = item;
}

DragDropManager::CursorItem DragDropManager::dropItem() {
    CursorItem result = std::move(cursorItem_);
    cursorItem_.clear();
    return result;
}

void DragDropManager::cancel() {
    cursorItem_.clear();
}

bool DragDropManager::isHolding() const {
    return !cursorItem_.empty();
}

bool DragDropManager::isHolding(const std::string& type) const {
    return !cursorItem_.empty() && cursorItem_.type == type;
}

const DragDropManager::CursorItem& DragDropManager::cursorItem() const {
    return cursorItem_;
}

void DragDropManager::renderCursorItem() {
    if (cursorItem_.empty()) return;

    // Cancel on Escape or right-click
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        cancel();
        return;
    }

    // Render a floating window at cursor position with the icon/text
    ImVec2 mousePos = ImGui::GetMousePos();
    ImGui::SetNextWindowPos({mousePos.x + 16, mousePos.y + 16});
    ImGui::SetNextWindowBgAlpha(0.7f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("##dnd_cursor_item", nullptr, flags)) {
        if (cursorItem_.textureId != 0) {
            ImGui::Image(cursorItem_.textureId,
                         {cursorItem_.iconWidth, cursorItem_.iconHeight});
        } else if (!cursorItem_.fallbackText.empty()) {
            ImGui::TextUnformatted(cursorItem_.fallbackText.c_str());
        } else {
            ImGui::TextUnformatted("[item]");
        }
    }
    ImGui::End();
}

} // namespace finegui
