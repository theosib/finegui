/**
 * @file gui_system.cpp
 * @brief Main GuiSystem implementation
 */

#include <finegui/gui_system.hpp>

#include "backend/imgui_impl_finevk.hpp"

#include <stdexcept>
#include <chrono>

namespace finegui {

// ============================================================================
// Implementation structure
// ============================================================================

struct GuiSystem::Impl {
    finevk::LogicalDevice* device = nullptr;
    GuiConfig config;

    // ImGui context
    ImGuiContext* context = nullptr;

    // Backend
    std::unique_ptr<backend::ImGuiBackend> backend;

    // Rendering state
    finevk::RenderSurface* surface = nullptr;
    uint32_t framesInFlight = 2;
    uint32_t currentFrameIndex = 0;  // For manual mode tracking
    bool initialized = false;

    // Draw data capture (for threaded mode)
    GuiDrawData capturedDrawData;

    // Display state
    float displayWidth = 800.0f;
    float displayHeight = 600.0f;
    float framebufferScaleX = 1.0f;
    float framebufferScaleY = 1.0f;
    float dpiScale = 1.0f;  // Cached from config

    // Time tracking for automatic delta time
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastFrameTime = Clock::now();
    bool firstFrame = true;

    ~Impl() {
        // Destroy backend first while ImGui context is still valid
        // This allows proper cleanup of GPU resources for ImGui textures
        backend.reset();

        if (context) {
            ImGui::DestroyContext(context);
        }
    }
};

// ============================================================================
// Constructor/Destructor
// ============================================================================

GuiSystem::GuiSystem(finevk::LogicalDevice* device, const GuiConfig& config)
    : impl_(std::make_unique<Impl>())
{
    if (!device) {
        throw std::runtime_error("GuiSystem: device cannot be null");
    }

    impl_->device = device;
    impl_->config = config;

    // Determine frames in flight
    impl_->framesInFlight = config.framesInFlight > 0
        ? config.framesInFlight
        : device->framesInFlight();

    if (impl_->framesInFlight == 0) {
        impl_->framesInFlight = 2;  // Safe default
    }

    // Set up DPI scaling
    // dpiScale of 0 means use 1.0 (auto-detect requires window access in initialize())
    impl_->dpiScale = config.dpiScale > 0.0f ? config.dpiScale : 1.0f;
    impl_->framebufferScaleX = impl_->dpiScale;
    impl_->framebufferScaleY = impl_->dpiScale;

    // Create ImGui context
    impl_->context = ImGui::CreateContext();
    ImGui::SetCurrentContext(impl_->context);

    // Configure ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (config.enableGamepad) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }

    // Set display size (will be updated per-frame)
    io.DisplaySize = ImVec2(impl_->displayWidth, impl_->displayHeight);
    io.DisplayFramebufferScale = ImVec2(impl_->framebufferScaleX, impl_->framebufferScaleY);

    // Configure font
    // RasterizerDensity handles high-DPI: rasterizes at dpiScale resolution
    // but displays at the logical font size. No manual size scaling needed.
    float logicalFontSize = config.fontSize * config.fontScale;
    if (!config.fontPath.empty()) {
        ImFontConfig fontConfig;
        fontConfig.RasterizerDensity = impl_->dpiScale;
        io.Fonts->AddFontFromFileTTF(config.fontPath.c_str(), logicalFontSize, &fontConfig);
    } else if (config.fontData && config.fontDataSize > 0) {
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;  // We manage the data
        fontConfig.RasterizerDensity = impl_->dpiScale;
        io.Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(config.fontData),
            static_cast<int>(config.fontDataSize),
            logicalFontSize,
            &fontConfig);
    } else {
        ImFontConfig fontConfig;
        fontConfig.SizePixels = logicalFontSize;
        fontConfig.RasterizerDensity = impl_->dpiScale;
        io.Fonts->AddFontDefaultVector(&fontConfig);
    }

    // Backend is created in initialize() when we have a RenderSurface
}

GuiSystem::~GuiSystem() = default;

GuiSystem::GuiSystem(GuiSystem&&) noexcept = default;
GuiSystem& GuiSystem::operator=(GuiSystem&&) noexcept = default;

// ============================================================================
// Setup
// ============================================================================

void GuiSystem::initialize(finevk::RenderPass* renderPass,
                           finevk::CommandPool* commandPool,
                           uint32_t subpass)
{
    if (!renderPass || !commandPool) {
        throw std::runtime_error("GuiSystem::initialize: renderPass and commandPool required");
    }
    if (!impl_->backend) {
        throw std::runtime_error("GuiSystem::initialize: backend not created. Use initialize(RenderSurface*) instead.");
    }

    ImGui::SetCurrentContext(impl_->context);

    impl_->backend->initialize(renderPass, commandPool, subpass, impl_->config.msaaSamples);
    impl_->initialized = true;
}

