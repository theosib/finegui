/**
 * @file retained_demo.cpp
 * @brief Visual demo of finegui retained-mode widget system
 *
 * Shows the same kind of UI as simple_demo, but built entirely
 * through the retained-mode WidgetNode / GuiRenderer API.
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>

#include <finevk/finevk.hpp>

#include <iostream>

int main() {
    try {
        // Create Vulkan instance
        auto instance = finevk::Instance::create()
            .applicationName("retained_demo")
            .enableValidation(true)
            .build();

        // Create window
        auto window = finevk::Window::create(instance.get())
            .title("finegui Retained-Mode Demo")
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

        // Create GUI system with high-DPI support
        finegui::GuiConfig guiConfig;
        guiConfig.msaaSamples = renderer->msaaSamples();
        auto contentScale = window->contentScale();
        guiConfig.dpiScale = contentScale.x;
        guiConfig.fontSize = 16.0f;

        finegui::GuiSystem gui(renderer->device(), guiConfig);
        gui.initialize(renderer.get());

        // Create retained-mode renderer
        finegui::GuiRenderer guiRenderer(gui);

        // -- Build the widget trees --

        // Demo state
        int counter = 0;

        // Main demo window
        int mainId = guiRenderer.show(finegui::WidgetNode::window("Retained-Mode Demo", {
            finegui::WidgetNode::text("Welcome to finegui retained mode!"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::slider("Float Slider", 0.5f, 0.0f, 1.0f),
            finegui::WidgetNode::sliderInt("Int Slider", 50, 0, 100),
            finegui::WidgetNode::checkbox("Checkbox", false),
            finegui::WidgetNode::button("Click me!", [&counter](finegui::WidgetNode&) {
                counter++;
            }),
            finegui::WidgetNode::text("Count: 0"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::inputText("Name", "World"),
            finegui::WidgetNode::inputInt("Integer", 42),
            finegui::WidgetNode::inputFloat("Float", 3.14f),
            finegui::WidgetNode::combo("Dropdown", {"Option A", "Option B", "Option C"}, 0),
        }));

        // A second window showing columns
        guiRenderer.show(finegui::WidgetNode::window("Layout Demo", {
            finegui::WidgetNode::text("Two-column layout:"),
            finegui::WidgetNode::columns(2, {
                finegui::WidgetNode::text("Left side"),
                finegui::WidgetNode::text("Right side"),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Nested groups:"),
            finegui::WidgetNode::group({
                finegui::WidgetNode::slider("Nested Slider A", 0.3f, 0.0f, 1.0f),
                finegui::WidgetNode::slider("Nested Slider B", 0.7f, 0.0f, 1.0f),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::button("Toggle Disabled", [&guiRenderer, mainId](finegui::WidgetNode& btn) {
                auto* main = guiRenderer.get(mainId);
                if (main && main->children.size() > 2) {
                    // Toggle enabled state on the float slider in the main window
                    auto& slider = main->children[2];
                    slider.enabled = !slider.enabled;
                    btn.label = slider.enabled ? "Toggle Disabled" : "Toggle Enabled";
                }
            }),
        }));

        std::cout << "Retained-mode demo started. Close window to exit.\n";
        if (window->isHighDPI()) {
            std::cout << "High-DPI display detected (scale: " << contentScale.x << "x)\n";
        }

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

                // Update counter text by mutating the tree directly
                auto* main = guiRenderer.get(mainId);
                if (main && main->children.size() > 6) {
                    main->children[6].textContent = "Count: " + std::to_string(counter);
                }

                // Render all retained-mode widget trees
                guiRenderer.renderAll();

                gui.endFrame();

                frame.beginRenderPass({0.1f, 0.1f, 0.15f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        renderer->waitIdle();
        std::cout << "Retained-mode demo finished.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
