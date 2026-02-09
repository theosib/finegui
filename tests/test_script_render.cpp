/**
 * @file test_script_render.cpp
 * @brief Integration tests for script-driven GUI rendering (requires Vulkan)
 *
 * Resources are created inline to avoid finevk smart pointer move issues.
 * Uses a single Vulkan context for all tests.
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/script_gui_manager.hpp>
#include <finegui/script_bindings.hpp>

#include <finevk/finevk.hpp>
#include <finescript/script_engine.h>

#include <iostream>
#include <cassert>

using namespace finegui;

// Helper: run N frames
static void runFrames(finevk::Window* window, finevk::SimpleRenderer* renderer,
                      GuiSystem& gui, GuiRenderer& guiRenderer,
                      ScriptGuiManager* mgr, int count) {
    for (int i = 0; i < count && window->isOpen(); i++) {
        window->pollEvents();
        if (auto frame = renderer->beginFrame()) {
            gui.beginFrame();
            if (mgr) mgr->processPendingMessages();
            guiRenderer.renderAll();
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

    GuiRenderer guiRenderer(gui);

    ScriptGuiManager mgr(engine, guiRenderer);

    // --- Test 1: Basic ScriptGui ---
    std::cout << "\n  1. Basic ScriptGui... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Script Window" [
                {ui.text "Hello from script!"}
                {ui.button "OK"}
            ]}
        )", "test1");
        assert(ok);
        assert(scriptGui.isActive());
        assert(scriptGui.guiId() >= 0);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, nullptr, 3);
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
        runFrames(window.get(), renderer.get(), gui, guiRenderer, &mgr, 3);
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
        runFrames(window.get(), renderer.get(), gui, guiRenderer, &mgr, 3);
        mgr.closeAll();
        mgr.cleanup();
    }
    std::cout << "ok";

    // --- Test 4: Message delivery ---
    std::cout << "\n  4. Message delivery... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
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

        runFrames(window.get(), renderer.get(), gui, guiRenderer, nullptr, 2);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 5: Queued messages ---
    std::cout << "\n  5. Queued messages... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
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
        ScriptGui scriptGui(engine, guiRenderer);
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
        runFrames(window.get(), renderer.get(), gui, guiRenderer, nullptr, 5);
        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 7: Script error handling ---
    std::cout << "\n  7. Script error handling... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
        bool ok = scriptGui.loadAndRun("this_is_invalid_syntax !!@#$", "test7");
        // Should fail gracefully
        assert(!ok || !scriptGui.isActive());
    }
    std::cout << "ok";

    // --- Test 8: Callback with ui.update ---
    std::cout << "\n  8. Script callback with ui.update... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            set gui_id {ui.show {ui.window "Dynamic" [
                {ui.text "Initial"}
                {ui.button "Update" fn [] do
                    ui.update gui_id {ui.window "Dynamic" [
                        {ui.text "Updated!"}
                    ]}
                end}
            ]}}
        )", "test8");
        assert(ok);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, nullptr, 3);

        // Verify tree exists
        auto* tree = scriptGui.widgetTree();
        assert(tree != nullptr);
        assert(tree->children.size() == 2);

        scriptGui.close();
    }
    std::cout << "ok";

    // --- Test 9: Broadcast via manager ---
    std::cout << "\n  9. Broadcast messages... ";
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

        runFrames(window.get(), renderer.get(), gui, guiRenderer, &mgr, 2);
        mgr.closeAll();
        mgr.cleanup();
    }
    std::cout << "ok";

    // --- Test 10: Variable bindings ---
    std::cout << "\n  10. Variable bindings... ";
    {
        ScriptGui scriptGui(engine, guiRenderer);
        bool ok = scriptGui.loadAndRun(R"(
            ui.show {ui.window "Bindings" [
                {ui.text player_name}
                {ui.text "Gold: {gold}"}
            ]}
        )", "test10", {
            {"player_name", finescript::Value::string("Alice")},
            {"gold", finescript::Value::integer(100)},
        });
        assert(ok);
        runFrames(window.get(), renderer.get(), gui, guiRenderer, nullptr, 3);
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
