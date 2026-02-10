#pragma once

/**
 * @file gui_system.hpp
 * @brief Main interface for finegui
 */

#include "gui_config.hpp"
#include "gui_state.hpp"
#include "gui_draw_data.hpp"
#include "input_adapter.hpp"
#include "texture_handle.hpp"

#include <finevk/finevk.hpp>

#include <imgui.h>

#include <memory>
#include <functional>
#include <unordered_map>

namespace finegui {

/**
 * @brief Main GUI system - wraps Dear ImGui with finevk backend
 *
 * Provides a clean interface for creating interactive GUI in finevk applications.
 * Supports both same-thread and threaded rendering modes.
 *
 * Usage (same-thread mode):
 * @code
 * auto gui = std::make_unique<GuiSystem>(device.get(), config);
 * gui->initialize(renderer.get());
 *
 * // Game loop
 * gui->beginFrame(deltaTime);
 * ImGui::Begin("Window");
 * ImGui::Text("Hello!");
 * ImGui::End();
 * gui->endFrame();
 *
 * // Inside render pass
 * gui->render(cmd);
 * @endcode
 */
class GuiSystem {
public:
    // ========================================================================
    // Lifecycle
    // ========================================================================

    /**
     * @brief Construct GuiSystem
     * @param device The finevk logical device
     * @param config Configuration options
     */
    explicit GuiSystem(finevk::LogicalDevice* device, const GuiConfig& config = {});

    /// Destructor
    ~GuiSystem();

    // Non-copyable
    GuiSystem(const GuiSystem&) = delete;
    GuiSystem& operator=(const GuiSystem&) = delete;

    // Movable
    GuiSystem(GuiSystem&&) noexcept;
    GuiSystem& operator=(GuiSystem&&) noexcept;

    // ========================================================================
    // Setup
    // ========================================================================

    /**
     * @brief Initialize with render pass info
     * @param renderPass The render pass to use for GUI rendering
     * @param commandPool Command pool for resource creation
     * @param subpass Subpass index (default 0)
     */
    void initialize(finevk::RenderPass* renderPass,
                    finevk::CommandPool* commandPool,
                    uint32_t subpass = 0);

    /// Convenience overload for references
    void initialize(finevk::RenderPass& renderPass,
                    finevk::CommandPool& commandPool,
                    uint32_t subpass = 0) {
        initialize(&renderPass, &commandPool, subpass);
    }

    /**
     * @brief Initialize from a RenderSurface (SimpleRenderer, OffscreenSurface, etc.)
     * @param surface The render surface (extracts render pass, command pool, etc.)
     * @param subpass Subpass index (default 0)
     */
    void initialize(finevk::RenderSurface* surface, uint32_t subpass = 0);

    /// Convenience overload for references
    void initialize(finevk::RenderSurface& surface, uint32_t subpass = 0) {
        initialize(&surface, subpass);
    }

    /// Backward-compatible overload: SimpleRenderer IS a RenderSurface
    void initialize(finevk::SimpleRenderer* renderer, uint32_t subpass = 0) {
        initialize(static_cast<finevk::RenderSurface*>(renderer), subpass);
    }

    /// Convenience overload for references
    void initialize(finevk::SimpleRenderer& renderer, uint32_t subpass = 0) {
        initialize(&renderer, subpass);
    }

    /**
     * @brief Register a texture for use in GUI
     * @param texture The finevk texture
     * @param sampler Optional sampler (uses default if null)
     * @return Handle for use with ImGui::Image()
     */
    TextureHandle registerTexture(finevk::Texture* texture,
                                  finevk::Sampler* sampler = nullptr);

    /**
     * @brief Register an image view for use in GUI (e.g. offscreen render result)
     * @param imageView The image view to sample from
     * @param sampler The sampler to use (uses default if null)
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @return Handle for use with ImGui::Image()
     */
    TextureHandle registerTexture(finevk::ImageView* imageView,
                                  finevk::Sampler* sampler,
                                  uint32_t width, uint32_t height);

    /**
     * @brief Unregister a texture
     * @param handle The texture handle to unregister
     */
    void unregisterTexture(TextureHandle handle);

    // ========================================================================
    // Per-Frame: Input
    // ========================================================================

    /**
     * @brief Process an input event
     * @param event The finegui input event
     */
    void processInput(const InputEvent& event);

