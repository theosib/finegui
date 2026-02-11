/**
 * @file test_script_render.cpp
 * @brief Integration tests for script-driven GUI rendering (requires Vulkan)
 *
 * Tests the map-based rendering path where finescript maps ARE the widget data.
 * Uses a single Vulkan context for all tests.
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/map_renderer.hpp>
#include <finegui/drag_drop_manager.hpp>
#include <finegui/texture_registry.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/script_gui_manager.hpp>
#include <finegui/script_bindings.hpp>

#include <finevk/finevk.hpp>
#include <finescript/script_engine.h>
#include <finescript/map_data.h>

#include <iostream>
#include <cassert>

using namespace finegui;

// Helper: run N frames with both renderers
static void runFrames(finevk::Window* window, finevk::SimpleRenderer* renderer,
                      GuiSystem& gui, GuiRenderer& guiRenderer,
                      MapRenderer& mapRenderer,
                      ScriptGuiManager* mgr, int count) {
    for (int i = 0; i < count && window->isOpen(); i++) {
        window->pollEvents();
        if (auto frame = renderer->beginFrame()) {
            gui.beginFrame();
            if (mgr) mgr->processPendingMessages();
            guiRenderer.renderAll();
            mapRenderer.renderAll();
            gui.endFrame();

            frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
            gui.render(frame);
            frame.endRenderPass();
            renderer->endFrame();
        }
    }
}

void test_script_rendering() {
    std::cout << "Testing: Script-driven rendering (comprehensive)... ";

    // Create script engine first so it outlives Vulkan/GUI resources
    finescript::ScriptEngine engine;
    registerGuiBindings(engine);

    // Create Vulkan resources
    auto instance = finevk::Instance::create()
        .applicationName("test_script")
        .enableValidation(true)
        .build();
    auto window = finevk::Window::create(instance.get())
        .title("test_script")
        .size(800, 600)
        .build();
    auto physicalDevice = instance->selectPhysicalDevice(window.get());
    auto device = physicalDevice.createLogicalDevice()
        .surface(window->surface())
        .build();
    window->bindDevice(device.get());
    finevk::RendererConfig rendererConfig;
    auto renderer = finevk::SimpleRenderer::create(window.get(), rendererConfig);
    GuiConfig guiConfig;
    guiConfig.msaaSamples = renderer->msaaSamples();
    GuiSystem gui(renderer->device(), guiConfig);
    gui.initialize(renderer.get());

    // Create both renderers
    GuiRenderer guiRenderer(gui);
    MapRenderer mapRenderer(engine);

    ScriptGuiManager mgr(engine, mapRenderer);

    // --- Test 1: Basic ScriptGui ---
    std::cout << "\n  1. Basic ScriptGui... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Script Window" [
                {ui.text "Hello from script!"}
                {ui.button "OK"}
            ]}
        )", "test1");
        assert(ok);
        assert(scriptGui.isActive());
        assert(scriptGui.guiId() >= 0);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);
        scriptGui.close();
        assert(!scriptGui.isActive());
    }
    std::cout << "ok";

    // --- Test 2: ScriptGuiManager ---
    std::cout << "\n  2. ScriptGuiManager... ";
    {
        auto* sg = mgr.showFromSource(R"(
            ui.show {ui.window "Managed" [
                {ui.text "Managed window"}
            ]}
        )", "test2");
        assert(sg != nullptr);
        assert(sg->isActive());
        assert(mgr.activeCount() == 1);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, &mgr, 3);
        mgr.closeAll();
        assert(mgr.activeCount() == 0);
        mgr.cleanup();
    }
    std::cout << "ok";

    // --- Test 3: Multiple scripted GUIs ---
    std::cout << "\n  3. Multiple scripted GUIs... ";
    {
        auto* sg1 = mgr.showFromSource(R"(
            ui.show {ui.window "Win 1" [{ui.text "First"}]}
        )", "multi1");
        auto* sg2 = mgr.showFromSource(R"(
            ui.show {ui.window "Win 2" [{ui.text "Second"}]}
        )", "multi2");
        assert(sg1 != nullptr && sg2 != nullptr);
        assert(sg1->guiId() != sg2->guiId());
        assert(mgr.activeCount() == 2);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, &mgr, 3);
        mgr.closeAll();
        mgr.cleanup();
    }
    std::cout << "ok";

    // --- Test 4: Message delivery ---
    std::cout << "\n  4. Message delivery... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set received false
            set msg_data nil
            ui.show {ui.window "Msg Test" [{ui.text "Waiting..."}]}
            gui.on_message :test_msg fn [data] do
                set received true
                set msg_data data
            end
        )", "test4");
        assert(ok);

        // Deliver a message
        bool handled = scriptGui.deliverMessage(
            engine.intern("test_msg"),
            finescript::Value::string("hello"));
        assert(handled);

        // Verify script received it
        auto receivedVal = scriptGui.context()->get("received");
        assert(receivedVal.isBool() && receivedVal.asBool() == true);
        auto dataVal = scriptGui.context()->get("msg_data");
        assert(dataVal.isString() && dataVal.asString() == "hello");

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 2);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 5: Queued messages ---
    std::cout << "\n  5. Queued messages... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        scriptGui.loadAndRun(R"(
            set count 0
            ui.show {ui.window "Queue" [{ui.text "..."}]}
            gui.on_message :increment fn [data] do
                set count (count + 1)
            end
        )", "test5");

        // Queue messages (simulates cross-thread delivery)
        scriptGui.queueMessage(engine.intern("increment"), finescript::Value::nil());
        scriptGui.queueMessage(engine.intern("increment"), finescript::Value::nil());
        scriptGui.queueMessage(engine.intern("increment"), finescript::Value::nil());

        // Process on GUI thread
        scriptGui.processPendingMessages();

        auto countVal = scriptGui.context()->get("count");
        assert(countVal.isInt() && countVal.asInt() == 3);

        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 6: Script with all Phase 1 widgets ---
    std::cout << "\n  6. All Phase 1 widgets from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "All Widgets" [
                {ui.text "Static text"}
                {ui.button "Click me"}
                {ui.checkbox "Check" false}
                {ui.slider "Float" 0.0 1.0 0.5}
                {ui.slider_int "Int" 0 100 50}
                {ui.input "Text" "hello"}
                {ui.input_int "Num" 42}
                {ui.input_float "Dec" 3.14}
                {ui.combo "Drop" ["A" "B" "C"] 0}
                {ui.separator}
                {ui.group [{ui.text "Grouped"}]}
                {ui.columns 2 [{ui.text "Left"} {ui.text "Right"}]}
            ]}
        )", "test6");
        assert(ok);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 7: Script error handling ---
    std::cout << "\n  7. Script error handling... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun("this_is_invalid_syntax !!@#$", "test7");
        // Should fail gracefully
        assert(!ok || !scriptGui.isActive());
    }
    std::cout << "ok";

    // --- Test 8: Map-based direct mutation ---
    std::cout << "\n  8. Direct map mutation (map-IS-widget-data)... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set text_widget {ui.text "Initial"}
            set gui_id {ui.show {ui.window "Dynamic" [
                text_widget
                {ui.button "Update" fn [] do
                    set text_widget.text "Updated!"
                end}
            ]}}
        )", "test8");
        assert(ok);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);

        // Verify map tree exists and is accessible
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        assert(tree->isMap());

        // Verify children via map API
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 2);

        // Verify text content
        auto& child0 = children.asArray()[0];
        assert(child0.isMap());
        auto textVal = child0.asMap().get(mapRenderer.syms().text);
        assert(textVal.isString());
        // Text should still be "Initial" since we didn't click the button

        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 9: Callback with direct map mutation ---
    std::cout << "\n  9. Callback-driven map mutation... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set count 0
            set text_widget {ui.text "Count: 0"}
            set gui_id {ui.show {ui.window "Mutation" [
                text_widget
                {ui.button "Inc" fn [] do
                    set count (count + 1)
                    set text_widget.text ("Count: " + {to_str count})
                end}
                {ui.checkbox "Flag" false fn [v] do
                    # onChange still works
                end}
            ]}}
        )", "test9_mut");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify initial state via map API
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 3);

        // Check initial text
        auto& textChild = children.asArray()[0];
        assert(textChild.isMap());
        auto textVal = textChild.asMap().get(mapRenderer.syms().text);
        assert(textVal.isString() && textVal.asString() == "Count: 0");

        // Simulate button click via script callback
        auto& btnChild = children.asArray()[1];
        assert(btnChild.isMap());
        auto onClickVal = btnChild.asMap().get(mapRenderer.syms().on_click);
        assert(onClickVal.isCallable());

        // Invoke the callback
        engine.callFunction(onClickVal, {}, *scriptGui.context());

        // Verify text was mutated via shared_ptr semantics
        textVal = textChild.asMap().get(mapRenderer.syms().text);
        assert(textVal.isString() && textVal.asString() == "Count: 1");

        // Click again
        engine.callFunction(onClickVal, {}, *scriptGui.context());
        textVal = textChild.asMap().get(mapRenderer.syms().text);
        assert(textVal.isString() && textVal.asString() == "Count: 2");

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 10: ui.node navigation ---
    std::cout << "\n  10. ui.node map navigation... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set gui_id {ui.show {ui.window "Nav Test" [
                {ui.text "Child 0"}
                {ui.text "Child 1"}
                {ui.group [{ui.text "Nested"}]}
            ]}}

            # Navigate to child 0
            set child0 {ui.node gui_id 0}

            # Navigate to nested child via array path
            set nested {ui.node gui_id [2 0]}
        )", "test10_nav");
        assert(ok);

        // Verify child0 was retrieved
        auto child0 = scriptGui.context()->get("child0");
        assert(child0.isMap());
        assert(child0.asMap().get(mapRenderer.syms().text).asString() == "Child 0");

        // Verify nested navigation
        auto nested = scriptGui.context()->get("nested");
        assert(nested.isMap());
        assert(nested.asMap().get(mapRenderer.syms().text).asString() == "Nested");

        // Mutate via navigated reference and verify it's visible in the tree
        child0.asMap().set(mapRenderer.syms().text, finescript::Value::string("Modified!"));
        auto* tree = scriptGui.mapTree();
        auto children = tree->asMap().get(mapRenderer.syms().children);
        auto updatedText = children.asArray()[0].asMap().get(mapRenderer.syms().text);
        assert(updatedText.asString() == "Modified!");

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 2);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 11: Broadcast via manager ---
    std::cout << "\n  11. Broadcast messages... ";
    {
        auto* sg1 = mgr.showFromSource(R"(
            set got_it false
            ui.show {ui.window "BC1" [{ui.text "..."}]}
            gui.on_message :notify fn [d] do
                set got_it true
            end
        )", "bc1");
        auto* sg2 = mgr.showFromSource(R"(
            set got_it false
            ui.show {ui.window "BC2" [{ui.text "..."}]}
            gui.on_message :notify fn [d] do
                set got_it true
            end
        )", "bc2");

        mgr.broadcastMessage(engine.intern("notify"), finescript::Value::nil());

        assert(sg1->context()->get("got_it").asBool() == true);
        assert(sg2->context()->get("got_it").asBool() == true);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, &mgr, 2);
        mgr.closeAll();
        mgr.cleanup();
    }
    std::cout << "ok";

    // --- Test 12: Variable bindings ---
    std::cout << "\n  12. Variable bindings... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Bindings" [
                {ui.text player_name}
                {ui.text ("Gold: " + {to_str gold})}
            ]}
        )", "test12", {
            {"player_name", finescript::Value::string("Alice")},
            {"gold", finescript::Value::integer(100)},
        });
        assert(ok);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 13: ImGui writeback to map ---
    std::cout << "\n  13. ImGui writeback to map... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set slider_widget {ui.slider "Test" 0.0 1.0 0.5}
            set gui_id {ui.show {ui.window "Writeback" [
                slider_widget
            ]}}
        )", "test13");
        assert(ok);

        // Render a few frames - ImGui will read the slider value
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);

        // The slider value should still be readable from the map
        auto sliderWidget = scriptGui.context()->get("slider_widget");
        assert(sliderWidget.isMap());
        auto val = sliderWidget.asMap().get(mapRenderer.syms().value);
        assert(val.isNumeric());
        // Value should be 0.5 (no user interaction)
        assert(static_cast<float>(val.asNumber()) == 0.5f);

        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 14: Phase 3 widgets from script ---
    std::cout << "\n  14. Phase 3 widgets from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 3" [
                {ui.text "Before"}
                {ui.same_line}
                {ui.text "After"}
                {ui.spacing}
                {ui.text_colored [1.0 0.3 0.3 1.0] "Red text"}
                {ui.text_wrapped "This is a long text that wraps."}
                {ui.text_disabled "Grayed out text"}
                {ui.progress_bar 0.75}
                {ui.separator}
                {ui.collapsing_header "Details" [
                    {ui.text "Hidden content"}
                    {ui.text "More hidden content"}
                ]}
            ]}
        )", "test14");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 10);

        // Verify same_line type (index 1)
        auto& sameLine = children.asArray()[1];
        assert(sameLine.isMap());
        assert(sameLine.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_same_line);

        // Verify text_colored has color array (index 4)
        auto& textColored = children.asArray()[4];
        assert(textColored.isMap());
        auto color = textColored.asMap().get(mapRenderer.syms().color);
        assert(color.isArray());
        assert(color.asArray().size() == 4);

        // Verify progress_bar has value (index 7)
        auto& progressBar = children.asArray()[7];
        assert(progressBar.isMap());
        auto val = progressBar.asMap().get(mapRenderer.syms().value);
        assert(val.isNumeric());
        assert(val.asNumber() == 0.75);

        // Verify collapsing_header has children (index 9)
        auto& header = children.asArray()[9];
        assert(header.isMap());
        auto headerChildren = header.asMap().get(mapRenderer.syms().children);
        assert(headerChildren.isArray());
        assert(headerChildren.asArray().size() == 2);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 15: Phase 4 widgets from script ---
    std::cout << "\n  15. Phase 4 widgets from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 4 Test" [
                {ui.tab_bar "tabs" [
                    {ui.tab "First" [
                        {ui.text "Tab 1 content"}
                        {ui.tree_node "Root" [
                            {ui.tree_node "Leaf"}
                        ]}
                    ]}
                    {ui.tab "Second" [
                        {ui.text "Tab 2 content"}
                    ]}
                ]}
                {ui.child "##scroll" [
                    {ui.text "Scrollable"}
                ]}
                {ui.menu "Edit" [
                    {ui.menu_item "Undo"}
                    {ui.menu_item "Redo"}
                ]}
            ]}
        )", "test15");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 3);

        // Verify tab_bar (index 0)
        auto& tabBar = children.asArray()[0];
        assert(tabBar.isMap());
        assert(tabBar.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_tab_bar);
        auto tabBarChildren = tabBar.asMap().get(mapRenderer.syms().children);
        assert(tabBarChildren.isArray());
        assert(tabBarChildren.asArray().size() == 2);

        // Verify first tab has tree_node
        auto& firstTab = tabBarChildren.asArray()[0];
        assert(firstTab.isMap());
        assert(firstTab.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_tab);
        auto tabChildren = firstTab.asMap().get(mapRenderer.syms().children);
        assert(tabChildren.isArray());
        assert(tabChildren.asArray().size() == 2);
        assert(tabChildren.asArray()[1].asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_tree_node);

        // Verify child (index 1)
        auto& childWidget = children.asArray()[1];
        assert(childWidget.isMap());
        assert(childWidget.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_child);

        // Verify menu (index 2) has 2 menu_items
        auto& menu = children.asArray()[2];
        assert(menu.isMap());
        assert(menu.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_menu);
        auto menuChildren = menu.asMap().get(mapRenderer.syms().children);
        assert(menuChildren.isArray());
        assert(menuChildren.asArray().size() == 2);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 16: Phase 5 table widgets from script ---
    std::cout << "\n  16. Phase 5 table widgets from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 5 Test" [
                {ui.table "stats" 2 [
                    {ui.table_row [{ui.text "HP"} {ui.text "100"}]}
                    {ui.table_row [{ui.text "MP"} {ui.text "50"}]}
                ]}
                {ui.table "grid" 3 [
                    {ui.table_next_column}
                    {ui.text "A"}
                    {ui.table_next_column}
                    {ui.text "B"}
                    {ui.table_next_column}
                    {ui.text "C"}
                ]}
            ]}
        )", "test16");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 2);

        // Verify first table
        auto& table1 = children.asArray()[0];
        assert(table1.isMap());
        assert(table1.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_table);
        assert(table1.asMap().get(mapRenderer.syms().num_columns).asInt() == 2);
        auto t1children = table1.asMap().get(mapRenderer.syms().children);
        assert(t1children.isArray());
        assert(t1children.asArray().size() == 2);
        // Each child should be a table_row
        assert(t1children.asArray()[0].asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_table_row);

        // Verify second table (imperative style)
        auto& table2 = children.asArray()[1];
        assert(table2.isMap());
        assert(table2.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_table);
        assert(table2.asMap().get(mapRenderer.syms().num_columns).asInt() == 3);
        auto t2children = table2.asMap().get(mapRenderer.syms().children);
        assert(t2children.isArray());
        assert(t2children.asArray().size() == 6);  // 3 next_column + 3 text
        assert(t2children.asArray()[0].asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_table_next_column);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 17: Phase 6 advanced input widgets from script ---
    std::cout << "\n  17. Phase 6 advanced input widgets from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 6 Test" [
                {ui.color_edit "Accent" [0.2 0.4 0.8 1.0]}
                {ui.color_picker "Background" [0.1 0.1 0.15 1.0]}
                {ui.drag_float "Speed" 1.5 0.1 0.0 10.0}
                {ui.drag_int "Count" 50 1.0 0 100}
            ]}
        )", "test17");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 4);

        // Verify color_edit
        auto& ce = children.asArray()[0];
        assert(ce.isMap());
        assert(ce.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_color_edit);

        // Verify drag_float
        auto& df = children.asArray()[2];
        assert(df.isMap());
        assert(df.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_drag_float);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 18: Phase 7 listbox, popup, modal from script ---
    std::cout << "\n  18. Phase 7 listbox, popup, modal from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set popup_widget {ui.popup "ctx_menu" [
                {ui.text "Cut"}
                {ui.text "Copy"}
                {ui.text "Paste"}
            ]}
            set modal_widget {ui.modal "Confirm Delete" [
                {ui.text "Are you sure?"}
                {ui.button "OK"}
            ]}
            ui.show {ui.window "Phase 7 Test" [
                {ui.listbox "Fruits" ["Apple" "Banana" "Cherry" "Date"] 1 4}
                {ui.separator}
                {ui.button "Show Popup" fn [] do
                    ui.open_popup popup_widget
                end}
                popup_widget
                {ui.separator}
                {ui.button "Show Modal" fn [] do
                    ui.open_popup modal_widget
                end}
                modal_widget
            ]}
        )", "test18");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());

        // Find the listbox
        auto& lb = children.asArray()[0];
        assert(lb.isMap());
        assert(lb.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_listbox);
        assert(lb.asMap().get(mapRenderer.syms().items).isArray());
        assert(lb.asMap().get(mapRenderer.syms().items).asArray().size() == 4);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 19: Phase 8 canvas and tooltip from script ---
    std::cout << "\n  19. Phase 8 canvas and tooltip from script... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 8 Test" [
                {ui.canvas "##drawing" 200 150 [
                    {ui.draw_line [10 10] [190 140] [1.0 0.0 0.0 1.0] 2.0}
                    {ui.draw_rect [20 20] [80 60] [0.0 1.0 0.0 1.0] true}
                    {ui.draw_circle [100 75] 30 [0.0 0.0 1.0 1.0] false 1.5}
                    {ui.draw_text [10 130] "Hello Canvas" [1.0 1.0 1.0 1.0]}
                    {ui.draw_triangle [150 20] [120 80] [180 80] [1.0 1.0 0.0 1.0] true}
                ]}
                {ui.tooltip "Drawing area - click to interact"}
                {ui.separator}
                {ui.button "Hover me"}
                {ui.tooltip [{ui.text "Rich tooltip"} {ui.text_colored [1.0 0.3 0.3 1.0] "Warning!"}]}
            ]}
        )", "test19");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree = scriptGui.mapTree();
        assert(tree != nullptr);
        auto children = tree->asMap().get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 5);

        // Verify canvas (index 0)
        auto& canvas = children.asArray()[0];
        assert(canvas.isMap());
        assert(canvas.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_canvas);
        assert(canvas.asMap().get(mapRenderer.syms().id).asString() == "##drawing");
        auto cmds = canvas.asMap().get(mapRenderer.syms().commands);
        assert(cmds.isArray());
        assert(cmds.asArray().size() == 5);

        // Verify draw_line command
        auto& line = cmds.asArray()[0];
        assert(line.isMap());
        assert(line.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_draw_line);
        assert(line.asMap().get(mapRenderer.syms().p1).isArray());
        assert(line.asMap().get(mapRenderer.syms().p2).isArray());

        // Verify draw_circle command
        auto& circle = cmds.asArray()[2];
        assert(circle.isMap());
        assert(circle.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_draw_circle);
        assert(circle.asMap().get(mapRenderer.syms().center).isArray());
        assert(circle.asMap().get(mapRenderer.syms().radius).asNumber() == 30.0);

        // Verify draw_triangle command
        auto& tri = cmds.asArray()[4];
        assert(tri.isMap());
        assert(tri.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_draw_triangle);

        // Verify text tooltip (index 1)
        auto& textTip = children.asArray()[1];
        assert(textTip.isMap());
        assert(textTip.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_tooltip);
        assert(textTip.asMap().get(mapRenderer.syms().text).asString()
               == "Drawing area - click to interact");

        // Verify rich tooltip (index 4)
        auto& richTip = children.asArray()[4];
        assert(richTip.isMap());
        assert(richTip.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_tooltip);
        auto tipChildren = richTip.asMap().get(mapRenderer.syms().children);
        assert(tipChildren.isArray());
        assert(tipChildren.asArray().size() == 2);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 20: Phase 9 script widgets rendering ---
    std::cout << "\n  20. Phase 9 script widgets... ";
    {
        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Phase 9 Script" [
                {ui.separator_text "Radio Group"}
                {ui.radio_button "Light" 0 0}
                {ui.radio_button "Dark" 0 1}
                {ui.separator_text "Selectables"}
                {ui.selectable "Item A" false}
                {ui.selectable "Item B" true}
                {ui.separator_text "Multiline"}
                {ui.input_multiline "Notes" "Line 1" 300 100}
                {ui.separator_text "Bullets"}
                {ui.indent 20}
                {ui.bullet_text "First point"}
                {ui.bullet_text "Second point"}
                {ui.unindent 20}
            ]}
        )", "test_phase9");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify the map tree was created correctly
        auto* root = scriptGui.mapTree();
        assert(root != nullptr);
        assert(root->isMap());
        auto& rootMap = root->asMap();
        auto children = rootMap.get(mapRenderer.syms().children);
        assert(children.isArray());
        assert(children.asArray().size() == 13);

        // Verify radio button map
        auto& rb = children.asArray()[1];
        assert(rb.isMap());
        assert(rb.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_radio_button);
        assert(rb.asMap().get(mapRenderer.syms().my_value).asInt() == 0);

        // Verify bullet text map
        auto& bt = children.asArray()[10];
        assert(bt.isMap());
        assert(bt.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_bullet_text);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 21: DnD script widgets rendering ---
    std::cout << "\n  21. DnD script widgets... ";
    {
        DragDropManager dndManager;
        mapRenderer.setDragDropManager(&dndManager);

        // Create DnD-enabled widget maps directly
        auto slot1 = finescript::Value::map();
        slot1.asMap().set(engine.intern("type"), finescript::Value::symbol(engine.intern("button")));
        slot1.asMap().set(engine.intern("label"), finescript::Value::string("Slot A"));
        slot1.asMap().set(engine.intern("id"), finescript::Value::string("dnd_slot_a"));
        slot1.asMap().set(engine.intern("drag_type"), finescript::Value::string("item"));
        slot1.asMap().set(engine.intern("drag_data"), finescript::Value::string("sword"));
        slot1.asMap().set(engine.intern("drop_accept"), finescript::Value::string("item"));

        auto slot2 = finescript::Value::map();
        slot2.asMap().set(engine.intern("type"), finescript::Value::symbol(engine.intern("button")));
        slot2.asMap().set(engine.intern("label"), finescript::Value::string("Slot B"));
        slot2.asMap().set(engine.intern("id"), finescript::Value::string("dnd_slot_b"));
        slot2.asMap().set(engine.intern("drag_type"), finescript::Value::string("item"));
        slot2.asMap().set(engine.intern("drag_data"), finescript::Value::string(""));
        slot2.asMap().set(engine.intern("drop_accept"), finescript::Value::string("item"));
        slot2.asMap().set(engine.intern("drag_mode"), finescript::Value::integer(2));

        auto win = finescript::Value::map();
        win.asMap().set(engine.intern("type"), finescript::Value::symbol(engine.intern("window")));
        win.asMap().set(engine.intern("title"), finescript::Value::string("DnD Script Test"));
        win.asMap().set(engine.intern("children"), finescript::Value::array({slot1, slot2}));

        finescript::ExecutionContext dndCtx(engine);
        int dndId = mapRenderer.show(win, dndCtx);

        // Verify map fields are readable
        auto* root = mapRenderer.get(dndId);
        assert(root != nullptr);
        auto& rootMap = root->asMap();
        auto childrenArr = rootMap.get(mapRenderer.syms().children);
        assert(childrenArr.isArray());
        assert(childrenArr.asArray().size() == 2);

        auto& s1 = childrenArr.asArray()[0];
        assert(s1.isMap());
        auto dragType = s1.asMap().get(mapRenderer.syms().drag_type);
        assert(dragType.isString());
        assert(std::string(dragType.asString()) == "item");

        auto& s2 = childrenArr.asArray()[1];
        auto dragMode = s2.asMap().get(mapRenderer.syms().drag_mode);
        assert(dragMode.isInt());
        assert(dragMode.asInt() == 2);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);

        mapRenderer.hide(dndId);
        mapRenderer.setDragDropManager(nullptr);
    }
    std::cout << "ok";

    // --- Test 22: Image widget from script (no texture registered = placeholder) ---
    std::cout << "\n  22. Image widget (placeholder)... ";
    {
        TextureRegistry texRegistry;
        mapRenderer.setTextureRegistry(&texRegistry);

        ScriptGui scriptGui(engine, mapRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Image Test" [
                {ui.image "sword_icon" 48 48}
                {ui.image "missing_tex"}
                {ui.text "Below images"}
            ]}
        )", "test22_img");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map tree structure
        auto* tree22 = scriptGui.mapTree();
        assert(tree22 != nullptr);
        auto children22 = tree22->asMap().get(mapRenderer.syms().children);
        assert(children22.isArray());
        assert(children22.asArray().size() == 3);

        // Verify image widget map
        auto& imgWidget = children22.asArray()[0];
        assert(imgWidget.isMap());
        assert(imgWidget.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_image);
        assert(imgWidget.asMap().get(mapRenderer.syms().texture).asString() == "sword_icon");
        assert(imgWidget.asMap().get(mapRenderer.syms().width).asNumber() == 48.0);
        assert(imgWidget.asMap().get(mapRenderer.syms().height).asNumber() == 48.0);

        // Render frames - should show placeholder text since no texture registered
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 3);

        // Now register a fake texture and render again
        TextureHandle fakeTex;
        fakeTex.id = 1;  // Non-zero = "valid" but won't be a real GPU texture
        fakeTex.width = 48;
        fakeTex.height = 48;
        // Don't register a real texture to keep test safe (no GPU texture needed)
        // Just verify the placeholder path works without crashes

        scriptGui.close();
        mapRenderer.setTextureRegistry(nullptr);
    }
    std::cout << "ok";

    // --- Test 23: Style push/pop via script ---
    {
        std::cout << "\n  23. Style push/pop... ";
        ScriptGui scriptGui(engine, mapRenderer);

        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Styled" [
                {ui.push_color :button [0.8 0.1 0.1 1.0]}
                {ui.push_var :frame_rounding 8.0}
                {ui.button "Red Round"}
                {ui.pop_var 1}
                {ui.pop_color 1}
                {ui.button "Normal"}
            ]}
        )", "test23_style");
        assert(ok);
        assert(scriptGui.isActive());

        // Verify map has correct type symbols
        auto* tree23 = scriptGui.mapTree();
        assert(tree23 != nullptr);
        auto children23 = tree23->asMap().get(mapRenderer.syms().children);
        assert(children23.isArray());
        assert(children23.asArray().size() == 6);

        // First child should be push_color
        auto& pushCol = children23.asArray()[0];
        assert(pushCol.isMap());
        assert(pushCol.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_push_color);

        // Second child should be push_var
        auto& pushVar = children23.asArray()[1];
        assert(pushVar.isMap());
        assert(pushVar.asMap().get(mapRenderer.syms().type).asSymbol()
               == mapRenderer.syms().sym_push_var);

        // Render frames - should not crash
        runFrames(window.get(), renderer.get(), gui, guiRenderer, mapRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    renderer->waitIdle();
    std::cout << "\nPASSED\n";

}

int main() {
    std::cout << "=== finegui Script Integration Rendering Tests ===\n\n";

    try {
        test_script_rendering();
        std::cout << "\n=== All script rendering tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
