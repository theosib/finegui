#pragma once

#include <imgui.h>
#include <functional>
#include <string>
#include <vector>

namespace finegui {

using HotkeyCallback = std::function<void()>;

/// Manages keyboard shortcut bindings.
///
/// Each frame, `update()` checks all registered key chords using ImGui's
/// `Shortcut()` API and fires matching callbacks. Supports modifier keys
/// (Ctrl, Shift, Alt, Super) and configurable routing flags.
///
/// Usage:
///   HotkeyManager hotkeys;
///   hotkeys.bind(ImGuiMod_Ctrl | ImGuiKey_S, []() { save(); });
///   hotkeys.bind(ImGuiKey_F5, []() { refresh(); });
///   // Each frame (between beginFrame/endFrame):
///   hotkeys.update();
class HotkeyManager {
public:
    HotkeyManager() = default;

    /// Check all bindings and fire matching callbacks. Call once per frame.
    void update();

    /// Bind a key chord to a callback. Returns a binding ID.
    int bind(ImGuiKeyChord chord, HotkeyCallback callback,
             ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal);

    /// Unbind by ID.
    void unbind(int id);

    /// Unbind all bindings for a specific chord.
    void unbindChord(ImGuiKeyChord chord);

    /// Unbind everything.
    void unbindAll();

    /// Enable/disable a specific binding.
    void setEnabled(int id, bool enabled);
    bool isEnabled(int id) const;

    /// Enable/disable all bindings globally.
    void setGlobalEnabled(bool enabled);
    bool isGlobalEnabled() const;

    /// Query binding count.
    int bindingCount() const;

    /// Parse a string like "ctrl+s", "shift+f5", "escape" into ImGuiKeyChord.
    /// Case-insensitive. Returns 0 on parse failure.
    static ImGuiKeyChord parseChord(const std::string& str);

    /// Format an ImGuiKeyChord as a human-readable string (e.g., "Ctrl+S").
    static std::string formatChord(ImGuiKeyChord chord);

private:
    struct Binding {
        int id;
        ImGuiKeyChord chord;
        ImGuiInputFlags flags;
        HotkeyCallback callback;
        bool enabled = true;
    };

    int nextId_ = 1;
    bool globalEnabled_ = true;
    std::vector<Binding> bindings_;
};

} // namespace finegui
