# finevk Input Event System Design

## Overview

finevk needs a unified input event system that replaces the current key event handling with a richer, extensible model covering keyboard, mouse, and scroll events. The system uses a priority-ordered listener chain with a fat event struct that carries both persistent input state ("level") and per-event trigger information ("edge").

## Event Struct: Level + Edge

A single `InputEvent` struct serves all event types. It has two conceptual layers:

### Level State (Ground State)

The persistent snapshot of all input at the moment the event fires. This is maintained continuously and copied into each event as it's created.

- **Held keys**: `std::unordered_set<int>` of GLFW key codes currently pressed. Typically 2-5 entries, so iteration and copy are cheap. Updated on every key press/release before the event is dispatched.
- **Mouse buttons**: `uint8_t` bitfield (bits 0-7 for GLFW mouse buttons 1-8). Compact, trivially copyable.
- **Mouse position**: `double mouseX, mouseY` — absolute screen coordinates when cursor is visible, relative deltas when cursor is hidden (game camera mode).
- **Modifier flags**: `int mods` — GLFW modifier bitmask (shift, ctrl, alt, super). Redundant with keysHeld but convenient for quick checks.
- **Cursor mode**: `bool cursorVisible` — indicates whether mouse coordinates are absolute or relative.

### Edge State (What Triggered This Event)

Identifies the specific change that caused this event to fire.

- **Event type enum**:
  ```
  KeyPress, KeyRelease, KeyRepeat,
  MouseButtonPress, MouseButtonRelease,
  MouseMove,
  MouseScroll
  ```
- **Key fields** (for key events): `int key` (GLFW keycode), `int scancode`
- **Button field** (for mouse button events): `int button` (GLFW button index)
- **Scroll fields** (for scroll events): `double scrollX, scrollY` (raw platform values — fractional on macOS trackpad, integer-ish for discrete wheels)

### Event Creation Flow

On each GLFW callback:

1. Update the persistent ground state (add/remove key from set, update mouse position, set/clear button bit)
2. Copy the ground state into a stack-allocated `InputEvent`
3. Fill in the edge fields (event type, which key/button/scroll)
4. Walk the listener chain, passing the event by reference

The event lives on the stack for the duration of dispatch. No heap allocation, no refcounting.

## Scroll Wheel Handling

macOS trackpads generate many small fractional scroll events (0.1, 0.3, etc.) while traditional mouse wheels produce clean +1.0/-1.0 per notch. Both platforms and input devices must work correctly out of the box.

### Raw Values

`InputEvent::scrollX` and `scrollY` always contain the platform's native values, unmodified. Listeners that want smooth behavior (zoom, pan) use these directly.

### Discrete Scroll Accumulator

Provide a utility class (or a method on the ground state) that accumulates fractional scroll values and emits discrete ticks:

```
ScrollAccumulator:
  - accumulate(double delta)
  - int ticks()  // returns whole ticks consumed, resets fractional remainder
```

Listeners that want discrete steps (hotbar cycling, list scrolling) use the accumulator. This eliminates the need for a "discrete scrolling" user preference — both smooth and discrete behaviors are available simultaneously from the same raw data.

## Mouse Coordinate Modes

The GLFW wrapper manages cursor mode and generates the right coordinate semantics:

- **Cursor visible** (GUI mode): `mouseX/mouseY` are absolute screen coordinates. GLFW's `glfwSetInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL)`.
- **Cursor hidden** (game camera mode): `mouseX/mouseY` are deltas since last poll. GLFW's `glfwSetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED)`.

The wrapper tracks which mode it's in and fills the event struct accordingly. Listeners check `cursorVisible` to know which interpretation to use. A mode-switch API on the wrapper lets the game or GUI request cursor mode changes:

```
void setCursorMode(CursorMode mode);  // Normal, Hidden, Disabled
```

## Listener Chain

### Registration

Listeners register with a priority value. Lower priority numbers get the event first (higher precedence). Listeners are always connected — behavior changes come from internal mode/state, not from register/unregister churn.

```
using ListenerResult = enum { Reject, Used, Consumed };
using InputListener = std::function<ListenerResult(InputEvent& event)>;

int addListener(InputListener listener, int priority);
void removeListener(int listenerId);
```

