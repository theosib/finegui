#pragma once

/**
 * @file texture_handle.hpp
 * @brief Opaque handle to textures for use in GUI
 */

#include <imgui.h>
#include <cstdint>

namespace finegui {

/**
 * @brief Opaque handle to a texture that can be displayed in GUI
 *
 * Used for 3D-in-GUI features like inventory item previews.
 * The handle references a texture registered with GuiSystem.
 */
struct TextureHandle {
    uint64_t id = 0;           ///< Unique identifier
    uint32_t width = 0;        ///< Texture width in pixels
    uint32_t height = 0;       ///< Texture height in pixels

    /// Check if handle is valid
    [[nodiscard]] bool valid() const { return id != 0; }

    /// Implicit conversion to ImTextureID for ImGui::Image()
    operator ImTextureID() const { return reinterpret_cast<ImTextureID>(id); }

    bool operator==(const TextureHandle& other) const { return id == other.id; }
    bool operator!=(const TextureHandle& other) const { return id != other.id; }
};

} // namespace finegui