void GuiSystem::initialize(finevk::RenderSurface* surface, uint32_t subpass) {
    if (!surface) {
        throw std::runtime_error("GuiSystem::initialize: surface cannot be null");
    }

    impl_->surface = surface;

    // Create backend with the surface (provides device, framesInFlight, deferDelete)
    impl_->backend = std::make_unique<backend::ImGuiBackend>(surface);

    // Get display size from surface (framebuffer size) and convert to logical size
    // For high-DPI displays, displayWidth/Height should be the logical size,
    // while framebufferScale handles the actual pixel scaling
    auto extent = surface->extent();
    impl_->displayWidth = static_cast<float>(extent.width) / impl_->dpiScale;
    impl_->displayHeight = static_cast<float>(extent.height) / impl_->dpiScale;

    initialize(surface->renderPass(), surface->commandPool(), subpass);
}

TextureHandle GuiSystem::registerTexture(finevk::Texture* texture, finevk::Sampler* sampler) {
    if (!impl_->initialized) {
        throw std::runtime_error("GuiSystem::registerTexture: must call initialize() first");
    }

    TextureHandle handle;
    handle.id = impl_->backend->registerTexture(texture, sampler);
    handle.width = texture->width();
    handle.height = texture->height();

    return handle;
}

void GuiSystem::unregisterTexture(TextureHandle handle) {
    if (handle.valid() && impl_->backend) {
        impl_->backend->unregisterTexture(handle.id);
    }
}

// ============================================================================
// Input processing
// ============================================================================

void GuiSystem::processInput(const InputEvent& event) {
    ImGui::SetCurrentContext(impl_->context);
    ImGuiIO& io = ImGui::GetIO();

    // Update modifiers
    io.AddKeyEvent(ImGuiMod_Ctrl, event.ctrl);
    io.AddKeyEvent(ImGuiMod_Shift, event.shift);
    io.AddKeyEvent(ImGuiMod_Alt, event.alt);
    io.AddKeyEvent(ImGuiMod_Super, event.super);

    switch (event.type) {
        case InputEventType::MouseMove:
            io.AddMousePosEvent(event.mouseX, event.mouseY);
            break;

        case InputEventType::MouseButton:
            io.AddMouseButtonEvent(event.button, event.pressed);
            break;

        case InputEventType::MouseScroll:
            io.AddMouseWheelEvent(event.scrollX, event.scrollY);
            break;

        case InputEventType::Key:
            if (event.keyCode != ImGuiKey_None) {
                io.AddKeyEvent(static_cast<ImGuiKey>(event.keyCode), event.keyPressed);
            }
            break;

        case InputEventType::Char:
            if (event.character > 0 && event.character < 0x10000) {
                io.AddInputCharacter(event.character);
            }
            break;

        case InputEventType::Focus:
            io.AddFocusEvent(event.focused);
            break;

        case InputEventType::WindowResize:
            // Convert to logical size for high-DPI
            impl_->displayWidth = static_cast<float>(event.windowWidth) / impl_->dpiScale;
            impl_->displayHeight = static_cast<float>(event.windowHeight) / impl_->dpiScale;
            io.DisplaySize = ImVec2(impl_->displayWidth, impl_->displayHeight);
            break;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void GuiSystem::beginFrame() {
    // Calculate delta time automatically
    auto now = Impl::Clock::now();
    float deltaTime = 1.0f / 60.0f;  // Default for first frame

    if (!impl_->firstFrame) {
        auto duration = std::chrono::duration<float>(now - impl_->lastFrameTime);
        deltaTime = duration.count();
    }
    impl_->lastFrameTime = now;
    impl_->firstFrame = false;

    // Get frame index from renderer
    uint32_t frameIndex = 0;
    if (impl_->surface) {
        frameIndex = impl_->surface->currentFrame();
    }

    beginFrame(frameIndex, deltaTime);
}

void GuiSystem::beginFrame(float deltaTime) {
    // Get frame index from renderer if available
    uint32_t frameIndex = 0;
    if (impl_->surface) {
        frameIndex = impl_->surface->currentFrame();
    }

    beginFrame(frameIndex, deltaTime);
}

void GuiSystem::beginFrame(uint32_t frameIndex, float deltaTime) {
    impl_->currentFrameIndex = frameIndex % impl_->framesInFlight;

    ImGui::SetCurrentContext(impl_->context);
    ImGuiIO& io = ImGui::GetIO();

    // Update display size from renderer if available
    // Convert framebuffer size to logical size for high-DPI support
    if (impl_->surface) {
        auto extent = impl_->surface->extent();
        impl_->displayWidth = static_cast<float>(extent.width) / impl_->dpiScale;
        impl_->displayHeight = static_cast<float>(extent.height) / impl_->dpiScale;
    }

    io.DisplaySize = ImVec2(impl_->displayWidth, impl_->displayHeight);
    io.DisplayFramebufferScale = ImVec2(impl_->framebufferScaleX, impl_->framebufferScaleY);
    io.DeltaTime = deltaTime > 0.0f ? deltaTime : (1.0f / 60.0f);

    ImGui::NewFrame();
}

void GuiSystem::endFrame() {
    ImGui::SetCurrentContext(impl_->context);
    ImGui::Render();

    // Capture draw data if enabled
    if (impl_->config.enableDrawDataCapture) {
        impl_->capturedDrawData.clear();

        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData && drawData->TotalVtxCount > 0) {
            impl_->capturedDrawData.displaySize = glm::vec2(
                drawData->DisplaySize.x, drawData->DisplaySize.y);
            impl_->capturedDrawData.framebufferScale = glm::vec2(
                drawData->FramebufferScale.x, drawData->FramebufferScale.y);

            // Copy vertex/index data
            for (int n = 0; n < drawData->CmdListsCount; n++) {
                const ImDrawList* cmdList = drawData->CmdLists[n];

                size_t vtxOffset = impl_->capturedDrawData.vertices.size();
                size_t idxOffset = impl_->capturedDrawData.indices.size();

                impl_->capturedDrawData.vertices.insert(
                    impl_->capturedDrawData.vertices.end(),
                    cmdList->VtxBuffer.Data,
                    cmdList->VtxBuffer.Data + cmdList->VtxBuffer.Size);

                impl_->capturedDrawData.indices.insert(
                    impl_->capturedDrawData.indices.end(),
                    cmdList->IdxBuffer.Data,
                    cmdList->IdxBuffer.Data + cmdList->IdxBuffer.Size);

                for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
                    const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdIdx];

                    DrawCommand cmd;
                    cmd.indexOffset = static_cast<uint32_t>(idxOffset + pcmd->IdxOffset);
                    cmd.indexCount = pcmd->ElemCount;
                    cmd.vertexOffset = static_cast<uint32_t>(vtxOffset + pcmd->VtxOffset);
                    cmd.texture.id = reinterpret_cast<uint64_t>(pcmd->GetTexID());
                    cmd.scissorRect = glm::ivec4(
                        static_cast<int>(pcmd->ClipRect.x),
                        static_cast<int>(pcmd->ClipRect.y),
                        static_cast<int>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                        static_cast<int>(pcmd->ClipRect.w - pcmd->ClipRect.y));

                    impl_->capturedDrawData.commands.push_back(cmd);
                }
            }
        }
    }
}