### Dispatch

Events are dispatched to listeners in priority order. Each listener returns one of three values:

| Result | Meaning | Propagation |
|--------|---------|-------------|
| **Reject** | "Not for me, I don't want it" | Continue to next listener |
| **Used** | "I noted it, but others should see it too" | Continue to next listener |
| **Consumed** | "Mine, stop here" | Stop propagation |

- **Reject**: The listener has no interest. The GUI sees W in navigation mode and rejects it — the game's movement listener gets it next.
- **Used**: The listener observed the event but doesn't claim exclusivity. The GUI notes that shift is held (for shift-click modifier behavior), but the game also sees shift (for sprint). Both act on it.
- **Consumed**: The listener claims the event exclusively. A text field has focus and eats the W keystroke — the game never sees it, so the player doesn't move.

Dispatch stops on the first `Consumed` result. `Reject` and `Used` both allow propagation to continue, but `Used` signals that the event was meaningful to someone (useful for debugging/logging if needed).

### Priority Conventions

Suggested priority bands:

| Priority | Listener | Notes |
|----------|----------|-------|
| 100 | GUI (text input) | When a text field has focus, consumes all key events |
| 200 | GUI (menu/inventory) | When menus are open, consumes most keys except reject list |
| 300 | GUI (HUD) | Always active, consumes clicks on HUD widgets only |
| 500 | Game controls | WASD movement, mouse look, hotbar |
| 900 | Debug/fallback | Catch-all logging, unhandled key display |

The GUI registers once at startup with a single listener (or a few at different priorities for different capture scopes). Its internal mode determines how aggressively it consumes:

- **Navigation mode**: GUI at priority 300 only consumes mouse clicks on HUD elements and number keys for hotbar. Rejects WASD, shift, ESC, most keys.
- **Menu/inventory mode**: GUI at priority 200 consumes most keys and all mouse events (to prevent WASD movement and mouse look). Selectively rejects ESC so the game can handle closing the menu.
- **Text field focused**: GUI at priority 100 consumes all key events.

## Integration with ImGui

When the GUI listener receives events, it translates them for ImGui:

- Key press/release → `ImGuiIO::AddKeyEvent()`
- Mouse position → `ImGuiIO::AddMousePosEvent()`
- Mouse button → `ImGuiIO::AddMouseButtonEvent()`
- Scroll → `ImGuiIO::AddMouseWheelEvent()`
- Text input → `ImGuiIO::AddInputCharacter()` (from GLFW char callback, separate from key events)

After ImGui processes a frame, `io.WantCaptureKeyboard` and `io.WantCaptureMouse` inform the GUI listener's consume/reject decisions for the next frame. But these are hints, not the sole authority — the GUI's mode (navigation vs menu vs text) takes precedence.

## What finevk Needs to Implement

### New Files

1. **`include/finevk/input_event.h`** — `InputEvent` struct, `ListenerResult` enum, `ScrollAccumulator` utility
2. **`include/finevk/input_dispatcher.h`** — `InputDispatcher` class (listener registration, dispatch, ground state management)
3. **`src/input_event.cpp`** — `ScrollAccumulator` implementation
4. **`src/input_dispatcher.cpp`** — Dispatch logic, ground state updates, GLFW callback integration

### Modified Files

1. **Window class** — Install GLFW callbacks that feed into `InputDispatcher` instead of (or in addition to) the current key event system. Add `setCursorMode()` API. Expose `InputDispatcher&` for listener registration.
2. **Existing key event struct** — Can be deprecated gradually. The new `InputEvent` subsumes it.

### API Surface

```cpp
// On the window or a dedicated input manager:
InputDispatcher& inputDispatcher();
void setCursorMode(CursorMode mode);

// Listener registration:
int InputDispatcher::addListener(InputListener fn, int priority);
void InputDispatcher::removeListener(int id);

// Ground state queries (for polling between events):
const InputState& InputDispatcher::state() const;
bool InputState::isKeyHeld(int glfwKey) const;
bool InputState::isMouseButtonHeld(int button) const;
double InputState::mouseX() const;
double InputState::mouseY() const;
```

### Backward Compatibility

