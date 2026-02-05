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

// Helper to create renderer with window
struct TestContext {
    finevk::InstancePtr instance;
    finevk::WindowPtr window;
    finevk::LogicalDevicePtr device;
    std::unique_ptr<finevk::SimpleRenderer> renderer;

    static TestContext create(const char* title = "finegui test") {
        TestContext ctx;

        // Create Vulkan instance
        ctx.instance = finevk::Instance::create()
            .applicationName(title)
            .enableValidation(true)
            .build();

        // Create window
        ctx.window = finevk::Window::create(ctx.instance.get())
            .title(title)
            .size(800, 600)
            .build();

        // Select physical device and create logical device
        auto physicalDevice = ctx.instance->selectPhysicalDevice(ctx.window.get());
        ctx.device = physicalDevice.createLogicalDevice()
            .surface(ctx.window->surface())
            .build();

        // Bind device to window
        ctx.window->bindDevice(ctx.device.get());

        // Create renderer
        finevk::RendererConfig config;
        ctx.renderer = finevk::SimpleRenderer::create(ctx.window.get(), config);

        return ctx;
    }
};

// ============================================================================
// Rendering Tests (require Vulkan)
// ============================================================================

void test_gui_system_creation() {
    std::cout << "Testing: GuiSystem creation with SimpleRenderer... ";

    auto ctx = TestContext::create("test_gui_system_creation");

    // Create GuiSystem
    GuiConfig guiConfig;
    guiConfig.msaaSamples = ctx.renderer->msaaSamples();

    GuiSystem gui(ctx.renderer->device(), guiConfig);
    assert(!gui.isInitialized());

    // Initialize
    gui.initialize(ctx.renderer.get());
    assert(gui.isInitialized());

    std::cout << "PASSED\n";
}

void test_basic_frame() {
    std::cout << "Testing: Basic frame rendering... ";

    auto ctx = TestContext::create("test_basic_frame");

    GuiConfig guiConfig;
    guiConfig.msaaSamples = ctx.renderer->msaaSamples();

    GuiSystem gui(ctx.renderer->device(), guiConfig);
    gui.initialize(ctx.renderer.get());

    // Run a few frames
    for (int i = 0; i < 3 && ctx.window->isOpen(); i++) {
        ctx.window->pollEvents();

        if (auto frame = ctx.renderer->beginFrame()) {
            gui.beginFrame();  // Auto mode - gets frame index and delta time

            // Simple ImGui content
            ImGui::Begin("Test Window");
            ImGui::Text("Hello from finegui!");
            ImGui::End();

            gui.endFrame();

            ctx.renderer->beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
            gui.render(frame);
            ctx.renderer->endRenderPass();
            ctx.renderer->endFrame();
        }
    }

    ctx.renderer->waitIdle();
    std::cout << "PASSED\n";
}

void test_input_processing() {
    std::cout << "Testing: Input processing... ";

    auto ctx = TestContext::create("test_input_processing");

    GuiConfig guiConfig;
    GuiSystem gui(ctx.renderer->device(), guiConfig);
    gui.initialize(ctx.renderer.get());

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

    ctx.renderer->waitIdle();
    std::cout << "PASSED\n";
}

void test_state_updates() {
    std::cout << "Testing: State update handling... ";

    auto ctx = TestContext::create("test_state_updates");

    GuiConfig guiConfig;
    GuiSystem gui(ctx.renderer->device(), guiConfig);
    gui.initialize(ctx.renderer.get());

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

    ctx.renderer->waitIdle();
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
