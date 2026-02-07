# finegui LLM Reference

Dense API reference for finegui (Dear ImGui + finevk Vulkan backend). Optimized for LLM context windows.

## Build

Builds both static (`libfinegui.a`) and shared (`libfinegui.dylib`) libraries. Tests link against the shared library. Examples link against the static library.

## Architecture

finegui wraps Dear ImGui with a Vulkan backend using finevk. Immediate-mode GUI: call widget functions each frame, no persistent widget objects. Renders as the last step in a render pass (on top of 3D content).

Backend handles ImGui 1.92+ texture lifecycle (`ImGuiBackendFlags_RendererHasTextures`): `WantCreate`, `WantUpdates` (lazy glyph rasterization), `WantDestroy`. Uses `RenderSurface::deferDelete()` for GPU-safe resource cleanup without stalling.

## Headers

```cpp
#include <finegui/finegui.hpp>  // Umbrella: all public headers
// Individual:
#include <finegui/gui_system.hpp>    // GuiSystem class
#include <finegui/gui_config.hpp>    // GuiConfig struct
#include <finegui/input_adapter.hpp> // InputEvent, InputAdapter
#include <finegui/texture_handle.hpp>// TextureHandle
#include <finegui/gui_draw_data.hpp> // GuiDrawData, DrawCommand
#include <finegui/gui_state.hpp>     // TypedStateUpdate<T>
```

## GuiConfig

```cpp
struct GuiConfig {
    float dpiScale = 0.0f;          // 0=use 1.0; set window->contentScale().x for HiDPI
    float fontScale = 1.0f;         // Multiplier on fontSize
    float fontSize = 16.0f;         // Logical pixels; RasterizerDensity handles HiDPI
    std::string fontPath;           // TTF path (empty=ProggyVector default)
    const void* fontData = nullptr; // Alt: font from memory
    size_t fontDataSize = 0;
    bool enableKeyboard = true;
    bool enableGamepad = false;
    uint32_t framesInFlight = 0;    // 0=auto from device
    bool enableDrawDataCapture = false; // For threaded rendering
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // Must match render pass
};
```

## GuiSystem

```cpp
class GuiSystem {
    // Lifecycle
    explicit GuiSystem(finevk::LogicalDevice* device, const GuiConfig& config = {});
    ~GuiSystem();
    GuiSystem(GuiSystem&&) noexcept;

    // Init (pick one overload)
    void initialize(finevk::RenderSurface* surface, uint32_t subpass = 0); // Primary
    void initialize(finevk::SimpleRenderer* renderer, uint32_t subpass = 0); // Compat (delegates to above)
    void initialize(finevk::RenderPass* rp, finevk::CommandPool* cp, uint32_t subpass = 0); // Manual (needs surface first)

    // Textures
    TextureHandle registerTexture(finevk::Texture* tex, finevk::Sampler* sampler = nullptr);
    void unregisterTexture(TextureHandle handle);

    // Input
    void processInput(const InputEvent& event);
    template<typename C> void processInputBatch(const C& events);

    // State updates (message-passing)
    template<typename T> void applyState(const T& update);
    template<typename T> void onStateUpdate(std::function<void(const T&)> handler);

    // Frame (call order: beginFrame -> ImGui widgets -> endFrame -> render)
    void beginFrame();                               // Auto dt + frame index from surface
    void beginFrame(float deltaTime);                // Auto frame index
    void beginFrame(uint32_t frameIndex, float dt);  // Full manual
    void endFrame();

    // Render (inside active render pass)
    void render(finevk::CommandBuffer& cmd);                    // Auto frame index
    void render(finevk::CommandBuffer& cmd, uint32_t frameIdx); // Manual

    // Threaded mode (requires enableDrawDataCapture=true)
    const GuiDrawData& getDrawData() const;
    void renderDrawData(finevk::CommandBuffer& cmd, const GuiDrawData& data);
    void renderDrawData(finevk::CommandBuffer& cmd, uint32_t frameIdx, const GuiDrawData& data);

    // Queries
    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;
    ImGuiContext* imguiContext();
    finevk::LogicalDevice* device() const;
    bool isInitialized() const;
    void rebuildFontAtlas();
};
```

## Input

