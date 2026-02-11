#pragma once

/**
 * @file input_adapter.hpp
 * @brief Abstracted input layer for finegui
 *
 * Defines finegui's own input event types, decoupled from GLFW and finevk.
 * This enables:
 * - Threading: GUI thread doesn't touch GLFW
 * - Flexibility: Input can come from any source (GLFW, network replay, testing)
 * - Isolation: No dependency on window system in GUI logic
 */

#include <finevk/engine/input_manager.hpp>

#include <cstdint>
#include <vector>

namespace finegui {

// ============================================================================
// Input event types
// ============================================================================

/**
 * @brief Types of input events
 */
enum class InputEventType {
    MouseMove,       ///< Mouse position changed
    MouseButton,     ///< Mouse button pressed/released
    MouseScroll,     ///< Mouse wheel scrolled
    Key,             ///< Keyboard key pressed/released
    Char,            ///< Character input (text entry)
    Focus,           ///< Window focus changed
    WindowResize     ///< Window size changed
};

/**
 * @brief Abstracted input event - independent of GLFW/finevk
 *
 * Contains all input information needed by ImGui in a portable format.
 * Key codes use ImGuiKey_* values for direct ImGui compatibility.
 */
struct InputEvent {
    InputEventType type;

    // ========================================================================
    // Mouse data
    // ========================================================================

    float mouseX = 0.0f;       ///< Mouse X position in pixels
    float mouseY = 0.0f;       ///< Mouse Y position in pixels
    float scrollX = 0.0f;      ///< Horizontal scroll delta
    float scrollY = 0.0f;      ///< Vertical scroll delta
    int button = 0;            ///< Mouse button (0=left, 1=right, 2=middle)
    bool pressed = false;      ///< Button/key pressed (true) or released (false)

    // ========================================================================
    // Keyboard data
    // ========================================================================

    int keyCode = 0;           ///< ImGui key code (ImGuiKey_*)
    bool keyPressed = false;   ///< Key pressed state
    uint32_t character = 0;    ///< Unicode codepoint for Char events

    // ========================================================================
    // Modifiers (always valid)
    // ========================================================================

    bool ctrl = false;         ///< Ctrl key held
    bool shift = false;        ///< Shift key held
    bool alt = false;          ///< Alt key held
    bool super = false;        ///< Super/Cmd/Win key held

    // ========================================================================
    // Window state
    // ========================================================================

    int windowWidth = 0;       ///< Window width in pixels
    int windowHeight = 0;      ///< Window height in pixels
    bool focused = true;       ///< Window has focus

    // ========================================================================
    // Timing
    // ========================================================================

    double time = 0.0;         ///< Timestamp (for replay/networking)
};

// ============================================================================
// Input adapter
// ============================================================================

/**
 * @brief Adapter to convert finevk input events to finegui events
 *
 * Handles the translation from GLFW key codes (used by finevk) to ImGui
 * key codes (used by finegui internally).
 */
class InputAdapter {
public:
    /**
     * @brief Convert a single finevk InputEvent to finegui InputEvent
     * @param event The finevk input event
     * @return Converted finegui input event
     */
    static InputEvent fromFineVK(const finevk::InputEvent& event);

    /**
     * @brief Convert GLFW key code to ImGui key code
     * @param glfwKey GLFW key constant (GLFW_KEY_*)
     * @return ImGui key code (ImGuiKey_*)
     */
    static int glfwKeyToImGui(int glfwKey);

    /**
     * @brief Convert GLFW mouse button to ImGui mouse button
     * @param glfwButton GLFW mouse button (GLFW_MOUSE_BUTTON_*)
     * @return ImGui mouse button index (0-4)
     */
    static int glfwMouseButtonToImGui(int glfwButton);
};

} // namespace finegui
