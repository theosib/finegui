#include <finegui/script_bindings.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/widget_converter.hpp>
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
    // Action functions (require ScriptGui context via ctx.userData())
    // =========================================================================

    // ui.show map  ->  converts map->WidgetNode, shows via renderer, returns ID
    uiMap.set(engine.intern("show"), makeFn(
        [&engine](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.empty() || !args[0].isMap()) {
                return Value::nil();
            }
            return gui->scriptShow(args[0], engine, ctx);
        }));

    // ui.update id map  ->  re-converts and replaces tree
    uiMap.set(engine.intern("update"), makeFn(
        [&engine](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui || args.size() < 2 || !args[0].isInt() || !args[1].isMap()) {
                return Value::nil();
            }
            gui->scriptUpdate(static_cast<int>(args[0].asInt()), args[1], engine, ctx);
            return Value::nil();
        }));

    // ui.hide [id]  ->  removes tree from renderer
    uiMap.set(engine.intern("hide"), makeFn(
        [](ExecutionContext& ctx, const std::vector<Value>& args) -> Value {
            auto* gui = static_cast<ScriptGui*>(ctx.userData());
            if (!gui) return Value::nil();
            if (!args.empty() && args[0].isInt()) {
                gui->scriptHide(static_cast<int>(args[0].asInt()));
            } else {
                gui->close();
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

    // Register the gui namespace
    engine.registerConstant("gui", gui);
}

} // namespace finegui
