/**
 * @file test_script_unit.cpp
 * @brief Unit tests for script engine integration (no Vulkan required)
 *
 * Tests:
 * - Widget converter: map → WidgetNode conversion
 * - Widget value extraction: WidgetNode → script Value
 * - Script bindings: ui.* functions produce correct maps
 * - ConverterSymbols interning
 */

#include <finegui/widget_converter.hpp>
#include <finegui/script_bindings.hpp>
#include <finegui/widget_node.hpp>
#include <finescript/script_engine.h>
#include <finescript/execution_context.h>
#include <finescript/map_data.h>

#include <iostream>
#include <cassert>
#include <string>

using namespace finegui;
using namespace finescript;

// ============================================================================
// Helper: create a script engine with gui bindings
// ============================================================================

static ScriptEngine& testEngine() {
    static ScriptEngine engine;
    static bool initialized = false;
    if (!initialized) {
        registerGuiBindings(engine);
        initialized = true;
    }
    return engine;
}

// ============================================================================
// ConverterSymbols Tests
// ============================================================================

void test_converter_symbols() {
    std::cout << "Testing: ConverterSymbols interning... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    // All symbols should be non-zero after interning
    assert(syms.type != 0);
    assert(syms.label != 0);
    assert(syms.on_click != 0);
    assert(syms.sym_window != 0);
    assert(syms.sym_button != 0);
    assert(syms.sym_separator != 0);

    // Same string should produce same ID
    assert(syms.type == engine.intern("type"));
    assert(syms.sym_button == engine.intern("button"));

    std::cout << "PASSED\n";
}

// ============================================================================
// convertToWidget Tests
// ============================================================================

void test_convert_button() {
    std::cout << "Testing: convertToWidget button... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    // Build a button map manually
    auto map = Value::map();
    map.asMap().set(engine.intern("type"), Value::symbol(engine.intern("button")));
    map.asMap().set(engine.intern("label"), Value::string("Click me"));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Button);
    assert(node.label == "Click me");

    std::cout << "PASSED\n";
}

void test_convert_slider() {
    std::cout << "Testing: convertToWidget slider... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(engine.intern("type"), Value::symbol(engine.intern("slider")));
    m.set(engine.intern("label"), Value::string("Volume"));
    m.set(engine.intern("min"), Value::number(0.0));
    m.set(engine.intern("max"), Value::number(1.0));
    m.set(engine.intern("value"), Value::number(0.5));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Slider);
    assert(node.label == "Volume");
    assert(node.minFloat == 0.0f);
    assert(node.maxFloat == 1.0f);
    assert(node.floatValue == 0.5f);

    std::cout << "PASSED\n";
}

void test_convert_checkbox_with_value() {
    std::cout << "Testing: convertToWidget checkbox with bool value... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(engine.intern("type"), Value::symbol(engine.intern("checkbox")));
    m.set(engine.intern("label"), Value::string("Enable"));
    m.set(engine.intern("value"), Value::boolean(true));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Checkbox);
    assert(node.label == "Enable");
    assert(node.boolValue == true);

    std::cout << "PASSED\n";
}

void test_convert_window_with_children() {
    std::cout << "Testing: convertToWidget window with children... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    // Build children array
    auto textMap = Value::map();
    textMap.asMap().set(engine.intern("type"), Value::symbol(engine.intern("text")));
    textMap.asMap().set(engine.intern("text"), Value::string("Hello"));

    auto btnMap = Value::map();
    btnMap.asMap().set(engine.intern("type"), Value::symbol(engine.intern("button")));
    btnMap.asMap().set(engine.intern("label"), Value::string("OK"));

    auto children = Value::array({textMap, btnMap});

    auto windowMap = Value::map();
    auto& wm = windowMap.asMap();
    wm.set(engine.intern("type"), Value::symbol(engine.intern("window")));
    wm.set(engine.intern("title"), Value::string("Test Window"));
    wm.set(engine.intern("children"), children);

    auto node = convertToWidget(windowMap, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Window);
    assert(node.label == "Test Window");
    assert(node.children.size() == 2);
    assert(node.children[0].type == WidgetNode::Type::Text);
    assert(node.children[0].textContent == "Hello");
    assert(node.children[1].type == WidgetNode::Type::Button);
    assert(node.children[1].label == "OK");

    std::cout << "PASSED\n";
}

void test_convert_combo() {
    std::cout << "Testing: convertToWidget combo... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(engine.intern("type"), Value::symbol(engine.intern("combo")));
    m.set(engine.intern("label"), Value::string("Resolution"));
    m.set(engine.intern("items"), Value::array({
        Value::string("1920x1080"),
        Value::string("2560x1440"),
    }));
    m.set(engine.intern("selected"), Value::integer(1));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Combo);
    assert(node.label == "Resolution");
    assert(node.items.size() == 2);
    assert(node.items[0] == "1920x1080");
    assert(node.items[1] == "2560x1440");
    assert(node.selectedIndex == 1);

    std::cout << "PASSED\n";
}

void test_convert_with_callback() {
    std::cout << "Testing: convertToWidget with on_click callback... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    // Create a script closure that sets a flag
    ctx.set("clicked", Value::boolean(false));
    auto result = engine.executeCommand(R"(
        fn [] do
            set clicked true
        end
    )", ctx);
    assert(result.success);
    assert(result.returnValue.isClosure());

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(engine.intern("type"), Value::symbol(engine.intern("button")));
    m.set(engine.intern("label"), Value::string("Test"));
    m.set(engine.intern("on_click"), result.returnValue);

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Button);
    assert(node.onClick);

    // Invoke the callback
    node.onClick(node);

    // Verify the script closure executed
    auto clickedVal = ctx.get("clicked");
    assert(clickedVal.isBool());
    assert(clickedVal.asBool() == true);

    std::cout << "PASSED\n";
}

