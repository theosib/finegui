#pragma once

/**
 * @file scene_texture.hpp
 * @brief Offscreen render-to-texture helper for 3D-in-GUI rendering
 *
 * SceneTexture wraps a finevk OffscreenSurface and manages its
 * registration as a GuiSystem texture. This lets you render a 3D
 * scene (or any custom Vulkan drawing) offscreen and display the
 * result in an Image or Canvas widget.
 *
 * Usage:
 * @code
 * SceneTexture scene(gui, 512, 512);
 *
 * // Each frame:
 * scene.beginScene(0, 0, 0, 1);   // clear to black
 * auto& cmd = scene.commandBuffer();
 * // ... record Vulkan draw commands on cmd ...
 * scene.endScene();
 *
 * // Display via Image widget:
 * auto node = WidgetNode::image(scene.textureHandle(), 512, 512);
 *
 * // Or display via Canvas widget:
 * auto canvas = WidgetNode::canvas("viewport", 512, 512, scene.textureHandle());
 * @endcode
 */

#include "texture_handle.hpp"

#include <finevk/finevk.hpp>

#include <memory>

namespace finegui {

class GuiSystem;

/**
 * @brief Manages an offscreen render surface and its ImGui texture registration.
 *
 * Owns a finevk::OffscreenSurface, renders to it each frame, and keeps a
 * registered TextureHandle that can be displayed in Image/Canvas widgets.
 * Handles re-registration on resize.
 */
class SceneTexture {
public:
    /**
     * @brief Create a scene texture
     * @param gui The GuiSystem (for texture registration)
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param enableDepth Whether to create a depth buffer (needed for 3D rendering)
     */
    SceneTexture(GuiSystem& gui, uint32_t width, uint32_t height, bool enableDepth = true);

    ~SceneTexture();

    // Non-copyable, movable
    SceneTexture(const SceneTexture&) = delete;
    SceneTexture& operator=(const SceneTexture&) = delete;
    SceneTexture(SceneTexture&&) noexcept;
    SceneTexture& operator=(SceneTexture&&) noexcept;

    // ========================================================================
    // Render cycle
    // ========================================================================

    /**
     * @brief Begin rendering a scene
     *
     * Begins the offscreen frame and render pass with the given clear color.
     * After calling this, use commandBuffer() to record draw commands.
     *
     * @param r Clear color red (0-1)
     * @param g Clear color green (0-1)
     * @param b Clear color blue (0-1)
     * @param a Clear color alpha (0-1)
     */
    void beginScene(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    /**
     * @brief Get the command buffer for recording draw commands
     *
     * Only valid between beginScene() and endScene().
     */
    finevk::CommandBuffer& commandBuffer();

    /**
     * @brief Get the render target (for creating compatible pipelines)
     *
     * The render target is available at all times after construction.
     * Use this to create graphics pipelines that are compatible with
     * the offscreen render pass.
     */
    finevk::RenderTarget* renderTarget();

    /**
     * @brief Get the offscreen surface (for advanced usage)
     */
    finevk::OffscreenSurface* surface();

    /**
     * @brief End rendering and submit
     *
     * Submits the command buffer and waits for completion.
     * After this call, textureHandle() is valid for display.
     */
    void endScene();

    // ========================================================================
    // Display
    // ========================================================================

    /**
     * @brief Get the texture handle for display in Image/Canvas widgets
     *
     * Returns an invalid handle before the first endScene() call.
     */
    [[nodiscard]] TextureHandle textureHandle() const { return handle_; }

    /**
     * @brief Get the texture width
     */
    [[nodiscard]] uint32_t width() const { return width_; }

    /**
     * @brief Get the texture height
     */
    [[nodiscard]] uint32_t height() const { return height_; }

    // ========================================================================
    // Resize
    // ========================================================================

    /**
     * @brief Resize the offscreen surface
     *
     * Unregisters the old texture, resizes the surface, and re-registers.
     * The old textureHandle() becomes invalid after this call.
     * Call endScene() at least once after resize before using the handle.
     *
     * @param width New width in pixels
     * @param height New height in pixels
     */
    void resize(uint32_t width, uint32_t height);

private:
    void registerTexture();
    void unregisterTexture();

    GuiSystem* gui_ = nullptr;
    std::unique_ptr<finevk::OffscreenSurface> offscreen_;
    TextureHandle handle_{};
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool rendering_ = false;
};

} // namespace finegui