```cpp
enum class InputEventType { MouseMove, MouseButton, MouseScroll, Key, Char, Focus, WindowResize };

struct InputEvent {
    InputEventType type;
    float mouseX, mouseY;           // Position (screen coords)
    float scrollX, scrollY;
    int button;                     // 0=left 1=right 2=middle
    bool pressed;
    int keyCode;                    // ImGuiKey_* values
    bool keyPressed;
    uint32_t character;             // Unicode codepoint (Char events)
    bool ctrl, shift, alt, super;   // Modifiers
    int windowWidth, windowHeight;  // WindowResize events
    bool focused;                   // Focus events
    double time;                    // Timestamp
};

struct InputAdapter {
    static InputEvent fromFineVK(const finevk::InputEvent& event);
    static std::vector<InputEvent> fromInputManager(finevk::InputManager& input);
    static int glfwKeyToImGui(int glfwKey);
    static int glfwMouseButtonToImGui(int glfwButton);
};
```

## TextureHandle

```cpp
struct TextureHandle {
    uint64_t id = 0;
    uint32_t width = 0, height = 0;
    bool valid() const;
    operator ImTextureID() const;  // Implicit conversion for ImGui::Image()
};
```

## GuiDrawData (Threaded Mode)

```cpp
struct DrawCommand {
    uint32_t indexOffset, indexCount, vertexOffset;
    TextureHandle texture;
    glm::ivec4 scissorRect; // x, y, width, height
};

struct GuiDrawData {
    std::vector<ImDrawVert> vertices;
    std::vector<ImDrawIdx> indices;
    std::vector<DrawCommand> commands;
    glm::vec2 displaySize, framebufferScale;
    bool empty() const;
    void clear();
};
```

## State Updates (Message Passing)

```cpp
// Base
struct GuiStateUpdate { virtual uint32_t typeId() const = 0; };

// Derive with CRTP for auto type ID
template<typename T>
struct TypedStateUpdate : GuiStateUpdate {
    static uint32_t staticTypeId();  // Unique per T
    uint32_t typeId() const override;
};

// Usage:
struct HealthUpdate : finegui::TypedStateUpdate<HealthUpdate> { float hp, maxHp; };
gui.onStateUpdate<HealthUpdate>([](const HealthUpdate& u) { /* store */ });
gui.applyState(HealthUpdate{80.f, 100.f});
```

## Game Loop Integration

### Render Order

Everything in one render pass. Order determines layering:

```
beginRenderPass(clearColor)
  1. 3D world (depth-tested)
  2. 3D entities (depth-tested)
  3. Particles/effects (alpha blended)
  4. 2D overlays (crosshair, minimap border)
  5. GUI (finegui) -- always last, on top
endRenderPass()
```

### Canonical Game Loop

```cpp
while (window->isOpen()) {
    window->pollEvents();

    // Input
    inputManager->update();
    finevk::InputEvent ev;
    while (inputManager->pollEvent(ev))
        gui.processInput(finegui::InputAdapter::fromFineVK(ev));

    // Game input (respect GUI capture)
    if (!gui.wantCaptureMouse())   { /* camera, interaction */ }
    if (!gui.wantCaptureKeyboard()){ /* movement, hotkeys */ }

    // Game logic
    physics.update(dt);
    camera.update(dt);
    worldRenderer.updateCamera(camera);
    worldRenderer.updateMeshes(16);

    // GUI frame
    gui.beginFrame();
    drawHUD();
    if (inventoryOpen) drawInventory();
    if (showDebug) drawDebugOverlay();
    gui.endFrame();

    // Render
    if (auto frame = renderer->beginFrame()) {
        frame.beginRenderPass(clearColor);
        worldRenderer.render(frame);    // 3D
        overlay.render(frame);          // 2D overlays
        gui.render(frame);              // GUI last
        frame.endRenderPass();
        renderer->endFrame();
    }
}
```

### Input Capture Pattern

```cpp
// Forward ALL events to GUI first, then check capture flags
while (inputManager->pollEvent(ev))
    gui.processInput(finegui::InputAdapter::fromFineVK(ev));

// Then gate game input
if (!gui.wantCaptureMouse()) {
    // Camera look, block break/place, etc.
}
```

## 2D GUI Patterns

### Transparent HUD Overlay

```cpp
ImGui::SetNextWindowPos(ImVec2(10, 10));
ImGui::Begin("##hud", nullptr,
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoInputs);  // Click-through
ImGui::ProgressBar(hp / maxHp);
ImGui::End();
```

### Centered Bottom Hotbar

```cpp
float w = 9 * 52.0f;
ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - w) / 2, io.DisplaySize.y - 60));
ImGui::Begin("##hotbar", nullptr, ImGuiWindowFlags_NoTitleBar | ...);
for (int i = 0; i < 9; i++) {
    if (i > 0) ImGui::SameLine();
    if (i == selected) ImGui::PushStyleColor(ImGuiCol_Button, highlight);
    ImGui::ImageButton("##slot", slotIcons[i], ImVec2(48, 48));
    if (i == selected) ImGui::PopStyleColor();
}
ImGui::End();
```