void test_convert_visibility_enabled() {
    std::cout << "Testing: convertToWidget visible/enabled flags... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(engine.intern("type"), Value::symbol(engine.intern("button")));
    m.set(engine.intern("label"), Value::string("Hidden"));
    m.set(engine.intern("visible"), Value::boolean(false));
    m.set(engine.intern("enabled"), Value::boolean(false));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.visible == false);
    assert(node.enabled == false);

    std::cout << "PASSED\n";
}

// ============================================================================
// widgetValueToScriptValue Tests
// ============================================================================

void test_value_extraction() {
    std::cout << "Testing: widgetValueToScriptValue all types... ";

    WidgetNode checkbox;
    checkbox.type = WidgetNode::Type::Checkbox;
    checkbox.boolValue = true;
    auto v1 = widgetValueToScriptValue(checkbox);
    assert(v1.isBool());
    assert(v1.asBool() == true);

    WidgetNode slider;
    slider.type = WidgetNode::Type::Slider;
    slider.floatValue = 0.75f;
    auto v2 = widgetValueToScriptValue(slider);
    assert(v2.isFloat());
    assert(static_cast<float>(v2.asFloat()) == 0.75f);

    WidgetNode sliderInt;
    sliderInt.type = WidgetNode::Type::SliderInt;
    sliderInt.intValue = 42;
    auto v3 = widgetValueToScriptValue(sliderInt);
    assert(v3.isInt());
    assert(v3.asInt() == 42);

    WidgetNode inputText;
    inputText.type = WidgetNode::Type::InputText;
    inputText.stringValue = "hello";
    auto v4 = widgetValueToScriptValue(inputText);
    assert(v4.isString());
    assert(v4.asString() == "hello");

    WidgetNode combo;
    combo.type = WidgetNode::Type::Combo;
    combo.selectedIndex = 2;
    auto v5 = widgetValueToScriptValue(combo);
    assert(v5.isInt());
    assert(v5.asInt() == 2);

    WidgetNode inputInt;
    inputInt.type = WidgetNode::Type::InputInt;
    inputInt.intValue = 99;
    auto v6 = widgetValueToScriptValue(inputInt);
    assert(v6.isInt());
    assert(v6.asInt() == 99);

    WidgetNode inputFloat;
    inputFloat.type = WidgetNode::Type::InputFloat;
    inputFloat.floatValue = 3.14f;
    auto v7 = widgetValueToScriptValue(inputFloat);
    assert(v7.isFloat());

    // Default: returns nil
    WidgetNode text;
    text.type = WidgetNode::Type::Text;
    auto v8 = widgetValueToScriptValue(text);
    assert(v8.isNil());

    std::cout << "PASSED\n";
}

// ============================================================================
// Script Binding Tests (ui.* builder functions)
// ============================================================================

void test_binding_ui_button() {
    std::cout << "Testing: ui.button binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.button "Press me")", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("button"));

    auto labelVal = m.get(engine.intern("label"));
    assert(labelVal.isString());
    assert(labelVal.asString() == "Press me");

    std::cout << "PASSED\n";
}

void test_binding_ui_window() {
    std::cout << "Testing: ui.window binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.window "Settings" [{ui.text "Hello"} {ui.separator}])", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("window"));

    auto titleVal = m.get(engine.intern("title"));
    assert(titleVal.isString());
    assert(titleVal.asString() == "Settings");

    auto childrenVal = m.get(engine.intern("children"));
    assert(childrenVal.isArray());
    assert(childrenVal.asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_slider() {
    std::cout << "Testing: ui.slider binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.slider "Volume" 0.0 1.0 0.5)", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("slider"));
    assert(m.get(engine.intern("label")).asString() == "Volume");
    assert(m.get(engine.intern("min")).asNumber() == 0.0);
    assert(m.get(engine.intern("max")).asNumber() == 1.0);
    assert(m.get(engine.intern("value")).asNumber() == 0.5);

    std::cout << "PASSED\n";
}

void test_binding_ui_checkbox() {
    std::cout << "Testing: ui.checkbox binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.checkbox "Enable" true)", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("checkbox"));
    assert(m.get(engine.intern("label")).asString() == "Enable");
    assert(m.get(engine.intern("value")).asBool() == true);

    std::cout << "PASSED\n";
}

void test_binding_ui_combo() {
    std::cout << "Testing: ui.combo binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.combo "Res" ["1080p" "1440p"] 0)", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("combo"));
    assert(m.get(engine.intern("items")).isArray());
    assert(m.get(engine.intern("items")).asArray().size() == 2);
    assert(m.get(engine.intern("selected")).asInt() == 0);

    std::cout << "PASSED\n";
}

void test_binding_ui_separator() {
    std::cout << "Testing: ui.separator binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand("ui.separator", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("separator"));

    std::cout << "PASSED\n";
}

