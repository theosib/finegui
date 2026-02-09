/**
 * @file script_demo.cpp
 * @brief Demo combining retained-mode (C++) and script-driven GUI
 *
 * Shows a C++ retained-mode control panel alongside script-driven windows.
 * Demonstrates:
 *   - MapRenderer for script-defined UI (maps ARE the widget data)
 *   - ScriptGuiManager for managing multiple script GUIs
 *   - C++ retained-mode widgets via GuiRenderer
 *   - Message passing between C++ and scripts
 *   - Direct map mutation from script callbacks
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/map_renderer.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/script_gui_manager.hpp>
#include <finegui/script_bindings.hpp>

#include <finevk/finevk.hpp>
#include <finescript/script_engine.h>

#include <iostream>

// ---------------------------------------------------------------------------
// Script sources embedded as string literals
// ---------------------------------------------------------------------------

static const char* kSettingsScript = R"SCRIPT(
    set bg_r 0.10
    set bg_g 0.10
    set bg_b 0.15

    # Capture slider widgets so the renderer can write values back to them
    set r_slider {ui.slider "Red"   0.0 1.0 bg_r fn [v] do set bg_r v end}
    set g_slider {ui.slider "Green" 0.0 1.0 bg_g fn [v] do set bg_g v end}
    set b_slider {ui.slider "Blue"  0.0 1.0 bg_b fn [v] do set bg_b v end}

    ui.show {ui.window "Settings (Script)" [
        {ui.text "Background Color"}
        r_slider
        g_slider
        b_slider
        {ui.separator}
        {ui.input "Note" "Type here..." fn [v] do
            set note_text v
        end}
        {ui.separator}
        {ui.text "Combo selection:"}
        {ui.combo "Theme" ["Dark" "Light" "Solarized" "Nord"] 0 fn [v] do
            set selected_theme v
        end}
    ]}

    gui.on_message :get_bg fn [data] do
        # This handler is queried by C++ to read background color
    end
)SCRIPT";

static const char* kCounterScript = R"SCRIPT(
    set count 0
    set text_widget {ui.text "Count: 0"}
    set gui_id {ui.show {ui.window "Counter (Script)" [
        text_widget
        {ui.button "Increment" fn [] do
            set count (count + 1)
            set text_widget.text ("Count: " + {to_str count})
        end}
        {ui.button "Reset" fn [] do
            set count 0
            set text_widget.text "Count: 0"
        end}
    ]}}

    gui.on_message :reset fn [data] do
        set count 0
        set text_widget.text "Count: 0"
    end
)SCRIPT";

static const char* kWidgetShowcaseScript = R"SCRIPT(
    ui.show {ui.window "Widget Showcase (Script)" [
        {ui.text "Phase 1-2: Basic widgets"}
        {ui.separator}
        {ui.checkbox "Enable feature" false fn [v] do
            set feature_on v
        end}
        {ui.slider "Volume" 0.0 1.0 0.75}
        {ui.slider_int "Quality" 1 10 5}
        {ui.input_int "Port" 8080}
        {ui.input_float "Scale" 1.0}
        {ui.separator}
        {ui.columns 2 [
            {ui.text "Left column"}
            {ui.text "Right column"}
        ]}
        {ui.separator}
        {ui.group [
            {ui.text "Grouped widgets:"}
            {ui.slider "Alpha" 0.0 1.0 1.0}
        ]}

        {ui.separator}
        {ui.text "Phase 3: Layout & Display"}
        {ui.separator}
        {ui.text_colored [1.0 0.3 0.3 1.0] "Red colored text"}
        {ui.text_colored [0.3 1.0 0.3 1.0] "Green colored text"}
        {ui.text_colored [0.4 0.4 1.0 1.0] "Blue colored text"}
        {ui.text_wrapped "This is wrapped text from a script. It should wrap when the window is narrow enough."}
        {ui.text_disabled "This text is disabled/grayed out"}
        {ui.spacing}
        {ui.text "SameLine:"}
        {ui.button "X"} {ui.same_line} {ui.button "Y"} {ui.same_line} {ui.button "Z"}
        {ui.progress_bar 0.42}
        {ui.collapsing_header "Collapsible (script)" [
            {ui.text "Hidden content revealed!"}
            {ui.slider "Inner slider" 0.0 1.0 0.5}
        ]}

        {ui.separator}
        {ui.text "Phase 4: Containers & Menus"}
        {ui.separator}
        {ui.tab_bar "script_tabs" [
            {ui.tab "First" [
                {ui.text "First tab content"}
            ]}
            {ui.tab "Second" [
                {ui.text "Second tab content"}
                {ui.checkbox "Tab check" true}
            ]}
        ]}
        {ui.tree_node "Tree Root" [
            {ui.tree_node "Branch A" [
                {ui.text "Leaf content A"}
            ]}
            {ui.tree_node "Branch B" [
                {ui.text "Leaf content B"}
            ]}
        ]}
        {ui.child "scroll_area" [
            {ui.text "Scrollable child line 1"}
            {ui.text "Scrollable child line 2"}
            {ui.text "Scrollable child line 3"}
            {ui.text "Scrollable child line 4"}
            {ui.text "Scrollable child line 5"}
        ]}

        {ui.separator}
        {ui.text "Phase 5: Tables"}
        {ui.separator}
        {ui.table "script_table" 3 [
            {ui.table_row [
                {ui.text "Alice"}
                {ui.text "42"}
                {ui.text_colored [0.3 1.0 0.3 1.0] "Active"}
            ]}
            {ui.table_row [
                {ui.text "Bob"}
                {ui.text "27"}
                {ui.text_colored [1.0 1.0 0.3 1.0] "Idle"}
            ]}
            {ui.table_row [
                {ui.text "Charlie"}
                {ui.text "35"}
                {ui.text_colored [1.0 0.3 0.3 1.0] "Offline"}
            ]}
        ]}
    ]}
)SCRIPT";

static const char* kAdvancedInputScript = R"SCRIPT(
    ui.show {ui.window "Phase 6: Advanced Input (Script)" [
        {ui.text "Color editors:"}
        {ui.color_edit "Accent Color" [0.2 0.4 0.8 1.0]}
        {ui.color_edit "Highlight" [1.0 0.8 0.0 1.0]}
        {ui.separator}
        {ui.text "Color picker:"}
        {ui.color_picker "Background" [0.1 0.1 0.15 1.0]}
        {ui.separator}
        {ui.text "Drag inputs:"}
        {ui.drag_float "Speed" 1.5 0.1 0.0 10.0}
        {ui.drag_float "Scale" 1.0 0.01 0.0 0.0}
        {ui.drag_int "Count" 50 1.0 0 200}
        {ui.drag_int "Level" 1 0.5 1 99}
    ]}
)SCRIPT";

int main() {
    try {
        // Create script engine first (must outlive Vulkan resources)
        finescript::ScriptEngine engine;
        finegui::registerGuiBindings(engine);

        // Create Vulkan instance
        auto instance = finevk::Instance::create()
            .applicationName("script_demo")
            .enableValidation(true)
            .build();

        // Create window
        auto window = finevk::Window::create(instance.get())
            .title("finegui Script + Retained Demo")
            .size(1280, 720)
            .build();

        // Select physical device and create logical device
        auto physicalDevice = instance->selectPhysicalDevice(window.get());
        auto device = physicalDevice.createLogicalDevice()
            .surface(window->surface())
            .build();

        window->bindDevice(device.get());

        // Create renderer
        finevk::RendererConfig config;
        auto renderer = finevk::SimpleRenderer::create(window.get(), config);

        // Create input manager
        auto input = finevk::InputManager::create(window.get());

        // Create GUI system
        finegui::GuiConfig guiConfig;
        guiConfig.msaaSamples = renderer->msaaSamples();
        auto contentScale = window->contentScale();
        guiConfig.dpiScale = contentScale.x;
        guiConfig.fontSize = 16.0f;

        finegui::GuiSystem gui(renderer->device(), guiConfig);
        gui.initialize(renderer.get());

        // Create C++ retained-mode renderer
        finegui::GuiRenderer guiRenderer(gui);

        // Create map-based renderer for scripts
        finegui::MapRenderer mapRenderer(engine);

        // Create script GUI manager
        finegui::ScriptGuiManager mgr(engine, mapRenderer);

        // Launch script-driven windows
        auto* settingsGui = mgr.showFromSource(kSettingsScript, "settings");
        auto* counterGui = mgr.showFromSource(kCounterScript, "counter");
        auto* showcaseGui = mgr.showFromSource(kWidgetShowcaseScript, "showcase");
        auto* advInputGui = mgr.showFromSource(kAdvancedInputScript, "adv_input");

        auto checkScript = [](const char* name, finegui::ScriptGui* sg) {
            if (!sg) {
                std::cerr << "Failed to load script: " << name << "\n";
                return false;
            }
            return true;
        };
        bool allOk = checkScript("settings", settingsGui)
                    & checkScript("counter", counterGui)
                    & checkScript("showcase", showcaseGui)
                    & checkScript("adv_input", advInputGui);
        if (!allOk) return 1;

        // Build a C++ retained-mode control panel
        int controlId = guiRenderer.show(finegui::WidgetNode::window("Control Panel (C++)", {
            finegui::WidgetNode::text("This window is built in C++ retained mode."),
            finegui::WidgetNode::text("Script windows run alongside it."),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::button("Reset Counter", [&](finegui::WidgetNode&) {
                // Send a message to the counter script
                counterGui->deliverMessage(
                    engine.intern("reset"), finescript::Value::nil());
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Active script GUIs: 4"),
            finegui::WidgetNode::button("Close Showcase", [&](finegui::WidgetNode& btn) {
                if (showcaseGui && showcaseGui->isActive()) {
                    showcaseGui->close();
                    btn.label = "Showcase Closed";
                    btn.enabled = false;
                    auto* ctrl = guiRenderer.get(controlId);
                    if (ctrl && ctrl->children.size() > 5) {
                        int active = static_cast<int>(mgr.activeCount());
                        ctrl->children[5].textContent =
                            "Active script GUIs: " + std::to_string(active);
                    }
                }
            }),
        }));

        // Background color (read from settings script each frame)
        float bgR = 0.10f, bgG = 0.10f, bgB = 0.15f;

        std::cout << "Script + retained demo started. Close window to exit.\n";
        if (window->isHighDPI()) {
            std::cout << "High-DPI display detected (scale: "
                      << contentScale.x << "x)\n";
        }
        std::cout << "  - Settings window: script-driven color sliders\n";
        std::cout << "  - Counter window: script-driven with direct map mutation\n";
        std::cout << "  - Widget showcase: all widget types from script\n";
        std::cout << "  - Advanced input: Phase 6 color/drag widgets\n";
        std::cout << "  - Control panel: C++ retained-mode with cross-GUI messaging\n";

        // Main loop
        while (window->isOpen()) {
            window->pollEvents();

            // Process input events
            input->update();
            finevk::InputEvent event;
            while (input->pollEvent(event)) {
                gui.processInput(finegui::InputAdapter::fromFineVK(event));
            }

            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();

                // Process pending script messages
                mgr.processPendingMessages();

                // Read background color from settings script
                if (settingsGui->isActive()) {
                    auto* ctx = settingsGui->context();
                    auto r = ctx->get("bg_r");
                    auto g = ctx->get("bg_g");
                    auto b = ctx->get("bg_b");
                    if (r.isNumeric()) bgR = static_cast<float>(r.asFloat());
                    if (g.isNumeric()) bgG = static_cast<float>(g.asFloat());
                    if (b.isNumeric()) bgB = static_cast<float>(b.asFloat());
                }

                // Render all widget trees
                guiRenderer.renderAll();     // C++ WidgetNode trees
                mapRenderer.renderAll();     // Script map trees

                gui.endFrame();

                frame.beginRenderPass({bgR, bgG, bgB, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        mgr.closeAll();
        renderer->waitIdle();
        std::cout << "Script + retained demo finished.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