    /**
     * @brief Process multiple input events
     * @param events Container of input events
     */
    template<typename Container>
    void processInputBatch(const Container& events) {
        for (const auto& e : events) {
            processInput(e);
        }
    }

    // ========================================================================
    // Per-Frame: State Updates
    // ========================================================================

    /**
     * @brief Apply a state update
     * @param update The state update to apply
     */
    template<typename T>
    void applyState(const T& update) {
        auto it = stateHandlers_.find(T::staticTypeId());
        if (it != stateHandlers_.end()) {
            it->second(update);
        }
    }

    /**
     * @brief Register handler for a state update type
     * @param handler Function to call when state update is applied
     */
    template<typename T>
    void onStateUpdate(std::function<void(const T&)> handler) {
        stateHandlers_[T::staticTypeId()] = [handler](const GuiStateUpdate& update) {
            handler(static_cast<const T&>(update));
        };
    }

    // ========================================================================
    // Per-Frame: Rendering
    // ========================================================================

    /**
     * @brief Begin a new frame (automatic mode)
     *
     * Uses internal delta time tracking. Requires initialize(SimpleRenderer*).
     * Call this before any ImGui widgets.
     */
    void beginFrame();

    /**
     * @brief Begin a new frame with explicit delta time
     * @param deltaTime Time since last frame in seconds
     *
     * Gets frame index automatically from renderer if available.
     * Call this before any ImGui widgets.
     */
    void beginFrame(float deltaTime);

    /**
     * @brief Begin a new frame (manual mode)
     * @param frameIndex Current frame-in-flight index
     * @param deltaTime Time since last frame in seconds
     *
     * Use this when not initialized with SimpleRenderer.
     * Call this before any ImGui widgets.
     */
    void beginFrame(uint32_t frameIndex, float deltaTime);

    /**
     * @brief End frame and finalize draw data
     *
     * Call this after all ImGui widgets.
     */
    void endFrame();

    /**
     * @brief Render to command buffer (automatic mode)
     * @param cmd The command buffer to record into
     *
     * Gets frame index automatically from renderer.
     * Must be called within an active render pass.
     * Requires initialize(SimpleRenderer*).
     */
    void render(finevk::CommandBuffer& cmd);

    /// Convenience overload for pointer
    void render(finevk::CommandBuffer* cmd) { render(*cmd); }

    /**
     * @brief Render to command buffer (manual mode)
     * @param cmd The command buffer to record into
     * @param frameIndex Current frame-in-flight index
     *
     * Use this when not initialized with SimpleRenderer.
     * Must be called within an active render pass.
     */
    void render(finevk::CommandBuffer& cmd, uint32_t frameIndex);

    /**
     * @brief Get draw data for external rendering (threaded mode)
     * @return Reference to the captured draw data
     *
     * Only valid between endFrame() and next beginFrame().
     * Requires enableDrawDataCapture=true in config.
     */
    const GuiDrawData& getDrawData() const;

    /**
     * @brief Render from captured draw data (threaded mode, automatic)
     * @param cmd Command buffer to record into
     * @param data Draw data from getDrawData()
     *
     * Gets frame index automatically from renderer.
     */
    void renderDrawData(finevk::CommandBuffer& cmd, const GuiDrawData& data);

    /**
     * @brief Render from captured draw data (threaded mode, manual)
     * @param cmd Command buffer to record into
     * @param frameIndex Current frame-in-flight index
     * @param data Draw data from getDrawData()
     */
    void renderDrawData(finevk::CommandBuffer& cmd, uint32_t frameIndex, const GuiDrawData& data);

    // ========================================================================
    // Utilities
    // ========================================================================

    /// Check if GUI wants to capture mouse input
    [[nodiscard]] bool wantCaptureMouse() const;

    /// Check if GUI wants to capture keyboard input
    [[nodiscard]] bool wantCaptureKeyboard() const;

    /// Get ImGui context for advanced usage (fonts, styles, etc.)
    [[nodiscard]] ImGuiContext* imguiContext();

    /// Rebuild font atlas (call after modifying fonts via imguiContext())
    void rebuildFontAtlas();

    /// Get the owning device
    [[nodiscard]] finevk::LogicalDevice* device() const;

    /// Check if initialized
    [[nodiscard]] bool isInitialized() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // State update handlers
    std::unordered_map<uint32_t, std::function<void(const GuiStateUpdate&)>> stateHandlers_;
};

} // namespace finegui
