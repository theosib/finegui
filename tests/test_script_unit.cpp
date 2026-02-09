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

        std::cout << "\n=== All script integration unit tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
