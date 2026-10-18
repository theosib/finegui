#include <finegui/script_bindings.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/widget_converter.hpp>
#include <finescript/execution_context.h>
#include <finescript/map_data.h>
#include <finescript/native_function.h>
#include <imgui.h>
#include <memory>
#include <unordered_map>

namespace finegui {

using finescript::Value;
using finescript::ExecutionContext;
using finescript::ScriptEngine;
using finescript::SimpleLambdaFunction;

// Helper: create a Value wrapping a native function lambda
static Value makeFn(std::function<Value(ExecutionContext&, const std::vector<Value>&)> fn) {
    return Value::nativeFunction(std::make_shared<SimpleLambdaFunction>(std::move(fn)));
}

// Helper: create a widget map with a pre-interned type symbol
static Value makeWidget(const ConverterSymbols& syms, uint32_t typeSym) {
    auto w = Value::map();
    w.asMap().set(syms.type, Value::symbol(typeSym));
    return w;
}

// Helper: if the last arg is a map, merge its keys into the widget map.
// This enables keyword-style parameters: {ui.slider "Vol" {=value 0.5 =min 0 =max 1}}
static void mergeOptions(const std::vector<Value>& args, Value& widget, const ConverterSymbols& syms) {
    if (!args.empty() && args.back().isMap()) {
        auto& opts = args.back().asMap();
        auto& w = widget.asMap();
        for (auto key : opts.keys()) {
            if (key != syms.type) {
                w.set(key, opts.get(key));
            }
        }
    }
}

void registerGuiBindings(ScriptEngine& engine) {
    auto syms = std::make_shared<ConverterSymbols>();
    syms->intern(engine);

    // =========================================================================
    // Build the "ui" namespace map
    // =========================================================================

    auto ui = Value::map();
    auto& uiMap = ui.asMap();

    // ui.window "title" [children]
    uiMap.set(syms->sym_window, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_window);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->title, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.text "content"
    uiMap.set(syms->sym_text, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_text);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->text, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.button "label" [on_click]
    uiMap.set(syms->sym_button, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_button);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isCallable()) {
                m.set(syms->on_click, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.checkbox "label" [value] [on_change]
    uiMap.set(syms->sym_checkbox, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_checkbox);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isBool()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.slider "label" [value] [min] [max] [on_change]
    uiMap.set(syms->sym_slider, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_slider);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->min, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->max, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_change, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.slider_int "label" [value] [min] [max] [on_change]
    uiMap.set(syms->sym_slider_int, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_slider_int);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->min, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->max, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_change, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.input "label" [value] [on_change] [on_submit]
    uiMap.set(engine.intern("input"), makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_input_text);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(syms->on_submit, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.input_int "label" [value] [on_change]
    uiMap.set(syms->sym_input_int, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_input_int);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.input_float "label" [value] [on_change]
    uiMap.set(syms->sym_input_float, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_input_float);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.combo "label" [items] [selected] [on_change]
    uiMap.set(syms->sym_combo, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_combo);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->items, args[1]);
            }
            if (args.size() > 2 && args[2].isInt()) {
                m.set(syms->selected, args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(syms->on_change, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.separator
    uiMap.set(syms->sym_separator, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(*syms, syms->sym_separator);
        }));

    // ui.group [children]
    uiMap.set(syms->sym_group, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_group);
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(syms->children, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.columns count [children]
    uiMap.set(syms->sym_columns, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_columns);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isInt()) {
                m.set(syms->count, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.image "texture_name" [width] [height] [on_click]
    uiMap.set(syms->sym_image, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_image);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->texture, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->width, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->height, args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(syms->on_click, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 3 - Layout & Display
    // =========================================================================

    // ui.same_line [offset]
    uiMap.set(syms->sym_same_line, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_same_line);
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(syms->offset, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.spacing
    uiMap.set(syms->sym_spacing, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(*syms, syms->sym_spacing);
        }));

    // ui.text_colored [r g b a] "text"
    uiMap.set(syms->sym_text_colored, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_text_colored);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isArray()) {
                m.set(syms->color, args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->text, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.text_wrapped "text"
    uiMap.set(syms->sym_text_wrapped, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_text_wrapped);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->text, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.text_disabled "text"
    uiMap.set(syms->sym_text_disabled, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_text_disabled);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->text, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.progress_bar fraction
    uiMap.set(syms->sym_progress_bar, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_progress_bar);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isNumeric()) {
                m.set(syms->value, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.collapsing_header "label" [children]
    uiMap.set(syms->sym_collapsing_header, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_collapsing_header);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 4 - Containers & Menus
    // =========================================================================

    // ui.tab_bar "id" [children]
    uiMap.set(syms->sym_tab_bar, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_tab_bar);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.tab "label" [children]
    uiMap.set(syms->sym_tab, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_tab);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.tree_node "label" [children]
    uiMap.set(syms->sym_tree_node, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_tree_node);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.child "id" [children]
    uiMap.set(syms->sym_child, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_child);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.menu_bar [children]
    uiMap.set(syms->sym_menu_bar, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_menu_bar);
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(syms->children, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.menu "label" [children]
    uiMap.set(syms->sym_menu, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_menu);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.menu_item "label" [on_click]
    uiMap.set(syms->sym_menu_item, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_menu_item);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isCallable()) {
                m.set(syms->on_click, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 6 - Advanced Input
    // =========================================================================

    // ui.color_edit "label" [color] [on_change]
    uiMap.set(syms->sym_color_edit, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_color_edit);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->color, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.color_picker "label" [color] [on_change]
    uiMap.set(syms->sym_color_picker, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_color_picker);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->color, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_change, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.drag_float "label" [value] [speed] [min] [max] [on_change]
    uiMap.set(syms->sym_drag_float, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_drag_float);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->speed, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->min, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->max, args[4]);
            }
            if (args.size() > 5 && args[5].isCallable()) {
                m.set(syms->on_change, args[5]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.drag_int "label" [value] [speed] [min] [max] [on_change]
    uiMap.set(syms->sym_drag_int, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_drag_int);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->speed, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->min, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->max, args[4]);
            }
            if (args.size() > 5 && args[5].isCallable()) {
                m.set(syms->on_change, args[5]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.drag_float3 "label" [value] [speed] [min] [max] [on_change]
    uiMap.set(syms->sym_drag_float3, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_drag_float3);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->speed, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->min, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->max, args[4]);
            }
            if (args.size() > 5 && args[5].isCallable()) {
                m.set(syms->on_change, args[5]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.input_with_hint "label" "hint" [value] [on_change] [on_submit]
    uiMap.set(syms->sym_input_with_hint, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_input_with_hint);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->hint, args[1]);
            }
            if (args.size() > 2 && args[2].isString()) {
                m.set(syms->value, args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(syms->on_change, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_submit, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.slider_angle "label" [value_rad] [min_deg] [max_deg] [on_change]
    uiMap.set(syms->sym_slider_angle, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_slider_angle);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->min, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->max, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_change, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.small_button "label" [on_click]
    uiMap.set(syms->sym_small_button, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_small_button);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isCallable()) {
                m.set(syms->on_click, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.color_button "label" [color] [on_click]
    uiMap.set(syms->sym_color_button, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_color_button);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->color, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_click, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 7 - Misc
    // =========================================================================

    // ui.listbox "label" [items] [selected] [height_in_items] [on_change]
    uiMap.set(syms->sym_listbox, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_listbox);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->items, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->selected, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->height_in_items, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_change, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.popup "id" [children]
    uiMap.set(syms->sym_popup, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_popup);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.modal "title" [children] [on_close]
    uiMap.set(syms->sym_modal, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_modal);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->title, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->children, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_close, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.open_popup popup_map  ->  sets :value to true on a popup/modal map
    uiMap.set(engine.intern("open_popup"), makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            if (args.size() > 0 && args[0].isMap()) {
                // Copy the Value to get mutable access (shares the same MapData via shared_ptr)
                Value mapCopy = args[0];
                mapCopy.asMap().set(syms->value, Value::boolean(true));
            }
            return Value::nil();
        }));

    // ui.close_popup  ->  closes the current popup (must be called during popup rendering)
    uiMap.set(engine.intern("close_popup"), makeFn(
        [](ExecutionContext&, const std::vector<Value>&) -> Value {
            ImGui::CloseCurrentPopup();
            return Value::nil();
        }));

    // ui.context_menu [children]
    uiMap.set(syms->sym_context_menu, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_context_menu);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isArray()) {
                m.set(syms->children, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.main_menu_bar [children]
    uiMap.set(syms->sym_main_menu_bar, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_main_menu_bar);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isArray()) {
                m.set(syms->children, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 14 - Tooltips & Images (continued)
    // =========================================================================

    // ui.item_tooltip "text" or ui.item_tooltip [children]
    uiMap.set(syms->sym_item_tooltip, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_item_tooltip);
            auto& m = w.asMap();
            if (args.size() > 0) {
                if (args[0].isString()) {
                    m.set(syms->text, args[0]);
                } else if (args[0].isArray()) {
                    m.set(syms->children, args[0]);
                }
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.image_button "id" "texture_name" [width] [height] [on_click]
    uiMap.set(syms->sym_image_button, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_image_button);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->texture, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->width, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->height, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_click, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 15 - Display (plots)
    // =========================================================================

    // ui.plot_lines "label" [values] [overlay] [min] [max] [width] [height]
    uiMap.set(syms->sym_plot_lines, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_plot_lines);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isString()) {
                m.set(syms->overlay, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->min, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->max, args[4]);
            }
            if (args.size() > 5 && args[5].isNumeric()) {
                m.set(syms->width, args[5]);
            }
            if (args.size() > 6 && args[6].isNumeric()) {
                m.set(syms->height, args[6]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.plot_histogram "label" [values] [overlay] [min] [max] [width] [height]
    uiMap.set(syms->sym_plot_histogram, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_plot_histogram);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isString()) {
                m.set(syms->overlay, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->min, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->max, args[4]);
            }
            if (args.size() > 5 && args[5].isNumeric()) {
                m.set(syms->width, args[5]);
            }
            if (args.size() > 6 && args[6].isNumeric()) {
                m.set(syms->height, args[6]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 8 - Custom
    // =========================================================================

    // ui.canvas "id" width height [commands]
    uiMap.set(syms->sym_canvas, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_canvas);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->width, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->height, args[2]);
            }
            if (args.size() > 3 && args[3].isArray()) {
                m.set(syms->commands, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.tooltip "text"  OR  ui.tooltip [children]
    uiMap.set(syms->sym_tooltip, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_tooltip);
            auto& m = w.asMap();
            if (args.size() > 0) {
                if (args[0].isString()) {
                    m.set(syms->text, args[0]);
                } else if (args[0].isArray()) {
                    m.set(syms->children, args[0]);
                }
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // Draw command constructors for Canvas

    // ui.draw_line [x1 y1] [x2 y2] [r g b a] [thickness]
    uiMap.set(syms->sym_draw_line, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_draw_line);
            auto& m = w.asMap();
            if (args.size() > 0) m.set(syms->p1, args[0]);
            if (args.size() > 1) m.set(syms->p2, args[1]);
            if (args.size() > 2) m.set(syms->color, args[2]);
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->thickness, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.draw_rect [x1 y1] [x2 y2] [r g b a] [filled] [thickness]
    uiMap.set(syms->sym_draw_rect, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_draw_rect);
            auto& m = w.asMap();
            if (args.size() > 0) m.set(syms->p1, args[0]);
            if (args.size() > 1) m.set(syms->p2, args[1]);
            if (args.size() > 2) m.set(syms->color, args[2]);
            if (args.size() > 3 && args[3].isBool()) {
                m.set(syms->filled, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->thickness, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.draw_circle [cx cy] radius [r g b a] [filled] [thickness]
    uiMap.set(syms->sym_draw_circle, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_draw_circle);
            auto& m = w.asMap();
            if (args.size() > 0) m.set(syms->center, args[0]);
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->radius, args[1]);
            }
            if (args.size() > 2) m.set(syms->color, args[2]);
            if (args.size() > 3 && args[3].isBool()) {
                m.set(syms->filled, args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(syms->thickness, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.draw_text [x y] "text" [r g b a]
    uiMap.set(syms->sym_draw_text, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_draw_text);
            auto& m = w.asMap();
            if (args.size() > 0) m.set(syms->pos, args[0]);
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->text, args[1]);
            }
            if (args.size() > 2) m.set(syms->color, args[2]);
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.draw_triangle [x1 y1] [x2 y2] [x3 y3] [r g b a] [filled] [thickness]
    uiMap.set(syms->sym_draw_triangle, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_draw_triangle);
            auto& m = w.asMap();
            if (args.size() > 0) m.set(syms->p1, args[0]);
            if (args.size() > 1) m.set(syms->p2, args[1]);
            if (args.size() > 2) m.set(syms->center, args[2]);
            if (args.size() > 3) m.set(syms->color, args[3]);
            if (args.size() > 4 && args[4].isBool()) {
                m.set(syms->filled, args[4]);
            }
            if (args.size() > 5 && args[5].isNumeric()) {
                m.set(syms->thickness, args[5]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Phase 9 - New widgets
    // =========================================================================

    // ui.radio_button "label" [active_value] [my_value] [on_change]
    uiMap.set(syms->sym_radio_button, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_radio_button);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->my_value, args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(syms->on_change, args[3]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.selectable "label" [selected] [on_click]
    uiMap.set(syms->sym_selectable, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_selectable);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isBool()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(syms->on_click, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.input_multiline "label" [value] [width] [height] [on_change]
    uiMap.set(syms->sym_input_multiline, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_input_multiline);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->label, args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(syms->value, args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(syms->width, args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(syms->height, args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(syms->on_change, args[4]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.bullet_text "text"
    uiMap.set(syms->sym_bullet_text, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_bullet_text);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->text, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.separator_text "label"
    uiMap.set(syms->sym_separator_text, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_separator_text);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->label, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.indent [width]
    uiMap.set(syms->sym_indent, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_indent);
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(syms->width, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.unindent [width]
    uiMap.set(syms->sym_unindent, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_unindent);
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(syms->width, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.dummy width height
    uiMap.set(syms->sym_dummy, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_dummy);
            if (args.size() >= 2 && args[0].isNumeric() && args[1].isNumeric()) {
                w.asMap().set(syms->width, args[0]);
                w.asMap().set(syms->height, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.new_line
    uiMap.set(syms->sym_new_line, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(*syms, syms->sym_new_line);
        }));

    // =========================================================================
    // Phase 10 - Style Push/Pop
    // =========================================================================

    // Build color symbol → ImGuiCol_ lookup map
    std::unordered_map<uint32_t, int> colorSymbolMap;
    auto addCol = [&](const char* name, int val) {
        colorSymbolMap[engine.intern(name)] = val;
    };
    addCol("text", ImGuiCol_Text);
    addCol("text_disabled", ImGuiCol_TextDisabled);
    addCol("window_bg", ImGuiCol_WindowBg);
    addCol("child_bg", ImGuiCol_ChildBg);
    addCol("popup_bg", ImGuiCol_PopupBg);
    addCol("border", ImGuiCol_Border);
    addCol("border_shadow", ImGuiCol_BorderShadow);
    addCol("frame_bg", ImGuiCol_FrameBg);
    addCol("frame_bg_hovered", ImGuiCol_FrameBgHovered);
    addCol("frame_bg_active", ImGuiCol_FrameBgActive);
    addCol("title_bg", ImGuiCol_TitleBg);
    addCol("title_bg_active", ImGuiCol_TitleBgActive);
    addCol("title_bg_collapsed", ImGuiCol_TitleBgCollapsed);
    addCol("menu_bar_bg", ImGuiCol_MenuBarBg);
    addCol("scrollbar_bg", ImGuiCol_ScrollbarBg);
    addCol("scrollbar_grab", ImGuiCol_ScrollbarGrab);
    addCol("scrollbar_grab_hovered", ImGuiCol_ScrollbarGrabHovered);
    addCol("scrollbar_grab_active", ImGuiCol_ScrollbarGrabActive);
    addCol("check_mark", ImGuiCol_CheckMark);
    addCol("slider_grab", ImGuiCol_SliderGrab);
    addCol("slider_grab_active", ImGuiCol_SliderGrabActive);
    addCol("button", ImGuiCol_Button);
    addCol("button_hovered", ImGuiCol_ButtonHovered);
    addCol("button_active", ImGuiCol_ButtonActive);
    addCol("header", ImGuiCol_Header);
    addCol("header_hovered", ImGuiCol_HeaderHovered);
    addCol("header_active", ImGuiCol_HeaderActive);
    addCol("separator", ImGuiCol_Separator);
    addCol("separator_hovered", ImGuiCol_SeparatorHovered);
    addCol("separator_active", ImGuiCol_SeparatorActive);
    addCol("resize_grip", ImGuiCol_ResizeGrip);
    addCol("resize_grip_hovered", ImGuiCol_ResizeGripHovered);
    addCol("resize_grip_active", ImGuiCol_ResizeGripActive);
    addCol("input_text_cursor", ImGuiCol_InputTextCursor);
    addCol("tab", ImGuiCol_Tab);
    addCol("tab_hovered", ImGuiCol_TabHovered);
    addCol("tab_selected", ImGuiCol_TabSelected);
    addCol("tab_selected_overline", ImGuiCol_TabSelectedOverline);
    addCol("tab_dimmed", ImGuiCol_TabDimmed);
    addCol("tab_dimmed_selected", ImGuiCol_TabDimmedSelected);
    addCol("tab_dimmed_selected_overline", ImGuiCol_TabDimmedSelectedOverline);
    addCol("plot_lines", ImGuiCol_PlotLines);
    addCol("plot_lines_hovered", ImGuiCol_PlotLinesHovered);
    addCol("plot_histogram", ImGuiCol_PlotHistogram);
    addCol("plot_histogram_hovered", ImGuiCol_PlotHistogramHovered);
    addCol("table_header_bg", ImGuiCol_TableHeaderBg);
    addCol("table_border_strong", ImGuiCol_TableBorderStrong);
    addCol("table_border_light", ImGuiCol_TableBorderLight);
    addCol("table_row_bg", ImGuiCol_TableRowBg);
    addCol("table_row_bg_alt", ImGuiCol_TableRowBgAlt);
    addCol("text_link", ImGuiCol_TextLink);
    addCol("text_selected_bg", ImGuiCol_TextSelectedBg);
    addCol("tree_lines", ImGuiCol_TreeLines);
    addCol("drag_drop_target", ImGuiCol_DragDropTarget);
    addCol("drag_drop_target_bg", ImGuiCol_DragDropTargetBg);
    addCol("unsaved_marker", ImGuiCol_UnsavedMarker);
    addCol("nav_cursor", ImGuiCol_NavCursor);
    addCol("nav_windowing_highlight", ImGuiCol_NavWindowingHighlight);
    addCol("nav_windowing_dim_bg", ImGuiCol_NavWindowingDimBg);
    addCol("modal_window_dim_bg", ImGuiCol_ModalWindowDimBg);

    // Build var symbol → ImGuiStyleVar_ lookup map
    std::unordered_map<uint32_t, int> varSymbolMap;
    auto addVar = [&](const char* name, int val) {
        varSymbolMap[engine.intern(name)] = val;
    };
    addVar("alpha", ImGuiStyleVar_Alpha);
    addVar("disabled_alpha", ImGuiStyleVar_DisabledAlpha);
    addVar("window_padding", ImGuiStyleVar_WindowPadding);
    addVar("window_rounding", ImGuiStyleVar_WindowRounding);
    addVar("window_border_size", ImGuiStyleVar_WindowBorderSize);
    addVar("window_min_size", ImGuiStyleVar_WindowMinSize);
    addVar("window_title_align", ImGuiStyleVar_WindowTitleAlign);
    addVar("child_rounding", ImGuiStyleVar_ChildRounding);
    addVar("child_border_size", ImGuiStyleVar_ChildBorderSize);
    addVar("popup_rounding", ImGuiStyleVar_PopupRounding);
    addVar("popup_border_size", ImGuiStyleVar_PopupBorderSize);
    addVar("frame_padding", ImGuiStyleVar_FramePadding);
    addVar("frame_rounding", ImGuiStyleVar_FrameRounding);
    addVar("frame_border_size", ImGuiStyleVar_FrameBorderSize);
    addVar("item_spacing", ImGuiStyleVar_ItemSpacing);
    addVar("item_inner_spacing", ImGuiStyleVar_ItemInnerSpacing);
    addVar("indent_spacing", ImGuiStyleVar_IndentSpacing);
    addVar("cell_padding", ImGuiStyleVar_CellPadding);
    addVar("scrollbar_size", ImGuiStyleVar_ScrollbarSize);
    addVar("scrollbar_rounding", ImGuiStyleVar_ScrollbarRounding);
    addVar("scrollbar_padding", ImGuiStyleVar_ScrollbarPadding);
    addVar("grab_min_size", ImGuiStyleVar_GrabMinSize);
    addVar("grab_rounding", ImGuiStyleVar_GrabRounding);
    addVar("image_rounding", ImGuiStyleVar_ImageRounding);
    addVar("image_border_size", ImGuiStyleVar_ImageBorderSize);
    addVar("tab_rounding", ImGuiStyleVar_TabRounding);
    addVar("tab_border_size", ImGuiStyleVar_TabBorderSize);
    addVar("tab_min_width_base", ImGuiStyleVar_TabMinWidthBase);
    addVar("tab_min_width_shrink", ImGuiStyleVar_TabMinWidthShrink);
    addVar("tab_bar_border_size", ImGuiStyleVar_TabBarBorderSize);
    addVar("tab_bar_overline_size", ImGuiStyleVar_TabBarOverlineSize);
    addVar("table_angled_headers_angle", ImGuiStyleVar_TableAngledHeadersAngle);
    addVar("table_angled_headers_text_align", ImGuiStyleVar_TableAngledHeadersTextAlign);
    addVar("tree_lines_size", ImGuiStyleVar_TreeLinesSize);
    addVar("tree_lines_rounding", ImGuiStyleVar_TreeLinesRounding);
    addVar("button_text_align", ImGuiStyleVar_ButtonTextAlign);
    addVar("selectable_text_align", ImGuiStyleVar_SelectableTextAlign);
    addVar("separator_text_border_size", ImGuiStyleVar_SeparatorTextBorderSize);
    addVar("separator_text_align", ImGuiStyleVar_SeparatorTextAlign);
    addVar("separator_text_padding", ImGuiStyleVar_SeparatorTextPadding);

    // ui.push_color :symbol [r g b a]
    uiMap.set(syms->sym_push_color, makeFn(
        [syms, colorSymbolMap = std::move(colorSymbolMap)](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_push_color);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isSymbol()) {
                auto it = colorSymbolMap.find(args[0].asSymbol());
                if (it != colorSymbolMap.end()) {
                    m.set(syms->value, Value::integer(it->second));
                }
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(syms->color, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.pop_color [count]
    uiMap.set(syms->sym_pop_color, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_pop_color);
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(syms->count, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.push_var :symbol value_or_array
    uiMap.set(syms->sym_push_var, makeFn(
        [syms, varSymbolMap = std::move(varSymbolMap)](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_push_var);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isSymbol()) {
                auto it = varSymbolMap.find(args[0].asSymbol());
                if (it != varSymbolMap.end()) {
                    m.set(syms->value, Value::integer(it->second));
                }
            }
            if (args.size() > 1) {
                m.set(syms->size, args[1]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.pop_var [count]
    uiMap.set(syms->sym_pop_var, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_pop_var);
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(syms->count, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // =========================================================================
    // Style & Theming - Named presets
    // =========================================================================

    // ui.push_theme "name"  ->  pushes a named style preset
    uiMap.set(syms->sym_push_theme, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_push_theme);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->label, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.pop_theme "name"  ->  pops a named style preset
    uiMap.set(syms->sym_pop_theme, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_pop_theme);
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(syms->label, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.set_theme "dark"/"light"/"classic"  ->  immediate action, switches global theme
    uiMap.set(engine.intern("set_theme"), makeFn(
        [](ExecutionContext&, const std::vector<Value>& args) -> Value {
            if (args.size() > 0 && args[0].isString()) {
                auto name = std::string(args[0].asString());
                if (name == "dark") {
                    ImGui::StyleColorsDark();
                } else if (name == "light") {
                    ImGui::StyleColorsLight();
                } else if (name == "classic") {
                    ImGui::StyleColorsClassic();
                }
            }
            return Value::nil();
        }));

    // =========================================================================
    // Phase 5 - Tables
    // =========================================================================

    // ui.table "id" num_cols [children]
    uiMap.set(syms->sym_table, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_table);
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(syms->id, args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(syms->num_columns, args[1]);
            }
            if (args.size() > 2 && args[2].isArray()) {
                m.set(syms->children, args[2]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.table_row [children]
    uiMap.set(syms->sym_table_row, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(*syms, syms->sym_table_row);
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(syms->children, args[0]);
            }
            mergeOptions(args, w, *syms);
            return w;
        }));

    // ui.table_next_column
    uiMap.set(syms->sym_table_next_column, makeFn(
        [syms](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(*syms, syms->sym_table_next_column);
        }));

    // =========================================================================
    // Action functions (require ScriptGui context via ctx.userData())
    // =========================================================================

    // ui.show map [=immediate true]  ->  stores map in MapRenderer, returns ID
    uiMap.set(engine.intern("show"), makeFn(
        [syms](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isMap()) {
                return Value::nil();
            }
            // Check for =immediate kwarg in trailing map
            bool immediate = false;
            if (args.size() > 1 && args.back().isMap()) {
                auto imm = args.back().asMap().get(syms->immediate);
                if (imm.isBool()) immediate = imm.asBool();
            }
            return gui->scriptShow(args[0], immediate);
        }));

    // ui.stage map  ->  stores map without rendering, returns ID
    uiMap.set(engine.intern("stage"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isMap()) {
                return Value::nil();
            }
            return gui->scriptStage(args[0]);
        }));

    // ui.go_live id  ->  begin rendering a staged tree
    uiMap.set(engine.intern("go_live"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isInt()) {
                return Value::nil();
            }
            gui->scriptGoLive(static_cast<int>(args[0].asInt()));
            return Value::nil();
        }));

    // ui.hide  ->  removes tree from renderer
    uiMap.set(engine.intern("hide"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>&) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui) return Value::nil();
            gui->scriptHide();
            return Value::nil();
        }));

    // ui.node gui_id [path]  ->  returns live child map from the tree
    //   No path: returns root map
    //   Integer: returns children[idx]
    //   Array of ints: navigates nested children
    uiMap.set(engine.intern("node"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isInt()) {
                return Value::nil();
            }
            int guiId = static_cast<int>(args[0].asInt());

            // No path → return root map
            if (args.size() < 2 || args[1].isNil()) {
                auto* root = gui->mapTree();
                if (!root || gui->guiId() != guiId) return Value::nil();
                return *root;
            }

            return gui->navigateMap(guiId, args[1]);
        }));

    // ui.save_state  ->  returns a map of {widget_id => value} for all widgets with :id
    uiMap.set(engine.intern("save_state"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>&) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui) return Value::map();
            return gui->scriptSaveState();
        }));

    // ui.load_state map  ->  restores widget state from a previously saved map
    uiMap.set(engine.intern("load_state"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isMap()) {
                return Value::nil();
            }
            gui->scriptLoadState(args[0]);
            return Value::nil();
        }));

    // ui.find "widget_id" or ui.find :widget_id  ->  find a widget map by :id
    uiMap.set(engine.intern("find"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty()) return Value::nil();
            if (args[0].isString()) {
                return gui->scriptFindById(std::string(args[0].asString()));
            } else if (args[0].isSymbol()) {
                return gui->scriptFindById(args[0].asSymbol());
            }
            return Value::nil();
        }));

    // Register the ui namespace
    engine.registerConstant("ui", ui);

    // =========================================================================
    // Build the "gui" namespace map
    // =========================================================================

    auto gui = Value::map();
    auto& guiMap = gui.asMap();

    // gui.on_message :symbol handler  ->  registers message handler
    guiMap.set(engine.intern("on_message"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* sg = static_cast<ScriptGui*>(ctx.userData());
            if (!sg || args.size() < 2 || !args[0].isSymbol() || !args[1].isCallable()) {
                return Value::nil();
            }
            sg->registerMessageHandler(args[0].asSymbol(), args[1]);
            return Value::nil();
        }));

    // gui.set_focus "widget_id"  ->  programmatically focus a widget by ID
    guiMap.set(engine.intern("set_focus"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* sg = static_cast<ScriptGui*>(ctx.userData());
            if (sg && !args.empty() && args[0].isString()) {
                sg->scriptSetFocus(std::string(args[0].asString()));
            }
            return Value::nil();
        }));

    // gui.bind_key "chord" callback  ->  bind a hotkey, returns binding ID
    guiMap.set(engine.intern("bind_key"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* sg = static_cast<ScriptGui*>(ctx.userData());
            if (!sg || args.size() < 2 || !args[0].isString() || !args[1].isCallable()) {
                return Value::integer(-1);
            }
            int id = sg->scriptBindKey(std::string(args[0].asString()), args[1]);
            return Value::integer(id);
        }));

    // gui.unbind_key id  ->  unbind a hotkey by ID
    guiMap.set(engine.intern("unbind_key"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* sg = static_cast<ScriptGui*>(ctx.userData());
            if (!sg || args.empty() || !args[0].isInt()) {
                return Value::nil();
            }
            sg->scriptUnbindKey(static_cast<int>(args[0].asInt()));
            return Value::nil();
        }));

    // Register the gui namespace
    engine.registerConstant("gui", gui);
}

} // namespace finegui
