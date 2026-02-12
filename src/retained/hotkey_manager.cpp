#include <finegui/hotkey_manager.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace finegui {

// -- update -------------------------------------------------------------------

void HotkeyManager::update() {
    if (!globalEnabled_) return;

    // Iterate by index — callbacks may call unbind() which modifies bindings_
    for (size_t i = 0; i < bindings_.size(); i++) {
        auto& b = bindings_[i];
        if (!b.enabled) continue;
        if (ImGui::Shortcut(b.chord, b.flags)) {
            b.callback();
            // After callback, bindings_ may have changed — don't cache size
        }
    }
}

// -- bind / unbind ------------------------------------------------------------

int HotkeyManager::bind(ImGuiKeyChord chord, HotkeyCallback callback,
                         ImGuiInputFlags flags) {
    int id = nextId_++;
    bindings_.push_back({id, chord, flags, std::move(callback), true});
    return id;
}

void HotkeyManager::unbind(int id) {
    bindings_.erase(
        std::remove_if(bindings_.begin(), bindings_.end(),
                        [id](const Binding& b) { return b.id == id; }),
        bindings_.end());
}

void HotkeyManager::unbindChord(ImGuiKeyChord chord) {
    bindings_.erase(
        std::remove_if(bindings_.begin(), bindings_.end(),
                        [chord](const Binding& b) { return b.chord == chord; }),
        bindings_.end());
}

void HotkeyManager::unbindAll() {
    bindings_.clear();
}

// -- enable / disable ---------------------------------------------------------

void HotkeyManager::setEnabled(int id, bool enabled) {
    for (auto& b : bindings_) {
        if (b.id == id) {
            b.enabled = enabled;
            return;
        }
    }
}

bool HotkeyManager::isEnabled(int id) const {
    for (const auto& b : bindings_) {
        if (b.id == id) return b.enabled;
    }
    return false;
}

void HotkeyManager::setGlobalEnabled(bool enabled) {
    globalEnabled_ = enabled;
}

bool HotkeyManager::isGlobalEnabled() const {
    return globalEnabled_;
}

int HotkeyManager::bindingCount() const {
    return static_cast<int>(bindings_.size());
}

// -- parseChord ---------------------------------------------------------------

