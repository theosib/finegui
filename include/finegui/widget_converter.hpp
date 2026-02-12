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

    // Phase 3 field keys
    uint32_t color = 0, overlay = 0, size = 0;
    uint32_t offset = 0, default_open = 0;
    uint32_t format = 0;

    // Phase 4 field keys
    uint32_t border = 0, auto_scroll = 0;
    uint32_t shortcut = 0, checked = 0, leaf = 0;

    // Phase 5 field keys
    uint32_t num_columns = 0, headers = 0, flags = 0;

    // Phase 6 field keys
    uint32_t speed = 0;

    // Phase 7 field keys
    uint32_t height_in_items = 0;

    // Phase 8 field keys (Canvas draw commands)
    uint32_t p1 = 0, p2 = 0, center = 0, pos = 0;
    uint32_t radius = 0, thickness = 0, filled = 0;
    uint32_t commands = 0;
    uint32_t bg_color = 0;

    // Table flag value symbols (for :flags array parsing)
    uint32_t sym_flag_row_bg = 0, sym_flag_borders = 0;
    uint32_t sym_flag_borders_h = 0, sym_flag_borders_v = 0;
    uint32_t sym_flag_borders_inner = 0, sym_flag_borders_outer = 0;
    uint32_t sym_flag_resizable = 0, sym_flag_sortable = 0;
    uint32_t sym_flag_scroll_x = 0, sym_flag_scroll_y = 0;

    // Callback keys
    uint32_t on_click = 0, on_change = 0, on_submit = 0, on_close = 0;
    uint32_t on_select = 0;

    // Type name symbols - Phase 1
    uint32_t sym_window = 0, sym_text = 0, sym_button = 0;
    uint32_t sym_checkbox = 0, sym_slider = 0, sym_slider_int = 0;
    uint32_t sym_input_text = 0, sym_input_int = 0, sym_input_float = 0;
    uint32_t sym_combo = 0, sym_separator = 0, sym_group = 0;
    uint32_t sym_columns = 0, sym_image = 0;

    // Type name symbols - Phase 3 (Layout & Display)
    uint32_t sym_same_line = 0, sym_spacing = 0;
    uint32_t sym_text_colored = 0, sym_text_wrapped = 0, sym_text_disabled = 0;
    uint32_t sym_progress_bar = 0, sym_collapsing_header = 0;

    // Type name symbols - Phase 4 (Containers & Menus)
    uint32_t sym_tab_bar = 0, sym_tab = 0, sym_tree_node = 0, sym_child = 0;
    uint32_t sym_menu_bar = 0, sym_menu = 0, sym_menu_item = 0;

    // Type name symbols - Phase 5 (Tables)
    uint32_t sym_table = 0, sym_table_row = 0, sym_table_next_column = 0;

    // Type name symbols - Phase 6 (Advanced Input)
    uint32_t sym_color_edit = 0, sym_color_picker = 0;
    uint32_t sym_drag_float = 0, sym_drag_int = 0;

    // Type name symbols - Phase 7 (Misc)
    uint32_t sym_listbox = 0, sym_popup = 0, sym_modal = 0;

    // Type name symbols - Phase 8 (Custom)
    uint32_t sym_canvas = 0, sym_tooltip = 0;
    // Canvas draw command type symbols
    uint32_t sym_draw_line = 0, sym_draw_rect = 0;
    uint32_t sym_draw_circle = 0, sym_draw_text = 0;
    uint32_t sym_draw_triangle = 0;

    // Type name symbols - Phase 9
    uint32_t sym_radio_button = 0, sym_selectable = 0;
    uint32_t sym_input_multiline = 0;
    uint32_t sym_bullet_text = 0, sym_separator_text = 0;
    uint32_t sym_indent = 0, sym_unindent = 0;

    // Type name symbols - Phase 10 (Style push/pop)
    uint32_t sym_push_color = 0, sym_pop_color = 0;
    uint32_t sym_push_var = 0, sym_pop_var = 0;

    // Type name symbols - Phase 11 (Layout helpers)
    uint32_t sym_dummy = 0, sym_new_line = 0;

    // Type name symbols - Phase 12 (Advanced Input continued)
    uint32_t sym_drag_float3 = 0, sym_input_with_hint = 0;
    uint32_t sym_slider_angle = 0, sym_small_button = 0, sym_color_button = 0;

    // Type name symbols - Phase 13 (Menus & Popups continued)
    uint32_t sym_context_menu = 0, sym_main_menu_bar = 0;

    // Type name symbols - Phase 14 (Tooltips & Images continued)
    uint32_t sym_item_tooltip = 0, sym_image_button = 0;

    // Type name symbols - Phase 15 (Display plots)
    uint32_t sym_plot_lines = 0, sym_plot_histogram = 0;

    // Type name symbols - Style & Theming (Named presets)
    uint32_t sym_push_theme = 0, sym_pop_theme = 0;

    // Phase 12 field keys
    uint32_t hint = 0;

    // Phase 9 field keys
    uint32_t my_value = 0;

    // Image field keys
    uint32_t texture = 0;

    // Focus management field keys
    uint32_t focusable = 0, auto_focus = 0;
    uint32_t on_focus = 0, on_blur = 0;

    // Animation field keys
    uint32_t window_alpha = 0, window_pos_x = 0, window_pos_y = 0;
    uint32_t scale_x = 0, scale_y = 0, rotation_y = 0;

    // DnD field keys
    uint32_t drag_type = 0, drag_data = 0, drop_accept = 0;
    uint32_t on_drop = 0, on_drag = 0, drag_mode = 0;

    // Window flag value symbols (for :window_flags array parsing)
    uint32_t window_flags = 0;
    uint32_t sym_flag_no_title_bar = 0, sym_flag_no_resize = 0;
    uint32_t sym_flag_no_move = 0, sym_flag_no_scrollbar = 0;
    uint32_t sym_flag_no_collapse = 0, sym_flag_always_auto_resize = 0;
    uint32_t sym_flag_no_background = 0, sym_flag_menu_bar = 0;
    uint32_t sym_flag_no_nav = 0, sym_flag_no_inputs = 0;

    // Window size field keys
    uint32_t window_size_w = 0, window_size_h = 0;

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
