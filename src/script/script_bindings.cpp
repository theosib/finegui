#include <finegui/script_bindings.hpp>
#include <finegui/script_gui.hpp>
#include <finescript/execution_context.h>
#include <finescript/map_data.h>
#include <finescript/native_function.h>
#include <imgui.h>
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

// Helper: create a widget map with a type symbol
static Value makeWidget(ScriptEngine& engine, const char* typeName) {
    auto w = Value::map();
    w.asMap().set(engine.intern("type"), Value::symbol(engine.intern(typeName)));
    return w;
}

void registerGuiBindings(ScriptEngine& engine) {

    // =========================================================================
    // Build the "ui" namespace map
    // =========================================================================

    auto ui = Value::map();
    auto& uiMap = ui.asMap();

    // ui.window "title" [children]
    uiMap.set(engine.intern("window"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "window");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("title"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.text "content"
    uiMap.set(engine.intern("text"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "text");
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(engine.intern("text"), args[0]);
            }
            return w;
        }));

    // ui.button "label" [on_click]
    uiMap.set(engine.intern("button"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "button");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isCallable()) {
                m.set(engine.intern("on_click"), args[1]);
            }
            return w;
        }));

    // ui.checkbox "label" value [on_change]
    uiMap.set(engine.intern("checkbox"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "checkbox");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isBool()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            return w;
        }));

    // ui.slider "label" min max value [on_change]
    uiMap.set(engine.intern("slider"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "slider");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("min"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("max"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("value"), args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(engine.intern("on_change"), args[4]);
            }
            return w;
        }));

    // ui.slider_int "label" min max value [on_change]
    uiMap.set(engine.intern("slider_int"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "slider_int");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("min"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("max"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("value"), args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(engine.intern("on_change"), args[4]);
            }
            return w;
        }));

    // ui.input "label" value [on_change] [on_submit]
    uiMap.set(engine.intern("input"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "input_text");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(engine.intern("on_submit"), args[3]);
            }
            return w;
        }));

    // ui.input_int "label" value [on_change]
    uiMap.set(engine.intern("input_int"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "input_int");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            return w;
        }));

    // ui.input_float "label" value [on_change]
    uiMap.set(engine.intern("input_float"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "input_float");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            return w;
        }));

    // ui.combo "label" items selected [on_change]
    uiMap.set(engine.intern("combo"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "combo");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("items"), args[1]);
            }
            if (args.size() > 2 && args[2].isInt()) {
                m.set(engine.intern("selected"), args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(engine.intern("on_change"), args[3]);
            }
            return w;
        }));

    // ui.separator
    uiMap.set(engine.intern("separator"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(engine, "separator");
        }));

    // ui.group [children]
    uiMap.set(engine.intern("group"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "group");
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(engine.intern("children"), args[0]);
            }
            return w;
        }));

    // ui.columns count [children]
    uiMap.set(engine.intern("columns"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "columns");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isInt()) {
                m.set(engine.intern("count"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.image "texture_name" [width] [height] [on_click]
    uiMap.set(engine.intern("image"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "image");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("texture"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("width"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("height"), args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(engine.intern("on_click"), args[3]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 3 - Layout & Display
    // =========================================================================

    // ui.same_line [=offset x]
    uiMap.set(engine.intern("same_line"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "same_line");
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(engine.intern("offset"), args[0]);
            }
            return w;
        }));

    // ui.spacing
    uiMap.set(engine.intern("spacing"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(engine, "spacing");
        }));

    // ui.text_colored [r g b a] "text"
    uiMap.set(engine.intern("text_colored"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "text_colored");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isArray()) {
                m.set(engine.intern("color"), args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(engine.intern("text"), args[1]);
            }
            return w;
        }));

    // ui.text_wrapped "text"
    uiMap.set(engine.intern("text_wrapped"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "text_wrapped");
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(engine.intern("text"), args[0]);
            }
            return w;
        }));

    // ui.text_disabled "text"
    uiMap.set(engine.intern("text_disabled"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "text_disabled");
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(engine.intern("text"), args[0]);
            }
            return w;
        }));

    // ui.progress_bar fraction [=overlay text] [=size [w h]]
    uiMap.set(engine.intern("progress_bar"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "progress_bar");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isNumeric()) {
                m.set(engine.intern("value"), args[0]);
            }
            return w;
        }));

    // ui.collapsing_header "label" [children] [=default_open bool]
    uiMap.set(engine.intern("collapsing_header"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "collapsing_header");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 4 - Containers & Menus
    // =========================================================================

    // ui.tab_bar "id" [children]
    uiMap.set(engine.intern("tab_bar"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "tab_bar");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("id"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.tab "label" [children]
    uiMap.set(engine.intern("tab"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "tab");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.tree_node "label" [children]
    uiMap.set(engine.intern("tree_node"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "tree_node");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.child "id" [children]
    uiMap.set(engine.intern("child"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "child");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("id"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.menu_bar [children]
    uiMap.set(engine.intern("menu_bar"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "menu_bar");
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(engine.intern("children"), args[0]);
            }
            return w;
        }));

    // ui.menu "label" [children]
    uiMap.set(engine.intern("menu"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "menu");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.menu_item "label" [on_click]
    uiMap.set(engine.intern("menu_item"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "menu_item");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isCallable()) {
                m.set(engine.intern("on_click"), args[1]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 6 - Advanced Input
    // =========================================================================

    // ui.color_edit "label" [r g b a] [on_change]
    uiMap.set(engine.intern("color_edit"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "color_edit");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("color"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            return w;
        }));

    // ui.color_picker "label" [r g b a] [on_change]
    uiMap.set(engine.intern("color_picker"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "color_picker");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("color"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_change"), args[2]);
            }
            return w;
        }));

    // ui.drag_float "label" value speed min max [on_change]
    uiMap.set(engine.intern("drag_float"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "drag_float");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("speed"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("min"), args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(engine.intern("max"), args[4]);
            }
            if (args.size() > 5 && args[5].isCallable()) {
                m.set(engine.intern("on_change"), args[5]);
            }
            return w;
        }));

    // ui.drag_int "label" value speed min max [on_change]
    uiMap.set(engine.intern("drag_int"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "drag_int");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("speed"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("min"), args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(engine.intern("max"), args[4]);
            }
            if (args.size() > 5 && args[5].isCallable()) {
                m.set(engine.intern("on_change"), args[5]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 7 - Misc
    // =========================================================================

    // ui.listbox "label" items [selected] [height_in_items] [on_change]
    uiMap.set(engine.intern("listbox"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "listbox");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("items"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("selected"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("height_in_items"), args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(engine.intern("on_change"), args[4]);
            }
            return w;
        }));

    // ui.popup "id" [children]
    uiMap.set(engine.intern("popup"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "popup");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("id"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            return w;
        }));

    // ui.modal "title" [children] [on_close]
    uiMap.set(engine.intern("modal"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "modal");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("title"), args[0]);
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("children"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_close"), args[2]);
            }
            return w;
        }));

    // ui.open_popup popup_map  ->  sets :value to true on a popup/modal map
    uiMap.set(engine.intern("open_popup"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            if (args.size() > 0 && args[0].isMap()) {
                // Copy the Value to get mutable access (shares the same MapData via shared_ptr)
                Value mapCopy = args[0];
                mapCopy.asMap().set(engine.intern("value"), Value::boolean(true));
            }
            return Value::nil();
        }));

    // =========================================================================
    // Phase 8 - Custom
    // =========================================================================

    // ui.canvas "id" width height [commands]
    uiMap.set(engine.intern("canvas"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "canvas");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("id"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("width"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("height"), args[2]);
            }
            if (args.size() > 3 && args[3].isArray()) {
                m.set(engine.intern("commands"), args[3]);
            }
            return w;
        }));

    // ui.tooltip "text"  OR  ui.tooltip [children]
    uiMap.set(engine.intern("tooltip"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "tooltip");
            auto& m = w.asMap();
            if (args.size() > 0) {
                if (args[0].isString()) {
                    m.set(engine.intern("text"), args[0]);
                } else if (args[0].isArray()) {
                    m.set(engine.intern("children"), args[0]);
                }
            }
            return w;
        }));

    // Draw command constructors for Canvas

    // ui.draw_line [x1 y1] [x2 y2] [r g b a] [thickness]
    uiMap.set(engine.intern("draw_line"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "draw_line");
            auto& m = w.asMap();
            if (args.size() > 0) m.set(engine.intern("p1"), args[0]);
            if (args.size() > 1) m.set(engine.intern("p2"), args[1]);
            if (args.size() > 2) m.set(engine.intern("color"), args[2]);
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("thickness"), args[3]);
            }
            return w;
        }));

    // ui.draw_rect [x1 y1] [x2 y2] [r g b a] [filled] [thickness]
    uiMap.set(engine.intern("draw_rect"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "draw_rect");
            auto& m = w.asMap();
            if (args.size() > 0) m.set(engine.intern("p1"), args[0]);
            if (args.size() > 1) m.set(engine.intern("p2"), args[1]);
            if (args.size() > 2) m.set(engine.intern("color"), args[2]);
            if (args.size() > 3 && args[3].isBool()) {
                m.set(engine.intern("filled"), args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(engine.intern("thickness"), args[4]);
            }
            return w;
        }));

    // ui.draw_circle [cx cy] radius [r g b a] [filled] [thickness]
    uiMap.set(engine.intern("draw_circle"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "draw_circle");
            auto& m = w.asMap();
            if (args.size() > 0) m.set(engine.intern("center"), args[0]);
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("radius"), args[1]);
            }
            if (args.size() > 2) m.set(engine.intern("color"), args[2]);
            if (args.size() > 3 && args[3].isBool()) {
                m.set(engine.intern("filled"), args[3]);
            }
            if (args.size() > 4 && args[4].isNumeric()) {
                m.set(engine.intern("thickness"), args[4]);
            }
            return w;
        }));

    // ui.draw_text [x y] "text" [r g b a]
    uiMap.set(engine.intern("draw_text"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "draw_text");
            auto& m = w.asMap();
            if (args.size() > 0) m.set(engine.intern("pos"), args[0]);
            if (args.size() > 1 && args[1].isString()) {
                m.set(engine.intern("text"), args[1]);
            }
            if (args.size() > 2) m.set(engine.intern("color"), args[2]);
            return w;
        }));

    // ui.draw_triangle [x1 y1] [x2 y2] [x3 y3] [r g b a] [filled] [thickness]
    uiMap.set(engine.intern("draw_triangle"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "draw_triangle");
            auto& m = w.asMap();
            if (args.size() > 0) m.set(engine.intern("p1"), args[0]);
            if (args.size() > 1) m.set(engine.intern("p2"), args[1]);
            if (args.size() > 2) m.set(engine.intern("center"), args[2]);
            if (args.size() > 3) m.set(engine.intern("color"), args[3]);
            if (args.size() > 4 && args[4].isBool()) {
                m.set(engine.intern("filled"), args[4]);
            }
            if (args.size() > 5 && args[5].isNumeric()) {
                m.set(engine.intern("thickness"), args[5]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 9 - New widgets
    // =========================================================================

    // ui.radio_button "label" active_value my_value [on_change]
    uiMap.set(engine.intern("radio_button"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "radio_button");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("my_value"), args[2]);
            }
            if (args.size() > 3 && args[3].isCallable()) {
                m.set(engine.intern("on_change"), args[3]);
            }
            return w;
        }));

    // ui.selectable "label" [selected] [on_click]
    uiMap.set(engine.intern("selectable"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "selectable");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isBool()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isCallable()) {
                m.set(engine.intern("on_click"), args[2]);
            }
            return w;
        }));

    // ui.input_multiline "label" value [width] [height] [on_change]
    uiMap.set(engine.intern("input_multiline"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "input_multiline");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("label"), args[0]);
            }
            if (args.size() > 1 && args[1].isString()) {
                m.set(engine.intern("value"), args[1]);
            }
            if (args.size() > 2 && args[2].isNumeric()) {
                m.set(engine.intern("width"), args[2]);
            }
            if (args.size() > 3 && args[3].isNumeric()) {
                m.set(engine.intern("height"), args[3]);
            }
            if (args.size() > 4 && args[4].isCallable()) {
                m.set(engine.intern("on_change"), args[4]);
            }
            return w;
        }));

    // ui.bullet_text "text"
    uiMap.set(engine.intern("bullet_text"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "bullet_text");
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(engine.intern("text"), args[0]);
            }
            return w;
        }));

    // ui.separator_text "label"
    uiMap.set(engine.intern("separator_text"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "separator_text");
            if (args.size() > 0 && args[0].isString()) {
                w.asMap().set(engine.intern("label"), args[0]);
            }
            return w;
        }));

    // ui.indent [width]
    uiMap.set(engine.intern("indent"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "indent");
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(engine.intern("width"), args[0]);
            }
            return w;
        }));

    // ui.unindent [width]
    uiMap.set(engine.intern("unindent"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "unindent");
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(engine.intern("width"), args[0]);
            }
            return w;
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
    uiMap.set(engine.intern("push_color"), makeFn(
        [&engine, colorSymbolMap](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "push_color");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isSymbol()) {
                auto it = colorSymbolMap.find(args[0].asSymbol());
                if (it != colorSymbolMap.end()) {
                    m.set(engine.intern("value"), Value::integer(it->second));
                }
            }
            if (args.size() > 1 && args[1].isArray()) {
                m.set(engine.intern("color"), args[1]);
            }
            return w;
        }));

    // ui.pop_color [count]
    uiMap.set(engine.intern("pop_color"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "pop_color");
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(engine.intern("count"), args[0]);
            }
            return w;
        }));

    // ui.push_var :symbol value_or_array
    uiMap.set(engine.intern("push_var"), makeFn(
        [&engine, varSymbolMap](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "push_var");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isSymbol()) {
                auto it = varSymbolMap.find(args[0].asSymbol());
                if (it != varSymbolMap.end()) {
                    m.set(engine.intern("value"), Value::integer(it->second));
                }
            }
            if (args.size() > 1) {
                m.set(engine.intern("size"), args[1]);
            }
            return w;
        }));

    // ui.pop_var [count]
    uiMap.set(engine.intern("pop_var"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "pop_var");
            if (args.size() > 0 && args[0].isNumeric()) {
                w.asMap().set(engine.intern("count"), args[0]);
            }
            return w;
        }));

    // =========================================================================
    // Phase 5 - Tables
    // =========================================================================

    // ui.table "id" num_cols [children]
    uiMap.set(engine.intern("table"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "table");
            auto& m = w.asMap();
            if (args.size() > 0 && args[0].isString()) {
                m.set(engine.intern("id"), args[0]);
            }
            if (args.size() > 1 && args[1].isNumeric()) {
                m.set(engine.intern("num_columns"), args[1]);
            }
            if (args.size() > 2 && args[2].isArray()) {
                m.set(engine.intern("children"), args[2]);
            }
            return w;
        }));

    // ui.table_row [children]
    uiMap.set(engine.intern("table_row"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>& args) -> Value {
            auto w = makeWidget(engine, "table_row");
            if (args.size() > 0 && args[0].isArray()) {
                w.asMap().set(engine.intern("children"), args[0]);
            }
            return w;
        }));

    // ui.table_next_column
    uiMap.set(engine.intern("table_next_column"), makeFn(
        [&engine](ExecutionContext&, const std::vector<Value>&) -> Value {
            return makeWidget(engine, "table_next_column");
        }));

    // =========================================================================
    // Action functions (require ScriptGui context via ctx.userData())
    // =========================================================================

    // ui.show map  ->  stores map in MapRenderer, returns ID
    uiMap.set(engine.intern("show"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isMap()) {
                return Value::nil();
            }
            return gui->scriptShow(args[0]);
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

    // ui.find "widget_id"  ->  find a widget map by :id string
    uiMap.set(engine.intern("find"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isString()) {
                return Value::nil();
            }
            return gui->scriptFindById(std::string(args[0].asString()));
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

    // Register the gui namespace
    engine.registerConstant("gui", gui);
}

} // namespace finegui
