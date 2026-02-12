#pragma once

/**
 * @file widget_state.hpp
 * @brief Lightweight types for widget state serialization (retained mode)
 *
 * WidgetStateValue is a variant holding the saveable state of a single widget.
 * WidgetStateMap is a flat string-keyed map of these values, used by
 * GuiRenderer::saveState() / loadState().
 */

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace finegui {

/// A single widget's serializable state.
///
/// - bool:            checkbox, selectable
/// - int:             combo/listbox selected index, slider_int, input_int, drag_int, radio_button
/// - double:          slider, input_float, drag_float, slider_angle, progress_bar
/// - string:          input_text, input_multiline, input_with_hint
/// - vector<float>:   color (4 floats), drag_float3 (3 floats)
using WidgetStateValue = std::variant<bool, int, double, std::string, std::vector<float>>;

/// Map of widget ID -> state value.
using WidgetStateMap = std::unordered_map<std::string, WidgetStateValue>;

} // namespace finegui
