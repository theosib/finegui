#pragma once

#include <finegui/widget_node.hpp>
#include <finescript/value.h>
#include <finescript/script_engine.h>
#include <finescript/execution_context.h>
#include <cstdint>

namespace finegui {

/// Pre-interned symbol IDs for fast map key lookup during conversion.
/// Call intern() once at startup to populate.
struct ConverterSymbols {
    // Field keys
    uint32_t type = 0, label = 0, title = 0, text = 0, value = 0;
    uint32_t min = 0, max = 0, id = 0;
    uint32_t children = 0, items = 0;
    uint32_t width = 0, height = 0, count = 0;
    uint32_t visible = 0, enabled = 0;
    uint32_t selected = 0;

    // Callback keys
    uint32_t on_click = 0, on_change = 0, on_submit = 0, on_close = 0;

    // Type name symbols
    uint32_t sym_window = 0, sym_text = 0, sym_button = 0;
    uint32_t sym_checkbox = 0, sym_slider = 0, sym_slider_int = 0;
    uint32_t sym_input_text = 0, sym_input_int = 0, sym_input_float = 0;
    uint32_t sym_combo = 0, sym_separator = 0, sym_group = 0;
    uint32_t sym_columns = 0, sym_image = 0;

    void intern(finescript::ScriptEngine& engine);
};

/// Convert a finescript map hierarchy into a WidgetNode tree.
/// The map should have been created by ui.window, ui.button, etc.
/// Script closures in on_click, on_change, etc. are wrapped as
/// C++ WidgetCallback lambdas that call back into the script engine.
WidgetNode convertToWidget(const finescript::Value& map,
                           finescript::ScriptEngine& engine,
                           finescript::ExecutionContext& ctx,
                           const ConverterSymbols& syms);

/// Convert a WidgetNode's current value into a finescript Value.
/// Used to pass widget state back to script callbacks.
finescript::Value widgetValueToScriptValue(const WidgetNode& widget);

} // namespace finegui
