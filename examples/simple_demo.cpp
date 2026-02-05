/**
 * @file simple_demo.cpp
 * @brief Simple finegui demonstration
 *
 * Shows basic usage of finegui with finevk SimpleRenderer.
 */

#include <finegui/finegui.hpp>

#include <finevk/finevk.hpp>
#include <finevk/engine/finevk_engine.hpp>

#include <iostream>

int main() {
    try {
        // Create renderer
        finevk::RendererConfig config;
        config.width = 1280;
        config.height = 720;
        config.enableValidation = true;
        config.framesInFlight = 2;
        config.vsync = true;

        auto renderer = finevk::SimpleRenderer::create(config);

        // Create input manager
        // Note: SimpleRenderer doesn't expose window directly, so for a real app
        // you'd use Window class. This demo keeps it simple.

        // Create GUI system
        finegui::GuiConfig guiConfig;
        guiConfig.msaaSamples = renderer->msaaSamples();

        finegui::GuiSystem gui(renderer->device(), guiConfig);
        gui.initialize(renderer.get());

        std::cout << "finegui demo started. Close window to exit.\n";

        // Demo state
        float sliderValue = 0.5f;
        bool checkboxValue = false;
        int counter = 0;
        float clearColor[3] = {0.1f, 0.1f, 0.15f};

        // Main loop
        while (!renderer->shouldClose()) {
            renderer->pollEvents();

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
                auto cmd = renderer->beginRenderPass(
                    {clearColor[0], clearColor[1], clearColor[2], 1.0f});

                gui.render(cmd);

                renderer->endRenderPass();
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
