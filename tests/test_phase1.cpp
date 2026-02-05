/**
 * @file test_phase1.cpp
 * @brief Phase 1 tests - Input adapter and basic GuiSystem
 *
 * Tests:
 * - GLFW to ImGui key code conversion
 * - InputEvent creation and conversion
 * - GuiSystem construction (without rendering)
 */

#include <finegui/finegui.hpp>

#include <finevk/finevk.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <cassert>

using namespace finegui;

// ============================================================================
// Input Adapter Tests
// ============================================================================

void test_glfw_key_to_imgui() {
    std::cout << "Testing: GLFW key to ImGui conversion... ";

    // Test common keys
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_A) == ImGuiKey_A);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_Z) == ImGuiKey_Z);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_0) == ImGuiKey_0);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_9) == ImGuiKey_9);

    // Test special keys
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_ESCAPE) == ImGuiKey_Escape);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_ENTER) == ImGuiKey_Enter);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_TAB) == ImGuiKey_Tab);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_BACKSPACE) == ImGuiKey_Backspace);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_SPACE) == ImGuiKey_Space);

    // Test arrow keys
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_LEFT) == ImGuiKey_LeftArrow);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_RIGHT) == ImGuiKey_RightArrow);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_UP) == ImGuiKey_UpArrow);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_DOWN) == ImGuiKey_DownArrow);

    // Test modifier keys
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_LEFT_SHIFT) == ImGuiKey_LeftShift);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_LEFT_CONTROL) == ImGuiKey_LeftCtrl);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_LEFT_ALT) == ImGuiKey_LeftAlt);

    // Test function keys
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_F1) == ImGuiKey_F1);
    assert(InputAdapter::glfwKeyToImGui(GLFW_KEY_F12) == ImGuiKey_F12);

    // Test unknown key
    assert(InputAdapter::glfwKeyToImGui(-1) == ImGuiKey_None);

    std::cout << "PASSED\n";
}

void test_mouse_button_conversion() {
    std::cout << "Testing: Mouse button conversion... ";

    assert(InputAdapter::glfwMouseButtonToImGui(GLFW_MOUSE_BUTTON_LEFT) == 0);
    assert(InputAdapter::glfwMouseButtonToImGui(GLFW_MOUSE_BUTTON_RIGHT) == 1);
    assert(InputAdapter::glfwMouseButtonToImGui(GLFW_MOUSE_BUTTON_MIDDLE) == 2);

    std::cout << "PASSED\n";
}

void test_input_event_creation() {
    std::cout << "Testing: InputEvent creation... ";

    InputEvent event{};
    event.type = InputEventType::MouseMove;
    event.mouseX = 100.0f;
    event.mouseY = 200.0f;
    event.ctrl = true;

    assert(event.type == InputEventType::MouseMove);
    assert(event.mouseX == 100.0f);
    assert(event.mouseY == 200.0f);
    assert(event.ctrl == true);
    assert(event.shift == false);

    std::cout << "PASSED\n";
}

// ============================================================================
// State Update Tests
// ============================================================================

struct TestHealthUpdate : TypedStateUpdate<TestHealthUpdate> {
    float current = 0.0f;
    float max = 100.0f;
};

struct TestInventoryUpdate : TypedStateUpdate<TestInventoryUpdate> {
    int itemCount = 0;
};

void test_state_update_type_ids() {
    std::cout << "Testing: State update type IDs... ";

    // Each type should get a unique ID
    uint32_t healthId = TestHealthUpdate::staticTypeId();
    uint32_t inventoryId = TestInventoryUpdate::staticTypeId();

    assert(healthId != inventoryId);

    // Same type should return same ID
    assert(TestHealthUpdate::staticTypeId() == healthId);
    assert(TestInventoryUpdate::staticTypeId() == inventoryId);

    // Instance should match static
    TestHealthUpdate health;
    assert(health.typeId() == healthId);

    std::cout << "PASSED\n";
}

// ============================================================================
// Texture Handle Tests
// ============================================================================

void test_texture_handle() {
    std::cout << "Testing: TextureHandle... ";

    TextureHandle invalid;
    assert(!invalid.valid());
    assert(invalid.id == 0);

    TextureHandle valid;
    valid.id = 42;
    valid.width = 256;
    valid.height = 256;
    assert(valid.valid());
    assert(valid.id == 42);

    // Test ImTextureID conversion
    ImTextureID texId = valid;
    assert(reinterpret_cast<uint64_t>(texId) == 42);

    std::cout << "PASSED\n";
}

// ============================================================================
// Draw Data Tests
// ============================================================================

void test_draw_data() {
    std::cout << "Testing: GuiDrawData... ";

    GuiDrawData data;
    assert(data.empty());

    data.commands.push_back(DrawCommand{});
    assert(!data.empty());

    data.clear();
    assert(data.empty());

    std::cout << "PASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== finegui Phase 1 Tests ===\n\n";

    // Initialize GLFW (needed for key constants to be valid)
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    try {
        test_glfw_key_to_imgui();
        test_mouse_button_conversion();
        test_input_event_creation();
        test_state_update_type_ids();
        test_texture_handle();
        test_draw_data();

        std::cout << "\n=== All Phase 1 tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}
