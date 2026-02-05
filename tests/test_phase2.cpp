/**
 * @file test_phase2.cpp
 * @brief Phase 2 tests - Rendering with finevk backend
 *
 * Tests:
 * - GuiSystem initialization with SimpleRenderer
 * - Basic frame rendering
 * - Input processing
 */

#include <finegui/finegui.hpp>

#include <finevk/finevk.hpp>

#include <iostream>
#include <cassert>

using namespace finegui;

// ============================================================================
// Rendering Tests (require Vulkan)
// ============================================================================

void test_gui_system_creation() {
    std::cout << "Testing: GuiSystem creation with SimpleRenderer... ";

    // Create a simple renderer
    finevk::RendererConfig config;
    config.width = 800;
    config.height = 600;
    config.enableValidation = true;
    config.framesInFlight = 2;

    auto renderer = finevk::SimpleRenderer::create(config);
    assert(renderer != nullptr);

    // Create GuiSystem
    GuiConfig guiConfig;
    guiConfig.msaaSamples = renderer->msaaSamples();

    GuiSystem gui(renderer->device(), guiConfig);
    assert(!gui.isInitialized());

    // Initialize
    gui.initialize(renderer.get());
    assert(gui.isInitialized());

    std::cout << "PASSED\n";
}

void test_basic_frame() {
    std::cout << "Testing: Basic frame rendering... ";

    // Create renderer and GUI
    finevk::RendererConfig config;
    config.width = 800;
    config.height = 600;
    config.enableValidation = true;

    auto renderer = finevk::SimpleRenderer::create(config);

    GuiConfig guiConfig;
    guiConfig.msaaSamples = renderer->msaaSamples();

    GuiSystem gui(renderer->device(), guiConfig);
    gui.initialize(renderer.get());

    // Run a few frames
    for (int i = 0; i < 3 && !renderer->shouldClose(); i++) {
        renderer->pollEvents();

        if (auto frame = renderer->beginFrame()) {
            gui.beginFrame();  // Auto mode - gets frame index and delta time

            // Simple ImGui content
            ImGui::Begin("Test Window");
            ImGui::Text("Hello from finegui!");
            ImGui::End();

            gui.endFrame();

            auto cmd = renderer->beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
            gui.render(cmd);
            renderer->endRenderPass();
            renderer->endFrame();
        }
    }

    renderer->waitIdle();
    std::cout << "PASSED\n";
}

void test_input_processing() {
    std::cout << "Testing: Input processing... ";

    // Create renderer and GUI
    finevk::RendererConfig config;
    config.width = 800;
    config.height = 600;
    config.enableValidation = true;

    auto renderer = finevk::SimpleRenderer::create(config);

    GuiConfig guiConfig;
    GuiSystem gui(renderer->device(), guiConfig);
    gui.initialize(renderer.get());

    // Simulate input events
    InputEvent mouseMove{};
    mouseMove.type = InputEventType::MouseMove;
    mouseMove.mouseX = 400.0f;
    mouseMove.mouseY = 300.0f;
    gui.processInput(mouseMove);

    InputEvent keyPress{};
    keyPress.type = InputEventType::Key;
    keyPress.keyCode = ImGuiKey_A;
    keyPress.keyPressed = true;
    gui.processInput(keyPress);

    InputEvent mouseClick{};
    mouseClick.type = InputEventType::MouseButton;
    mouseClick.button = 0;  // Left button
    mouseClick.pressed = true;
    gui.processInput(mouseClick);

    // Verify capture state queries work
    gui.beginFrame();  // Auto mode
    bool wantMouse = gui.wantCaptureMouse();
    bool wantKeyboard = gui.wantCaptureKeyboard();
    gui.endFrame();

    // Just verify these don't crash - actual values depend on ImGui state
    (void)wantMouse;
    (void)wantKeyboard;

    renderer->waitIdle();
    std::cout << "PASSED\n";
}

void test_state_updates() {
    std::cout << "Testing: State update handling... ";

    // Create renderer and GUI
    finevk::RendererConfig config;
    config.width = 800;
    config.height = 600;
    config.enableValidation = true;

    auto renderer = finevk::SimpleRenderer::create(config);

    GuiConfig guiConfig;
    GuiSystem gui(renderer->device(), guiConfig);
    gui.initialize(renderer.get());

    // Define a test state update
    struct TestState : TypedStateUpdate<TestState> {
        int value = 0;
    };

    // Register handler
    int receivedValue = -1;
    gui.onStateUpdate<TestState>([&receivedValue](const TestState& state) {
        receivedValue = state.value;
    });

    // Apply state update
    TestState state;
    state.value = 42;
    gui.applyState(state);

    assert(receivedValue == 42);

    renderer->waitIdle();
    std::cout << "PASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== finegui Phase 2 Tests ===\n\n";

    try {
        test_gui_system_creation();
        test_basic_frame();
        test_input_processing();
        test_state_updates();

        std::cout << "\n=== All Phase 2 tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