void test_binding_ui_group() {
    std::cout << "Testing: ui.group binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.group [{ui.text "A"} {ui.text "B"}])", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("group"));
    assert(m.get(engine.intern("children")).isArray());
    assert(m.get(engine.intern("children")).asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_columns() {
    std::cout << "Testing: ui.columns binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.columns 2 [{ui.text "Left"} {ui.text "Right"}])", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("columns"));
    assert(m.get(engine.intern("count")).asInt() == 2);
    assert(m.get(engine.intern("children")).asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_input() {
    std::cout << "Testing: ui.input binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.input "Name" "Alice")", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("input_text"));
    assert(m.get(engine.intern("label")).asString() == "Name");
    assert(m.get(engine.intern("value")).asString() == "Alice");

    std::cout << "PASSED\n";
}

void test_binding_roundtrip() {
    std::cout << "Testing: binding → convert roundtrip... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);
    ConverterSymbols syms;
    syms.intern(engine);

    // Build via script bindings
    auto result = engine.executeCommand(
        R"(ui.window "Settings" [
            {ui.text "Audio"}
            {ui.slider "Volume" 0.0 1.0 0.5}
            {ui.checkbox "Mute" false}
            {ui.separator}
            {ui.button "Apply"}
        ])", ctx);
    assert(result.success);

    // Convert to WidgetNode
    auto node = convertToWidget(result.returnValue, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Window);
    assert(node.label == "Settings");
    assert(node.children.size() == 5);
    assert(node.children[0].type == WidgetNode::Type::Text);
    assert(node.children[0].textContent == "Audio");
    assert(node.children[1].type == WidgetNode::Type::Slider);
    assert(node.children[1].label == "Volume");
    assert(node.children[1].floatValue == 0.5f);
    assert(node.children[2].type == WidgetNode::Type::Checkbox);
    assert(node.children[2].boolValue == false);
    assert(node.children[3].type == WidgetNode::Type::Separator);
    assert(node.children[4].type == WidgetNode::Type::Button);
    assert(node.children[4].label == "Apply");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 3 Binding Tests
// ============================================================================

void test_binding_ui_same_line() {
    std::cout << "Testing: ui.same_line binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.same_line})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());
    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("same_line"));

    // With offset via named parameter
    auto result2 = engine.executeCommand(R"({ui.same_line 100})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("offset")).asNumber() == 100.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_spacing() {
    std::cout << "Testing: ui.spacing binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.spacing})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());
    assert(result.returnValue.asMap().get(engine.intern("type")).asSymbol()
           == engine.intern("spacing"));

    std::cout << "PASSED\n";
}

void test_binding_ui_text_colored() {
    std::cout << "Testing: ui.text_colored binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.text_colored [1.0 0.3 0.3 1.0] "Error!"})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("text_colored"));
    assert(m.get(engine.intern("text")).asString() == "Error!");

    auto colorVal = m.get(engine.intern("color"));
    assert(colorVal.isArray());
    assert(colorVal.asArray().size() == 4);
    assert(colorVal.asArray()[0].asNumber() == 1.0);
    assert(colorVal.asArray()[1].asNumber() == 0.3);

    std::cout << "PASSED\n";
}

void test_binding_ui_text_wrapped() {
    std::cout << "Testing: ui.text_wrapped binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.text_wrapped "Long text"})", ctx);
    assert(result.success);
    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("text_wrapped"));
    assert(m.get(engine.intern("text")).asString() == "Long text");

    std::cout << "PASSED\n";
}

void test_binding_ui_text_disabled() {
    std::cout << "Testing: ui.text_disabled binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.text_disabled "Grayed out"})", ctx);
    assert(result.success);
    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("text_disabled"));
    assert(m.get(engine.intern("text")).asString() == "Grayed out");

    std::cout << "PASSED\n";
}

void test_binding_ui_progress_bar() {
    std::cout << "Testing: ui.progress_bar binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.progress_bar 0.75})", ctx);
    assert(result.success);
    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("progress_bar"));
    assert(m.get(engine.intern("value")).asNumber() == 0.75);

    std::cout << "PASSED\n";
}