### Minimap (Texture in GUI)

```cpp
// minimapTex = gui.registerTexture(minimapRenderTarget->colorTexture());
ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 210, 10));
ImGui::Begin("##minimap", nullptr, fixed_flags);
ImGui::Image(minimapTex, ImVec2(200, 200));
ImGui::End();
```

## 3D Objects in GUI

Pattern: render 3D to OffscreenSurface, register texture, display via ImGui::Image.

### Static Item Icons (Pre-rendered)

```cpp
// Setup
auto iconSurface = finevk::OffscreenSurface::create(device)
    .size(128, 128).colorFormat(VK_FORMAT_R8G8B8A8_UNORM)
    .enableDepthBuffer(true).build();

finevk::Camera iconCam;
iconCam.setPerspective(45.f, 1.f, 0.1f, 10.f);
iconCam.moveTo({2, 1.5, 2}); iconCam.lookAt({0, 0, 0});

// Render one icon
iconSurface->beginFrame();
iconSurface->beginRenderPass({0, 0, 0, 0}); // Transparent
blockRenderer.render(iconSurface->commandBuffer(), blockType, iconCam);
iconSurface->endRenderPass();
iconSurface->endFrame();

TextureHandle icon = gui.registerTexture(iconSurface->colorTexture());

// Use in GUI
ImGui::Image(icon, ImVec2(48, 48));
```

### Icon Cache

```cpp
class IconCache {
    std::unordered_map<uint32_t, TextureHandle> cache_;
public:
    TextureHandle get(BlockTypeId id) {
        auto it = cache_.find(id.id);
        if (it != cache_.end()) return it->second;
        // render to offscreen, register, store in cache_
        return cache_[id.id] = renderAndRegister(id);
    }
};
```

### Animated 3D Preview (Per-Frame)

```cpp
// Each frame: re-render with rotating camera
float angle = ImGui::GetTime() * 45.0f;
iconCam.moveTo({2.5*cos(rad(angle)), 1.5, 2.5*sin(rad(angle))});
iconCam.lookAt({0, 0, 0});

iconSurface->beginFrame();
iconSurface->beginRenderPass({0, 0, 0, 0});
blockRenderer.render(iconSurface->commandBuffer(), blockType, iconCam);
iconSurface->endRenderPass();
iconSurface->endFrame();

gui.unregisterTexture(previewHandle);
previewHandle = gui.registerTexture(iconSurface->colorTexture());
ImGui::Image(previewHandle, ImVec2(128, 128));
```

## Threaded Rendering

```cpp
// Config
guiConfig.enableDrawDataCapture = true;

// Game thread: build GUI
gui.beginFrame();
/* widgets */
gui.endFrame();
auto drawData = gui.getDrawData(); // Copy to send to render thread

// Render thread: replay draw data
frame.beginRenderPass(clearColor);
worldRenderer.render(frame);
gui.renderDrawData(frame.commandBuffer(), drawData);
frame.endRenderPass();
```

## High-DPI

```cpp
guiConfig.dpiScale = window->contentScale().x; // e.g. 2.0 on Retina
guiConfig.fontSize = 16.0f; // Logical size -- NOT scaled manually
```

Internally: `io.DisplaySize` = framebuffer/dpiScale (logical coords), `io.DisplayFramebufferScale` = dpiScale, font uses `RasterizerDensity = dpiScale` for native-res rasterization at logical display size.

## HiDPI + Mouse

GLFW mouse coords are already in screen (logical) coordinates. No scaling needed. finegui passes them through directly.

## Destruction Order

Backend destroyed before ImGui context (`backend.reset()` in `~Impl()` before `DestroyContext()`). This allows proper GPU texture cleanup through `GetPlatformIO().Textures`.

## Key Constraints

- `msaaSamples` in GuiConfig MUST match the render pass
- `render()` must be called inside an active render pass
- `beginFrame()` before any ImGui calls, `endFrame()` after
- All ImGui widget calls between `beginFrame()`/`endFrame()`
- `registerTexture()` requires `initialize()` first
- Backend uses `surface->deferDelete()` -- no `waitIdle()` in render path
- `waitIdle()` only in destructor (shutdown, acceptable)
- Pool must outlive all deferred DescriptorSetPtr objects (naturally satisfied)
