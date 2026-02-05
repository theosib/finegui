#pragma once

/**
 * @file gui_draw_data.hpp
 * @brief Serializable draw data for threaded rendering mode
 */

#include "texture_handle.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace finegui {

/**
 * @brief Serializable draw command
 */
struct DrawCommand {
    uint32_t indexOffset;      ///< Offset into index buffer
    uint32_t indexCount;       ///< Number of indices to draw
    uint32_t vertexOffset;     ///< Offset to add to vertex indices
    TextureHandle texture;     ///< Texture to bind
    glm::ivec4 scissorRect;    ///< Scissor rect (x, y, width, height)
};

/**
 * @brief Complete frame's draw data for threaded rendering
 *
 * Can be queued between threads for deferred rendering.
 */
struct GuiDrawData {
    std::vector<ImDrawVert> vertices;   ///< All vertex data
    std::vector<ImDrawIdx> indices;     ///< All index data
    std::vector<DrawCommand> commands;  ///< Draw commands
    glm::vec2 displaySize;              ///< Display size in pixels
    glm::vec2 framebufferScale;         ///< Framebuffer scale factor

    /// Check if there's anything to draw
    [[nodiscard]] bool empty() const { return commands.empty(); }

    /// Clear all data
    void clear() {
        vertices.clear();
        indices.clear();
        commands.clear();
        displaySize = glm::vec2(0.0f);
        framebufferScale = glm::vec2(1.0f);
    }
};

} // namespace finegui