void test_binding_ui_collapsing_header() {
    std::cout << "Testing: ui.collapsing_header binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.collapsing_header "Details" [{ui.text "Content"}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("collapsing_header"));
    assert(m.get(engine.intern("label")).asString() == "Details");
    auto children = m.get(engine.intern("children"));
    assert(children.isArray());
    assert(children.asArray().size() == 1);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 4 Binding Tests
// ============================================================================

void test_binding_ui_tab_bar() {
    std::cout << "Testing: ui.tab_bar binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.tab_bar "my_tabs" [{ui.tab "Tab1" [{ui.text "C1"}]}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("tab_bar"));
    assert(m.get(engine.intern("id")).asString() == "my_tabs");
    auto children = m.get(engine.intern("children"));
    assert(children.isArray());
    assert(children.asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_tab() {
    std::cout << "Testing: ui.tab binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.tab "Settings" [{ui.text "Content"}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("tab"));
    assert(m.get(engine.intern("label")).asString() == "Settings");
    assert(m.get(engine.intern("children")).asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_tree_node() {
    std::cout << "Testing: ui.tree_node binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.tree_node "Root" [{ui.tree_node "Child" []}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("tree_node"));
    assert(m.get(engine.intern("label")).asString() == "Root");
    assert(m.get(engine.intern("children")).asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_child() {
    std::cout << "Testing: ui.child binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.child "##scroll" [{ui.text "Content"}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("child"));
    assert(m.get(engine.intern("id")).asString() == "##scroll");
    assert(m.get(engine.intern("children")).asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_menu_bar() {
    std::cout << "Testing: ui.menu_bar binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.menu_bar [{ui.menu "File" [{ui.menu_item "New"}]}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("menu_bar"));
    auto children = m.get(engine.intern("children"));
    assert(children.isArray());
    assert(children.asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_menu() {
    std::cout << "Testing: ui.menu binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.menu "Edit" [{ui.menu_item "Undo"} {ui.menu_item "Redo"}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("menu"));
    assert(m.get(engine.intern("label")).asString() == "Edit");
    assert(m.get(engine.intern("children")).asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_menu_item() {
    std::cout << "Testing: ui.menu_item binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.menu_item "Save"})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("menu_item"));
    assert(m.get(engine.intern("label")).asString() == "Save");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 5 Binding Tests
// ============================================================================

void test_binding_ui_table() {
    std::cout << "Testing: ui.table binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.table "stats" 3 [{ui.text "cell"}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("table"));
    assert(m.get(engine.intern("id")).asString() == "stats");
    assert(m.get(engine.intern("num_columns")).asInt() == 3);
    assert(m.get(engine.intern("children")).asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_table_row() {
    std::cout << "Testing: ui.table_row binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.table_row [{ui.text "A"} {ui.text "B"}]})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("table_row"));
    assert(m.get(engine.intern("children")).asArray().size() == 2);

    // Bare table_row (no children)
    auto result2 = engine.executeCommand(R"({ui.table_row})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("type")).asSymbol() == engine.intern("table_row"));

    std::cout << "PASSED\n";
}

void test_binding_ui_table_next_column() {
    std::cout << "Testing: ui.table_next_column binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.table_next_column})", ctx);
    assert(result.success);

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("table_next_column"));

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 6 Binding Tests
// ============================================================================

void test_binding_ui_color_edit() {
    std::cout << "Testing: ui.color_edit binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.color_edit "Accent" [0.2 0.4 0.8 1.0]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("color_edit"));
    assert(m.get(engine.intern("label")).asString() == "Accent");
    auto color = m.get(engine.intern("color"));
    assert(color.isArray());
    assert(color.asArray().size() == 4);

    std::cout << "PASSED\n";
}

void test_binding_ui_color_picker() {
    std::cout << "Testing: ui.color_picker binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.color_picker "BG" [0.1 0.1 0.15 1.0]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("color_picker"));
    assert(m.get(engine.intern("label")).asString() == "BG");

    std::cout << "PASSED\n";
}

void test_binding_ui_drag_float() {
    std::cout << "Testing: ui.drag_float binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.drag_float "Speed" 1.5 0.1 0.0 10.0})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("drag_float"));
    assert(m.get(engine.intern("label")).asString() == "Speed");
    assert(m.get(engine.intern("value")).asFloat() == 1.5);
    assert(m.get(engine.intern("speed")).asFloat() == 0.1);
    assert(m.get(engine.intern("min")).asFloat() == 0.0);
    assert(m.get(engine.intern("max")).asFloat() == 10.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_drag_int() {
    std::cout << "Testing: ui.drag_int binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.drag_int "Count" 50 1.0 0 100})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("drag_int"));
    assert(m.get(engine.intern("label")).asString() == "Count");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 7 Binding Tests
// ============================================================================

void test_binding_ui_listbox() {
    std::cout << "Testing: ui.listbox binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.listbox "Fruits" ["Apple" "Banana" "Cherry"] 1 5})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("listbox"));
    assert(m.get(engine.intern("label")).asString() == "Fruits");
    assert(m.get(engine.intern("items")).isArray());
    assert(m.get(engine.intern("items")).asArray().size() == 3);
    assert(m.get(engine.intern("selected")).asInt() == 1);
    assert(m.get(engine.intern("height_in_items")).asNumber() == 5.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_popup() {
    std::cout << "Testing: ui.popup binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.popup "ctx_menu" [{ui.text "Cut"} {ui.text "Copy"}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("popup"));
    assert(m.get(engine.intern("id")).asString() == "ctx_menu");
    assert(m.get(engine.intern("children")).isArray());
    assert(m.get(engine.intern("children")).asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_modal() {
    std::cout << "Testing: ui.modal binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.modal "Confirm" [{ui.text "Are you sure?"}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("modal"));
    assert(m.get(engine.intern("title")).asString() == "Confirm");
    assert(m.get(engine.intern("children")).isArray());

    std::cout << "PASSED\n";
}

void test_binding_ui_open_popup() {
    std::cout << "Testing: ui.open_popup binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    // Create a popup, then open it
    auto result = engine.executeCommand(R"(
        set p {ui.popup "test_popup" []}
        ui.open_popup p
        p
    )", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    // After open_popup, :value should be true
    assert(m.get(engine.intern("value")).isBool());
    assert(m.get(engine.intern("value")).asBool() == true);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 8 Binding Tests
// ============================================================================

void test_binding_ui_canvas() {
    std::cout << "Testing: ui.canvas binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.canvas "##draw" 200 150})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("canvas"));
    assert(m.get(engine.intern("id")).asString() == "##draw");
    assert(m.get(engine.intern("width")).asNumber() == 200.0);
    assert(m.get(engine.intern("height")).asNumber() == 150.0);

    // Canvas with commands
    auto result2 = engine.executeCommand(
        R"({ui.canvas "##art" 100 100 [
            {ui.draw_line [10 10] [90 90] [1.0 0.0 0.0 1.0]}
        ]})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("commands")).isArray());
    assert(m2.get(engine.intern("commands")).asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_tooltip() {
    std::cout << "Testing: ui.tooltip binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    // Text tooltip
    auto result = engine.executeCommand(R"({ui.tooltip "Hover info"})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("tooltip"));
    assert(m.get(engine.intern("text")).asString() == "Hover info");

    // Children tooltip
    auto result2 = engine.executeCommand(
        R"({ui.tooltip [{ui.text "Line 1"} {ui.text "Line 2"}]})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("type")).asSymbol() == engine.intern("tooltip"));
    assert(m2.get(engine.intern("children")).isArray());
    assert(m2.get(engine.intern("children")).asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_draw_line() {
    std::cout << "Testing: ui.draw_line binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.draw_line [10 20] [90 80] [1.0 0.0 0.0 1.0] 2.0})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("draw_line"));
    assert(m.get(engine.intern("p1")).isArray());
    assert(m.get(engine.intern("p2")).isArray());
    assert(m.get(engine.intern("color")).isArray());
    assert(m.get(engine.intern("thickness")).asNumber() == 2.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_draw_rect() {
    std::cout << "Testing: ui.draw_rect binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.draw_rect [0 0] [100 50] [0.0 1.0 0.0 1.0] true})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("draw_rect"));
    assert(m.get(engine.intern("p1")).isArray());
    assert(m.get(engine.intern("p2")).isArray());
    assert(m.get(engine.intern("filled")).asBool() == true);

    std::cout << "PASSED\n";
}

void test_binding_ui_draw_circle() {
    std::cout << "Testing: ui.draw_circle binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.draw_circle [50 50] 25 [0.0 0.0 1.0 1.0] false 2.0})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("draw_circle"));
    assert(m.get(engine.intern("center")).isArray());
    assert(m.get(engine.intern("radius")).asNumber() == 25.0);
    assert(m.get(engine.intern("filled")).asBool() == false);
    assert(m.get(engine.intern("thickness")).asNumber() == 2.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_draw_text() {
    std::cout << "Testing: ui.draw_text binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.draw_text [10 10] "Hello" [1.0 1.0 1.0 1.0]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("draw_text"));
    assert(m.get(engine.intern("pos")).isArray());
    assert(m.get(engine.intern("text")).asString() == "Hello");
    assert(m.get(engine.intern("color")).isArray());

    std::cout << "PASSED\n";
}

void test_binding_ui_draw_triangle() {
    std::cout << "Testing: ui.draw_triangle binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.draw_triangle [50 10] [10 90] [90 90] [1.0 1.0 0.0 1.0] true})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("draw_triangle"));
    assert(m.get(engine.intern("p1")).isArray());
    assert(m.get(engine.intern("p2")).isArray());
    assert(m.get(engine.intern("center")).isArray());  // p3 stored as "center"
    assert(m.get(engine.intern("color")).isArray());
    assert(m.get(engine.intern("filled")).asBool() == true);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 9 binding tests
// ============================================================================

void test_binding_ui_radio_button() {
    std::cout << "Testing: ui.radio_button binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.radio_button "Option A" 0 1})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("radio_button"));
    assert(m.get(engine.intern("label")).asString() == "Option A");
    assert(m.get(engine.intern("value")).asInt() == 0);
    assert(m.get(engine.intern("my_value")).asInt() == 1);

    std::cout << "PASSED\n";
}

void test_binding_ui_selectable() {
    std::cout << "Testing: ui.selectable binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.selectable "Item 1" true})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("selectable"));
    assert(m.get(engine.intern("label")).asString() == "Item 1");
    assert(m.get(engine.intern("value")).asBool() == true);

    std::cout << "PASSED\n";
}

void test_binding_ui_input_multiline() {
    std::cout << "Testing: ui.input_multiline binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.input_multiline "Notes" "Hello" 400 300})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("input_multiline"));
    assert(m.get(engine.intern("label")).asString() == "Notes");
    assert(m.get(engine.intern("value")).asString() == "Hello");
    assert(m.get(engine.intern("width")).asNumber() == 400.0);
    assert(m.get(engine.intern("height")).asNumber() == 300.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_bullet_text() {
    std::cout << "Testing: ui.bullet_text binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.bullet_text "Important point"})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("bullet_text"));
    assert(m.get(engine.intern("text")).asString() == "Important point");

    std::cout << "PASSED\n";
}

void test_binding_ui_separator_text() {
    std::cout << "Testing: ui.separator_text binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.separator_text "Section A"})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("separator_text"));
    assert(m.get(engine.intern("label")).asString() == "Section A");

    std::cout << "PASSED\n";
}

void test_binding_ui_indent() {
    std::cout << "Testing: ui.indent / ui.unindent binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"({ui.indent 20})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());
    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("indent"));
    assert(m.get(engine.intern("width")).asNumber() == 20.0);

    auto result2 = engine.executeCommand(R"({ui.unindent 20})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("type")).asSymbol() == engine.intern("unindent"));
    assert(m2.get(engine.intern("width")).asNumber() == 20.0);

    std::cout << "PASSED\n";
}

// ============================================================================
// Image Binding Tests
// ============================================================================

void test_binding_ui_image() {
    std::cout << "Testing: ui.image binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    // Basic image with texture name
    auto result = engine.executeCommand(
        R"({ui.image "sword_icon" 48 32})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    assert(m.get(engine.intern("type")).asSymbol() == engine.intern("image"));
    assert(m.get(engine.intern("texture")).asString() == "sword_icon");
    assert(m.get(engine.intern("width")).asNumber() == 48.0);
    assert(m.get(engine.intern("height")).asNumber() == 32.0);

    // Image with just texture name (no size)
    auto result2 = engine.executeCommand(
        R"({ui.image "shield_icon"})", ctx);
    assert(result2.success);
    auto& m2 = result2.returnValue.asMap();
    assert(m2.get(engine.intern("type")).asSymbol() == engine.intern("image"));
    assert(m2.get(engine.intern("texture")).asString() == "shield_icon");

    std::cout << "PASSED\n";
}

void test_texture_symbol_interned() {
    std::cout << "Testing: Texture symbol interning... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.texture != 0);
    assert(syms.texture == engine.intern("texture"));
    assert(syms.sym_image != 0);
    assert(syms.sym_image == engine.intern("image"));

    std::cout << "PASSED\n";
}

// ============================================================================
// DnD Tests
// ============================================================================

void test_dnd_symbols_interned() {
    std::cout << "Testing: DnD symbols interning... ";

    auto& engine = testEngine();
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.drag_type != 0);
    assert(syms.drag_data != 0);
    assert(syms.drop_accept != 0);
    assert(syms.on_drop != 0);
    assert(syms.on_drag != 0);
    assert(syms.drag_mode != 0);

    std::cout << "PASSED\n";
}

void test_dnd_map_fields() {
    std::cout << "Testing: DnD map field round-trip... ";

    auto& engine = testEngine();
    ConverterSymbols syms;
    syms.intern(engine);

    auto w = Value::map();
    w.asMap().set(syms.drag_type, Value::string("item"));
    w.asMap().set(syms.drag_data, Value::string("sword"));
    w.asMap().set(syms.drop_accept, Value::string("item"));
    w.asMap().set(syms.drag_mode, Value::integer(2));

    auto dt = w.asMap().get(syms.drag_type);
    assert(dt.isString());
    assert(std::string(dt.asString()) == "item");

    auto dd = w.asMap().get(syms.drag_data);
    assert(dd.isString());
    assert(std::string(dd.asString()) == "sword");

    auto da = w.asMap().get(syms.drop_accept);
    assert(da.isString());
    assert(std::string(da.asString()) == "item");

    auto dm = w.asMap().get(syms.drag_mode);
    assert(dm.isInt());
    assert(dm.asInt() == 2);

    std::cout << "PASSED\n";
}

void test_dnd_convert_to_widget() {
    std::cout << "Testing: DnD convertToWidget fields... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);
    ConverterSymbols syms;
    syms.intern(engine);

    auto w = Value::map();
    w.asMap().set(syms.type, Value::symbol(syms.sym_button));
    w.asMap().set(syms.label, Value::string("Slot"));
    w.asMap().set(syms.drag_type, Value::string("item"));
    w.asMap().set(syms.drag_data, Value::string("sword_01"));
    w.asMap().set(syms.drop_accept, Value::string("item"));
    w.asMap().set(syms.drag_mode, Value::integer(1));

    auto node = convertToWidget(w, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Button);
    assert(node.dragType == "item");
    assert(node.dragData == "sword_01");
    assert(node.dropAcceptType == "item");
    assert(node.dragMode == 1);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 10 - Style Push/Pop Bindings
// ============================================================================

void test_binding_ui_push_color() {
    std::cout << "Testing: ui.push_color binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.push_color :button [0.2 0.1 0.1 1.0])", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("push_color"));

    // Value should be the integer ImGuiCol_Button (21)
    auto valVal = m.get(engine.intern("value"));
    assert(valVal.isInt());
    assert(valVal.asInt() == 21);  // ImGuiCol_Button

    // Color should be the array
    auto colorVal = m.get(engine.intern("color"));
    assert(colorVal.isArray());
    assert(colorVal.asArray().size() == 4);

    std::cout << "PASSED\n";
}

void test_binding_ui_pop_color() {
    std::cout << "Testing: ui.pop_color binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.pop_color 2)", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("pop_color"));

    auto countVal = m.get(engine.intern("count"));
    assert(countVal.isInt());
    assert(countVal.asInt() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_push_var_float() {
    std::cout << "Testing: ui.push_var (float) binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.push_var :frame_rounding 8.0)", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.asSymbol() == engine.intern("push_var"));

    auto valVal = m.get(engine.intern("value"));
    assert(valVal.isInt());
    // ImGuiStyleVar_FrameRounding = 12
    assert(valVal.asInt() == 12);

    auto sizeVal = m.get(engine.intern("size"));
    assert(sizeVal.isNumeric());
    assert(sizeVal.asNumber() == 8.0);

    std::cout << "PASSED\n";
}

void test_binding_ui_push_var_vec2() {
    std::cout << "Testing: ui.push_var (Vec2) binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"(ui.push_var :window_padding [12 12])", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto valVal = m.get(engine.intern("value"));
    assert(valVal.isInt());
    // ImGuiStyleVar_WindowPadding = 2
    assert(valVal.asInt() == 2);

    auto sizeVal = m.get(engine.intern("size"));
    assert(sizeVal.isArray());
    assert(sizeVal.asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_pop_var() {
    std::cout << "Testing: ui.pop_var binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(R"(ui.pop_var 3)", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.asSymbol() == engine.intern("pop_var"));

    auto countVal = m.get(engine.intern("count"));
    assert(countVal.isInt());
    assert(countVal.asInt() == 3);

    std::cout << "PASSED\n";
}

void test_style_symbols_interned() {
    std::cout << "Testing: Style push/pop type symbols interned... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.sym_push_color != 0);
    assert(syms.sym_pop_color != 0);
    assert(syms.sym_push_var != 0);
    assert(syms.sym_pop_var != 0);
    // All should be different
    assert(syms.sym_push_color != syms.sym_pop_color);
    assert(syms.sym_push_var != syms.sym_pop_var);
    assert(syms.sym_push_color != syms.sym_push_var);

    std::cout << "PASSED\n";
}

// ============================================================================
// Focus Management Tests
// ============================================================================

void test_focus_symbols_interned() {
    std::cout << "Testing: Focus symbols interning... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.focusable != 0);
    assert(syms.auto_focus != 0);
    assert(syms.on_focus != 0);
    assert(syms.on_blur != 0);

    assert(syms.focusable == engine.intern("focusable"));
    assert(syms.auto_focus == engine.intern("auto_focus"));
    assert(syms.on_focus == engine.intern("on_focus"));
    assert(syms.on_blur == engine.intern("on_blur"));

    std::cout << "PASSED\n";
}

void test_convert_focusable_false() {
    std::cout << "Testing: convertToWidget with focusable=false... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);
    ConverterSymbols syms;
    syms.intern(engine);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(syms.type, Value::symbol(syms.sym_button));
    m.set(syms.label, Value::string("Skip Me"));
    m.set(syms.focusable, Value::boolean(false));
    m.set(syms.auto_focus, Value::boolean(true));

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::Button);
    assert(node.label == "Skip Me");
    assert(node.focusable == false);
    assert(node.autoFocus == true);

    std::cout << "PASSED\n";
}

void test_convert_focus_callbacks() {
    std::cout << "Testing: convertToWidget with on_focus/on_blur callbacks... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);
    ExecutionContext ctx(engine);

    // Create closures
    ctx.set("focus_fired", Value::boolean(false));
    ctx.set("blur_fired", Value::boolean(false));

    auto focusResult = engine.executeCommand(R"(
        fn [] do
            set focus_fired true
        end
    )", ctx);
    assert(focusResult.success);

    auto blurResult = engine.executeCommand(R"(
        fn [] do
            set blur_fired true
        end
    )", ctx);
    assert(blurResult.success);

    auto map = Value::map();
    auto& m = map.asMap();
    m.set(syms.type, Value::symbol(syms.sym_input_text));
    m.set(syms.label, Value::string("Name"));
    m.set(syms.on_focus, focusResult.returnValue);
    m.set(syms.on_blur, blurResult.returnValue);

    auto node = convertToWidget(map, engine, ctx, syms);
    assert(node.type == WidgetNode::Type::InputText);
    assert(node.onFocus);
    assert(node.onBlur);

    // Invoke and verify
    node.onFocus(node);
    assert(ctx.get("focus_fired").asBool() == true);

    node.onBlur(node);
    assert(ctx.get("blur_fired").asBool() == true);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 13: Context Menu, Main Menu Bar, Close Popup
// ============================================================================

void test_binding_ui_context_menu() {
    std::cout << "Testing: ui.context_menu binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.context_menu [{ui.menu_item "Cut"} {ui.menu_item "Copy"}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("context_menu"));

    auto children = m.get(engine.intern("children"));
    assert(children.isArray());
    assert(children.asArray().size() == 2);

    std::cout << "PASSED\n";
}

void test_binding_ui_main_menu_bar() {
    std::cout << "Testing: ui.main_menu_bar binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.main_menu_bar [{ui.menu "File" [{ui.menu_item "New"}]}]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("main_menu_bar"));

    auto children = m.get(engine.intern("children"));
    assert(children.isArray());
    assert(children.asArray().size() == 1);

    std::cout << "PASSED\n";
}

void test_phase13_symbols_interned() {
    std::cout << "Testing: Phase 13 symbols interned... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.sym_context_menu != 0);
    assert(syms.sym_main_menu_bar != 0);
    assert(syms.sym_context_menu == engine.intern("context_menu"));
    assert(syms.sym_main_menu_bar == engine.intern("main_menu_bar"));

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 14 - ItemTooltip & ImageButton
// ============================================================================

void test_binding_ui_item_tooltip() {
    std::cout << "Testing: ui.item_tooltip binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.item_tooltip "Hover info"})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("item_tooltip"));

    auto textVal = m.get(engine.intern("text"));
    assert(textVal.isString());
    assert(textVal.asString() == "Hover info");

    std::cout << "PASSED\n";
}

void test_binding_ui_image_button() {
    std::cout << "Testing: ui.image_button binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.image_button "btn1" "sword" 48 48})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("image_button"));

    auto idVal = m.get(engine.intern("id"));
    assert(idVal.isString());
    assert(idVal.asString() == "btn1");

    auto texVal = m.get(engine.intern("texture"));
    assert(texVal.isString());
    assert(texVal.asString() == "sword");

    auto wVal = m.get(engine.intern("width"));
    assert(wVal.isNumeric());
    assert(wVal.asNumber() == 48.0);

    std::cout << "PASSED\n";
}

void test_phase14_symbols_interned() {
    std::cout << "Testing: Phase 14 symbols interned... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.sym_item_tooltip != 0);
    assert(syms.sym_image_button != 0);
    assert(syms.sym_item_tooltip == engine.intern("item_tooltip"));
    assert(syms.sym_image_button == engine.intern("image_button"));

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 15 - PlotLines & PlotHistogram
// ============================================================================

void test_binding_ui_plot_lines() {
    std::cout << "Testing: ui.plot_lines binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.plot_lines "FPS" [30 60 45] "avg" 0 100})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("plot_lines"));

    auto labelVal = m.get(engine.intern("label"));
    assert(labelVal.isString());
    assert(labelVal.asString() == "FPS");

    auto valArr = m.get(engine.intern("value"));
    assert(valArr.isArray());
    assert(valArr.asArray().size() == 3);

    auto overlayVal = m.get(engine.intern("overlay"));
    assert(overlayVal.isString());
    assert(overlayVal.asString() == "avg");

    std::cout << "PASSED\n";
}

void test_binding_ui_plot_histogram() {
    std::cout << "Testing: ui.plot_histogram binding... ";

    auto& engine = testEngine();
    ExecutionContext ctx(engine);

    auto result = engine.executeCommand(
        R"({ui.plot_histogram "Scores" [10 20 30]})", ctx);
    assert(result.success);
    assert(result.returnValue.isMap());

    auto& m = result.returnValue.asMap();
    auto typeVal = m.get(engine.intern("type"));
    assert(typeVal.isSymbol());
    assert(typeVal.asSymbol() == engine.intern("plot_histogram"));

    auto labelVal = m.get(engine.intern("label"));
    assert(labelVal.isString());
    assert(labelVal.asString() == "Scores");

    auto valArr = m.get(engine.intern("value"));
    assert(valArr.isArray());
    assert(valArr.asArray().size() == 3);

    std::cout << "PASSED\n";
}

void test_phase15_symbols_interned() {
    std::cout << "Testing: Phase 15 symbols interned... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    assert(syms.sym_plot_lines != 0);
    assert(syms.sym_plot_histogram != 0);
    assert(syms.sym_plot_lines == engine.intern("plot_lines"));
    assert(syms.sym_plot_histogram == engine.intern("plot_histogram"));

    std::cout << "PASSED\n";
}

void test_window_control_symbols_interned() {
    std::cout << "Testing: Window control symbols interned... ";

    ScriptEngine engine;
    ConverterSymbols syms;
    syms.intern(engine);

    // New window flags
    assert(syms.sym_flag_no_nav != 0);
    assert(syms.sym_flag_no_inputs != 0);
    assert(syms.sym_flag_no_nav == engine.intern("no_nav"));
    assert(syms.sym_flag_no_inputs == engine.intern("no_inputs"));

    // Window size fields
    assert(syms.window_size_w != 0);
    assert(syms.window_size_h != 0);
    assert(syms.window_size_w == engine.intern("window_size_w"));
    assert(syms.window_size_h == engine.intern("window_size_h"));

    std::cout << "PASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== finegui Script Integration Unit Tests ===\n\n";

    try {
        // Converter tests
        test_converter_symbols();
        test_convert_button();
        test_convert_slider();
        test_convert_checkbox_with_value();
        test_convert_window_with_children();
        test_convert_combo();
        test_convert_with_callback();
        test_convert_visibility_enabled();

        // Value extraction tests
        test_value_extraction();

        // Binding tests
        test_binding_ui_button();
        test_binding_ui_window();
        test_binding_ui_slider();
        test_binding_ui_checkbox();
        test_binding_ui_combo();
        test_binding_ui_separator();
        test_binding_ui_group();
        test_binding_ui_columns();
        test_binding_ui_input();

        // Roundtrip test
        test_binding_roundtrip();

        // Phase 3 binding tests
        test_binding_ui_same_line();
        test_binding_ui_spacing();
        test_binding_ui_text_colored();
        test_binding_ui_text_wrapped();
        test_binding_ui_text_disabled();
        test_binding_ui_progress_bar();
        test_binding_ui_collapsing_header();

        // Phase 4 binding tests
        test_binding_ui_tab_bar();
        test_binding_ui_tab();
        test_binding_ui_tree_node();
        test_binding_ui_child();
        test_binding_ui_menu_bar();
        test_binding_ui_menu();
        test_binding_ui_menu_item();

        // Phase 5 binding tests
        test_binding_ui_table();
        test_binding_ui_table_row();
        test_binding_ui_table_next_column();

        // Phase 6 binding tests
        test_binding_ui_color_edit();
        test_binding_ui_color_picker();
        test_binding_ui_drag_float();
        test_binding_ui_drag_int();

        // Phase 7 binding tests
        test_binding_ui_listbox();
        test_binding_ui_popup();
        test_binding_ui_modal();
        test_binding_ui_open_popup();

        // Phase 8 binding tests
        test_binding_ui_canvas();
        test_binding_ui_tooltip();
        test_binding_ui_draw_line();
        test_binding_ui_draw_rect();
        test_binding_ui_draw_circle();
        test_binding_ui_draw_text();
        test_binding_ui_draw_triangle();

        // Phase 9 binding tests
        test_binding_ui_radio_button();
        test_binding_ui_selectable();
        test_binding_ui_input_multiline();
        test_binding_ui_bullet_text();
        test_binding_ui_separator_text();
        test_binding_ui_indent();

        // Image binding tests
        test_binding_ui_image();
        test_texture_symbol_interned();

        // DnD tests
        test_dnd_symbols_interned();
        test_dnd_map_fields();
        test_dnd_convert_to_widget();

        // Phase 10 - Style push/pop
        test_binding_ui_push_color();
        test_binding_ui_pop_color();
        test_binding_ui_push_var_float();
        test_binding_ui_push_var_vec2();
        test_binding_ui_pop_var();
        test_style_symbols_interned();

        // Focus management tests
        test_focus_symbols_interned();
        test_convert_focusable_false();
        test_convert_focus_callbacks();

        // Phase 13 - Context Menu, Main Menu Bar
        test_binding_ui_context_menu();
        test_binding_ui_main_menu_bar();
        test_phase13_symbols_interned();

        // Phase 14 - ItemTooltip & ImageButton
        test_binding_ui_item_tooltip();
        test_binding_ui_image_button();
        test_phase14_symbols_interned();

        // Phase 15 - PlotLines & PlotHistogram
        test_binding_ui_plot_lines();
        test_binding_ui_plot_histogram();
        test_phase15_symbols_interned();

        // Window Control
        test_window_control_symbols_interned();

        std::cout << "\n=== All script integration unit tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
