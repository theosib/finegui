/**
 * @file scene_texture.cpp
 * @brief SceneTexture implementation
 */

#include <finegui/scene_texture.hpp>
#include <finegui/gui_system.hpp>

#include <stdexcept>

namespace finegui {

SceneTexture::SceneTexture(GuiSystem& gui, uint32_t width, uint32_t height, bool enableDepth)
    : gui_(&gui), width_(width), height_(height)
{
    if (!gui_->isInitialized()) {
        throw std::runtime_error("SceneTexture: GuiSystem must be initialized first");
    }

    auto builder = finevk::OffscreenSurface::create(gui_->device())
        .extent(width, height)
        .colorFormat(VK_FORMAT_R8G8B8A8_SRGB);

    if (enableDepth) {
        builder.enableDepth();
    }

    offscreen_ = builder.build();
}

SceneTexture::~SceneTexture() {
    unregisterTexture();
}

SceneTexture::SceneTexture(SceneTexture&& other) noexcept
    : gui_(other.gui_)
    , offscreen_(std::move(other.offscreen_))
    , handle_(other.handle_)
    , width_(other.width_)
    , height_(other.height_)
    , rendering_(other.rendering_)
{
    other.gui_ = nullptr;
    other.handle_ = {};
    other.rendering_ = false;
}

SceneTexture& SceneTexture::operator=(SceneTexture&& other) noexcept {
    if (this != &other) {
        unregisterTexture();
        gui_ = other.gui_;
        offscreen_ = std::move(other.offscreen_);
        handle_ = other.handle_;
        width_ = other.width_;
        height_ = other.height_;
        rendering_ = other.rendering_;
        other.gui_ = nullptr;
        other.handle_ = {};
        other.rendering_ = false;
    }
    return *this;
}

void SceneTexture::beginScene(float r, float g, float b, float a) {
    if (rendering_) {
        throw std::runtime_error("SceneTexture::beginScene: already rendering");
    }
    offscreen_->beginFrame();
    offscreen_->beginRenderPass({r, g, b, a});
    rendering_ = true;
}

finevk::CommandBuffer& SceneTexture::commandBuffer() {
    if (!rendering_) {
        throw std::runtime_error("SceneTexture::commandBuffer: must call beginScene() first");
    }
    return *offscreen_->currentCommandBuffer();
}

finevk::RenderTarget* SceneTexture::renderTarget() {
    return offscreen_->renderTarget();
}

finevk::OffscreenSurface* SceneTexture::surface() {
    return offscreen_.get();
}

void SceneTexture::endScene() {
    if (!rendering_) {
        throw std::runtime_error("SceneTexture::endScene: must call beginScene() first");
    }
    offscreen_->endRenderPass();
    offscreen_->endFrame();
    rendering_ = false;

    // Register texture on first call (or after resize).
    // The OffscreenSurface is single-buffered, so the colorImageView is stable
    // across frames and doesn't need re-registration each frame.
    if (!handle_.valid()) {
        registerTexture();
    }
}

void SceneTexture::resize(uint32_t width, uint32_t height) {
    if (rendering_) {
        throw std::runtime_error("SceneTexture::resize: cannot resize while rendering");
    }
    unregisterTexture();
    offscreen_->resize(width, height);
    width_ = width;
    height_ = height;
    // Handle will be registered on next endScene()
}

void SceneTexture::registerTexture() {
    if (!gui_ || !offscreen_) return;
    handle_ = gui_->registerTexture(
        offscreen_->colorImageView(),
        offscreen_->colorSampler(),
        width_, height_);
}

void SceneTexture::unregisterTexture() {
    if (gui_ && handle_.valid()) {
        gui_->unregisterTexture(handle_);
        handle_ = {};
    }
}

} // namespace finegui