void GuiSystem::render(finevk::CommandBuffer& cmd) {
    // Use frame index from beginFrame (automatic or stored from renderer)
    render(cmd, impl_->currentFrameIndex);
}

void GuiSystem::render(finevk::CommandBuffer& cmd, uint32_t frameIndex) {
    if (!impl_->initialized) {
        throw std::runtime_error("GuiSystem::render: must call initialize() first");
    }

    ImGui::SetCurrentContext(impl_->context);
    impl_->backend->render(cmd, frameIndex % impl_->framesInFlight);
}

const GuiDrawData& GuiSystem::getDrawData() const {
    if (!impl_->config.enableDrawDataCapture) {
        throw std::runtime_error("GuiSystem::getDrawData: enableDrawDataCapture not set in config");
    }
    return impl_->capturedDrawData;
}

void GuiSystem::renderDrawData(finevk::CommandBuffer& cmd, const GuiDrawData& data) {
    // Use frame index from beginFrame
    renderDrawData(cmd, impl_->currentFrameIndex, data);
}

void GuiSystem::renderDrawData(finevk::CommandBuffer& cmd, uint32_t frameIndex, const GuiDrawData& data) {
    if (!impl_->initialized) {
        throw std::runtime_error("GuiSystem::renderDrawData: must call initialize() first");
    }

    impl_->backend->renderDrawData(cmd, frameIndex % impl_->framesInFlight, data);
}

// ============================================================================
// Utilities
// ============================================================================

bool GuiSystem::wantCaptureMouse() const {
    ImGui::SetCurrentContext(impl_->context);
    return ImGui::GetIO().WantCaptureMouse;
}

bool GuiSystem::wantCaptureKeyboard() const {
    ImGui::SetCurrentContext(impl_->context);
    return ImGui::GetIO().WantCaptureKeyboard;
}

ImGuiContext* GuiSystem::imguiContext() {
    return impl_->context;
}

void GuiSystem::rebuildFontAtlas() {
    if (!impl_->initialized) {
        throw std::runtime_error("GuiSystem::rebuildFontAtlas: must call initialize() first");
    }

    ImGui::SetCurrentContext(impl_->context);
    // ImGui 1.92+ with ImGuiBackendFlags_RendererHasTextures handles font texture
    // rebuilding automatically through the texture lifecycle system. When fonts
    // are added/modified, ImGui marks the texture as needing update and the backend
    // handles it during the next render call. This function is kept for API
    // compatibility but is effectively a no-op.
}

finevk::LogicalDevice* GuiSystem::device() const {
    return impl_->device;
}

bool GuiSystem::isInitialized() const {
    return impl_->initialized;
}

} // namespace finegui
