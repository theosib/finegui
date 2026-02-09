#include <finegui/script_bindings.hpp>
#include <finegui/script_gui.hpp>
#include <finescript/execution_context.h>
#include <finescript/map_data.h>
#include <finescript/native_function.h>

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

    // Register the gui namespace
    engine.registerConstant("gui", gui);
}

} // namespace finegui