static std::string toLower(const std::string& s) {
    std::string result = s;
    for (auto& c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

ImGuiKeyChord HotkeyManager::parseChord(const std::string& str) {
    if (str.empty()) return 0;

    std::string lower = toLower(str);
    ImGuiKeyChord chord = 0;
    bool hasKey = false;

    // Split on '+'
    std::istringstream ss(lower);
    std::string token;
    while (std::getline(ss, token, '+')) {
        // Trim whitespace
        while (!token.empty() && token.front() == ' ') token.erase(token.begin());
        while (!token.empty() && token.back() == ' ') token.pop_back();
        if (token.empty()) continue;

        // Modifiers
        if (token == "ctrl") { chord |= ImGuiMod_Ctrl; continue; }
        if (token == "shift") { chord |= ImGuiMod_Shift; continue; }
        if (token == "alt") { chord |= ImGuiMod_Alt; continue; }
        if (token == "super" || token == "cmd") { chord |= ImGuiMod_Super; continue; }

        // Single letter a-z
        if (token.size() == 1 && token[0] >= 'a' && token[0] <= 'z') {
            chord |= (ImGuiKey_A + (token[0] - 'a'));
            hasKey = true;
            continue;
        }

        // Single digit 0-9
        if (token.size() == 1 && token[0] >= '0' && token[0] <= '9') {
            chord |= (ImGuiKey_0 + (token[0] - '0'));
            hasKey = true;
            continue;
        }

        // Function keys f1-f24
        if (token[0] == 'f' && token.size() >= 2 && token.size() <= 3) {
            int num = std::atoi(token.c_str() + 1);
            if (num >= 1 && num <= 24) {
                chord |= (ImGuiKey_F1 + (num - 1));
                hasKey = true;
                continue;
            }
        }

        // Named keys
        if (token == "escape" || token == "esc") { chord |= ImGuiKey_Escape; hasKey = true; continue; }
        if (token == "enter" || token == "return") { chord |= ImGuiKey_Enter; hasKey = true; continue; }
        if (token == "space") { chord |= ImGuiKey_Space; hasKey = true; continue; }
        if (token == "tab") { chord |= ImGuiKey_Tab; hasKey = true; continue; }
        if (token == "backspace") { chord |= ImGuiKey_Backspace; hasKey = true; continue; }
        if (token == "delete" || token == "del") { chord |= ImGuiKey_Delete; hasKey = true; continue; }
        if (token == "insert" || token == "ins") { chord |= ImGuiKey_Insert; hasKey = true; continue; }
        if (token == "up") { chord |= ImGuiKey_UpArrow; hasKey = true; continue; }
        if (token == "down") { chord |= ImGuiKey_DownArrow; hasKey = true; continue; }
        if (token == "left") { chord |= ImGuiKey_LeftArrow; hasKey = true; continue; }
        if (token == "right") { chord |= ImGuiKey_RightArrow; hasKey = true; continue; }
        if (token == "home") { chord |= ImGuiKey_Home; hasKey = true; continue; }
        if (token == "end") { chord |= ImGuiKey_End; hasKey = true; continue; }
        if (token == "pageup") { chord |= ImGuiKey_PageUp; hasKey = true; continue; }
        if (token == "pagedown") { chord |= ImGuiKey_PageDown; hasKey = true; continue; }
        if (token == "minus") { chord |= ImGuiKey_Minus; hasKey = true; continue; }
        if (token == "equals" || token == "equal") { chord |= ImGuiKey_Equal; hasKey = true; continue; }

        // Unknown token — parse failure
        return 0;
    }

    // Must have at least one non-modifier key
    if (!hasKey) return 0;

    return chord;
}

// -- formatChord --------------------------------------------------------------

std::string HotkeyManager::formatChord(ImGuiKeyChord chord) {
    std::string result;

    // Modifiers first
    if (chord & ImGuiMod_Ctrl) result += "Ctrl+";
    if (chord & ImGuiMod_Shift) result += "Shift+";
    if (chord & ImGuiMod_Alt) result += "Alt+";
    if (chord & ImGuiMod_Super) result += "Super+";

    // Extract the key (strip modifiers)
    ImGuiKey key = static_cast<ImGuiKey>(chord & ~ImGuiMod_Mask_);

    if (key >= ImGuiKey_A && key <= ImGuiKey_Z) {
        result += static_cast<char>('A' + (key - ImGuiKey_A));
    } else if (key >= ImGuiKey_0 && key <= ImGuiKey_9) {
        result += static_cast<char>('0' + (key - ImGuiKey_0));
    } else if (key >= ImGuiKey_F1 && key <= ImGuiKey_F24) {
        result += "F" + std::to_string(1 + (key - ImGuiKey_F1));
    } else {
        // Named keys
        switch (key) {
            case ImGuiKey_Escape:     result += "Escape"; break;
            case ImGuiKey_Enter:      result += "Enter"; break;
            case ImGuiKey_Space:      result += "Space"; break;
            case ImGuiKey_Tab:        result += "Tab"; break;
            case ImGuiKey_Backspace:  result += "Backspace"; break;
            case ImGuiKey_Delete:     result += "Delete"; break;
            case ImGuiKey_Insert:     result += "Insert"; break;
            case ImGuiKey_UpArrow:    result += "Up"; break;
            case ImGuiKey_DownArrow:  result += "Down"; break;
            case ImGuiKey_LeftArrow:  result += "Left"; break;
            case ImGuiKey_RightArrow: result += "Right"; break;
            case ImGuiKey_Home:       result += "Home"; break;
            case ImGuiKey_End:        result += "End"; break;
            case ImGuiKey_PageUp:     result += "PageUp"; break;
            case ImGuiKey_PageDown:   result += "PageDown"; break;
            case ImGuiKey_Minus:      result += "Minus"; break;
            case ImGuiKey_Equal:      result += "Equal"; break;
            default:
                result += "Unknown";
                break;
        }
    }

    return result;
}

} // namespace finegui
