#pragma once

// The join between ImGui and finegui's skins. third_party/imgui-skin-hooks.patch adds calls to
// ImGuiSkinDraw at the points where ImGui would paint a widget's background: if a skin is
// active and has the part, it draws that instead. Everything else about the widget, from
// layout and input to its text, stays ImGui's.
//
// This is a separate header so that imgui.h itself stays as it comes from upstream.

#include "imgui.h"

enum ImGuiSkinPart {
    ImGuiSkinPart_WindowBg,
    ImGuiSkinPart_PopupBg,
    ImGuiSkinPart_TooltipBg,
    ImGuiSkinPart_TitleBar,
    ImGuiSkinPart_MenuBar,
    ImGuiSkinPart_Button,
    ImGuiSkinPart_CloseButton,
    ImGuiSkinPart_Frame,          // Text inputs, drags, combos
    ImGuiSkinPart_Checkbox,
    ImGuiSkinPart_Radio,
    ImGuiSkinPart_SliderTrack,
    ImGuiSkinPart_SliderGrab,
    ImGuiSkinPart_ProgressTrack,
    ImGuiSkinPart_ProgressFill,
    ImGuiSkinPart_ScrollbarTrack,
    ImGuiSkinPart_ScrollbarGrab,
    ImGuiSkinPart_Header,         // Selectables, tree nodes, collapsing headers, menu items
    ImGuiSkinPart_Tab,
    ImGuiSkinPart_COUNT
};

enum ImGuiSkinState {
    ImGuiSkinState_Normal = 0,
    ImGuiSkinState_Hovered = 1,
    ImGuiSkinState_Active = 2,    // Pressed, or held
    ImGuiSkinState_Selected = 3,  // Tabs, and windows that have focus
    ImGuiSkinState_COUNT
};

/// `clip` is the part of the box to show, for a bar that fills to some fraction of it. The
/// art is laid out for the whole box either way.
using ImGuiSkinDrawFn = bool (*)(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state,
                                 const ImVec2& min, const ImVec2& max, const ImVec2& clipMin,
                                 const ImVec2& clipMax);

/// Set by finegui::setActiveSkin. Null means no skin, and ImGui draws as it always has.
extern ImGuiSkinDrawFn GImGuiSkinDraw;

inline bool ImGuiSkinDraw(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state,
                          const ImVec2& min, const ImVec2& max) {
    return GImGuiSkinDraw && GImGuiSkinDraw(drawList, part, state, min, max, min, max);
}

inline bool ImGuiSkinDrawClipped(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state,
                                 const ImVec2& min, const ImVec2& max, const ImVec2& clipMin,
                                 const ImVec2& clipMax) {
    return GImGuiSkinDraw && GImGuiSkinDraw(drawList, part, state, min, max, clipMin, clipMax);
}

inline ImGuiSkinState ImGuiSkinStateOf(bool hovered, bool held) {
    return held ? ImGuiSkinState_Active : hovered ? ImGuiSkinState_Hovered : ImGuiSkinState_Normal;
}