The existing finevk key event mechanism continues to work during migration. The new system can coexist — GLFW callbacks can feed both systems until consumers are ported. Once all consumers use `InputDispatcher`, the old system can be removed.

## Focus Management

Focus determines which widget receives keyboard input and is the primary driver of the GUI listener's consume/reject behavior. finegui needs explicit focus tracking beyond what ImGui provides natively.

### Focus State

The GUI maintains a focus state that answers: "does any widget currently want keyboard input, and if so, which one?"

- **No focus**: No widget is focused. GUI rejects most keys (navigation mode behavior).
- **Widget focus**: A non-text widget (button, slider, combo) is focused. GUI consumes Tab (for navigation) and Enter/Space (for activation), rejects letter keys.
- **Text focus**: A text input field is focused. GUI consumes all key events.

ImGui's `io.WantCaptureKeyboard` partially reflects this, but finegui's focus manager is authoritative because it also tracks application-level focus intent (e.g., "inventory is open but nothing is focused yet — still consume WASD to prevent movement").

### Tab Order

Tab key cycles focus between focusable widgets within the active GUI panel. Shift+Tab cycles in reverse.

**How tab order is determined:**

1. **Implicit order**: By default, widgets are focusable in the order they appear in the widget tree (depth-first traversal of children). This matches ImGui's internal navigation order.
2. **Explicit tab index**: Widgets can specify a `tab_index` field to override ordering. Lower indices come first. Widgets without an explicit index use their tree position relative to explicitly-indexed neighbors.
3. **Skip**: A widget can set `tab_index = -1` (or `focusable = false`) to be excluded from the tab order entirely.

**Tab behavior:**
- Tab on the last focusable widget wraps to the first (within the current window/panel).
- Tab is always consumed by the GUI when a panel with focusable widgets is active — it never reaches the game layer.
- Tab across windows is not supported (each window has its own tab ring).

### Programmatic Focus

The game or scripts can set focus to a specific widget:

```
// C++ API
guiSystem.setFocus("inventory_search_field");

// Script API
gui.set_focus "inventory_search_field"
```

**Use cases:**
- Auto-focus a search/filter field when inventory opens
- Focus a confirmation button when a dialog appears
- Return focus to a specific widget after a sub-dialog closes
- Focus the chat input when the player presses Enter

**Implementation:**
- `setFocus(widgetId)` queues a focus request that takes effect on the next frame (before ImGui processes input). This avoids mid-frame state confusion.
- Uses ImGui's `SetKeyboardFocusHere()` internally, called at the right point during widget rendering when the target widget ID matches.
- For MapRenderer trees, the widget's `:id` field serves as the focus target identifier.
- For retained-mode WidgetNode trees, `WidgetNode::id` serves the same purpose.

### Focus and the Listener Chain

Focus state feeds back into the input listener's consume/reject logic:

| Focus State | GUI Listener Behavior |
|-------------|----------------------|
| No focus, nav mode | Reject most keys, consume only HUD clicks + hotbar numbers |
| No focus, menu mode | Consume most keys (block WASD), reject ESC |
| Widget focus (non-text) | Consume Tab, Enter, Space, arrow keys. Reject letters. |
| Text focus | Consume all key events |

The GUI listener checks focus state at the top of its handler to decide its strategy for the current event. This is a per-event check, not a mode change — focus can shift between events within the same frame (e.g., Tab press moves focus from a button to a text field, and the very next keystroke gets consumed differently).

### Widget-Level Focus Fields

New fields for WidgetNode and map widgets:

```
tab_index: int       // Explicit position in tab order (-1 to skip)
focusable: bool      // Whether this widget participates in tab navigation (default: auto-detected from type)
auto_focus: bool     // Focus this widget when its parent window/panel first appears
```

Text inputs, combos, and sliders are focusable by default. Text, separator, spacing, and other display-only widgets are not. Buttons are focusable (Enter/Space activates them).

### Focus Events

When focus changes, the system can fire callbacks:

```
on_focus: callback     // Called when this widget gains focus
on_blur: callback      // Called when this widget loses focus
```

These are useful for scripts that need to react to focus changes — e.g., showing a help tooltip when a field is focused, or validating input when a field loses focus.
