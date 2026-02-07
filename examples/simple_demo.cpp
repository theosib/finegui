/**
 * @file simple_demo.cpp
 * @brief Simple finegui demonstration
 *
 * Shows basic usage of finegui with finevk SimpleRenderer.
 */

#include <finegui/finegui.hpp>

#include <finevk/finevk.hpp>

#include <iostream>

int main() {
    try {
        // Create Vulkan instance
        auto instance = finevk::Instance::create()
            .applicationName("finegui demo")
            .enableValidation(true)
            .build();

        // Create window
        auto window = finevk::Window::create(instance.get())
            .title("finegui Demo")
            .size(1280, 720)
            .build();

        // Select physical device and create logical device
        auto physicalDevice = instance->selectPhysicalDevice(window.get());
        auto device = physicalDevice.createLogicalDevice()
            .surface(window->surface())
            .build();

        // Bind device to window
        window->bindDevice(device.get());

        // Create renderer
        finevk::RendererConfig config;
        auto renderer = finevk::SimpleRenderer::create(window.get(), config);

        // Create input manager for handling user input
        auto input = finevk::InputManager::create(window.get());

        // Create GUI system with high-DPI support
        finegui::GuiConfig guiConfig;
        guiConfig.msaaSamples = renderer->msaaSamples();
        // Scale font for high-DPI displays
        auto contentScale = window->contentScale();
        guiConfig.dpiScale = contentScale.x;
        guiConfig.fontSize = 16.0f;  // Will be scaled by dpiScale

        finegui::GuiSystem gui(renderer->device(), guiConfig);
        gui.initialize(renderer.get());

        std::cout << "finegui demo started. Close window to exit.\n";
        if (window->isHighDPI()) {
            std::cout << "High-DPI display detected (scale: " << contentScale.x << "x)\n";
        }

        // Demo state
        float sliderValue = 0.5f;
        bool checkboxValue = false;
        int counter = 0;
        float clearColor[3] = {0.1f, 0.1f, 0.15f};

        // Main loop
        while (window->isOpen()) {
            window->pollEvents();

            // Process input events and forward to GUI
            input->update();
            finevk::InputEvent event;
            while (input->pollEvent(event)) {
                gui.processInput(finegui::InputAdapter::fromFineVK(event));
            }

            if (auto frame = renderer->beginFrame()) {
                // beginFrame() auto-gets delta time and frame index from renderer
                gui.beginFrame();

                // Demo window
                ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);

                ImGui::Begin("finegui Demo", nullptr,
                             ImGuiWindowFlags_NoCollapse);

                ImGui::Text("Welcome to finegui!");
                ImGui::Separator();

                ImGui::SliderFloat("Slider", &sliderValue, 0.0f, 1.0f);
                ImGui::Checkbox("Checkbox", &checkboxValue);

                if (ImGui::Button("Click me!")) {
                    counter++;
                }
                ImGui::SameLine();
                ImGui::Text("Count: %d", counter);

                ImGui::ColorEdit3("Clear Color", clearColor);

                ImGui::Separator();
                ImGui::Text("Mouse captured: %s", gui.wantCaptureMouse() ? "yes" : "no");
                ImGui::Text("Keyboard captured: %s", gui.wantCaptureKeyboard() ? "yes" : "no");

                ImGui::End();

                // Show ImGui demo window
                ImGui::ShowDemoWindow();

                gui.endFrame();

                // Render
                frame.beginRenderPass(
                    {clearColor[0], clearColor[1], clearColor[2], 1.0f});

                gui.render(frame);

                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        renderer->waitIdle();
        std::cout << "finegui demo finished.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
