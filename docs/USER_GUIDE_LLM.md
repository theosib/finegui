# finegui LLM Reference

Dense API reference for finegui (Dear ImGui + finevk Vulkan backend). Optimized for LLM context windows.

## Build

Three layered libraries, each static + shared:
- `finegui` — Core (immediate mode, finevk backend)
- `finegui-retained` — Retained-mode widgets (`FINEGUI_BUILD_RETAINED=ON`)
- `finegui-script` — Script integration (`FINEGUI_BUILD_SCRIPT=ON`, requires finescript)

Tests link shared. Examples link static.

## Architecture

finegui wraps Dear ImGui with a Vulkan backend using finevk. Four levels:
1. **Core**: Immediate-mode — call ImGui widget functions each frame
2. **Retained**: Declarative `WidgetNode` trees rendered by `GuiRenderer`
3. **Map**: finescript maps rendered directly by `MapRenderer` (no WidgetNode conversion)
4. **Script**: finescript scripts build widget maps via `ui.*` functions, rendered by `MapRenderer`

Backend handles ImGui 1.92+ texture lifecycle (`ImGuiBackendFlags_RendererHasTextures`): `WantCreate`, `WantUpdates` (lazy glyph rasterization), `WantDestroy`. Uses `RenderSurface::deferDelete()` for GPU-safe resource cleanup without stalling.

## Headers

```cpp
// Core
#include <finegui/finegui.hpp>       // Umbrella: all core headers
#include <finegui/gui_system.hpp>    // GuiSystem class
#include <finegui/gui_config.hpp>    // GuiConfig struct
#include <finegui/input_adapter.hpp> // InputEvent, InputAdapter
#include <finegui/texture_handle.hpp>// TextureHandle
#include <finegui/gui_draw_data.hpp> // GuiDrawData, DrawCommand
#include <finegui/gui_state.hpp>     // TypedStateUpdate<T>
#include <finegui/scene_texture.hpp> // SceneTexture (offscreen 3D-in-GUI)

// Retained mode
#include <finegui/widget_node.hpp>   // WidgetNode struct + builders
#include <finegui/gui_renderer.hpp>  // GuiRenderer class
#include <finegui/drag_drop_manager.hpp> // DragDropManager
#include <finegui/texture_registry.hpp>  // TextureRegistry

// Map-based rendering
#include <finegui/map_renderer.hpp>  // MapRenderer class

// Script integration
#include <finegui/script_gui.hpp>           // ScriptGui
#include <finegui/script_gui_manager.hpp>   // ScriptGuiManager
#include <finegui/script_bindings.hpp>      // registerGuiBindings()
#include <finegui/widget_converter.hpp>     // convertToWidget(), ConverterSymbols
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

## GuiMode

```cpp
enum class GuiMode {
    Auto,       // Use ImGui's WantCapture* flags (default)
    Passive,    // Feed to ImGui, always pass through (never block game)
    Exclusive   // Consume all input (menu/inventory mode)
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

    // InputManager integration
    int connectToInputManager(finevk::InputManager& input, int priority = 300);
    void disconnectFromInputManager();
    finevk::ListenerResult handleInputEvent(const finevk::InputEvent& event);
    void setGuiMode(GuiMode mode);  // Auto, Passive, Exclusive
    GuiMode guiMode() const;

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

### SceneTexture (Offscreen 3D-in-GUI Helper)

`#include <finegui/scene_texture.hpp>`

Wraps a finevk `OffscreenSurface` with automatic GuiSystem texture registration. Renders a 3D scene (or any Vulkan drawing) offscreen and produces a `TextureHandle` for display in Image or Canvas widgets.

```cpp
class SceneTexture {
    SceneTexture(GuiSystem& gui, uint32_t width, uint32_t height, bool enableDepth = true);
    ~SceneTexture();

    // Render cycle
    void beginScene(float r=0, float g=0, float b=0, float a=1); // Clear + begin render pass
    finevk::CommandBuffer& commandBuffer();                       // Record draw commands (valid between begin/end)
    void endScene();                                              // Submit + wait; textureHandle() valid after this

    // Display
    TextureHandle textureHandle() const;      // For Image/Canvas widgets (invalid before first endScene)
    uint32_t width() const;
    uint32_t height() const;

    // Access (for pipeline creation / advanced usage)
    finevk::RenderTarget* renderTarget();     // Compatible render pass for pipeline creation
    finevk::OffscreenSurface* surface();      // Underlying surface

    // Resize
    void resize(uint32_t width, uint32_t height); // Unregisters old, re-registers new; call endScene() after to get valid handle
};
```

Usage:
```cpp
// Create
SceneTexture scene(gui, 512, 512);

// Build pipeline against scene's render target
auto pipeline = PipelineBuilder(device)
    .renderTarget(scene.renderTarget())
    /* ... shaders, vertex layout, etc. ... */
    .build();

// Each frame: render 3D content
scene.beginScene(0, 0, 0, 1);          // Clear to black
auto& cmd = scene.commandBuffer();
pipeline->bind(cmd);
mesh.draw(cmd);
scene.endScene();

// Display in Image widget
ImGui::Image(scene.textureHandle(), ImVec2(512, 512));

// Or in retained mode
auto node = WidgetNode::image(scene.textureHandle(), 512, 512);

// Or in a Canvas (texture drawn before draw commands)
auto viewport = WidgetNode::canvas("viewport", 512, 512, scene.textureHandle());
```

Resize:
```cpp
device->waitIdle();             // GPU must be idle before resize
scene.resize(newW, newH);
// Must call beginScene/endScene at least once before using textureHandle()
scene.beginScene(0, 0, 0, 1);
/* re-render */
scene.endScene();
// scene.textureHandle() now valid at new size
```

### Canvas with Texture

Canvas widgets now support an optional texture that is drawn via `ImDrawList::AddImage()` before any draw commands and the `onDraw` callback. This is the primary way to display a SceneTexture inside a Canvas.

**Retained mode** -- overloaded builder:
```cpp
// Canvas with texture (no onDraw callback; onClick optional)
static WidgetNode canvas(std::string id, float width, float height,
                         TextureHandle texture, WidgetCallback onClick = {});

// Original canvas (no texture; onDraw + onClick)
static WidgetNode canvas(std::string id, float width, float height,
                         WidgetCallback onDraw = {}, WidgetCallback onClick = {});
```

The texture overload sets `node.texture` on the Canvas. The renderer draws the texture first, then executes `onDraw` (if set), so custom draw commands layer on top of the texture.

**Script mode** -- Canvas map with `:texture` field:
```
ui.canvas "viewport" 512 512 [
    {ui.draw_rect [10 10] [100 100] [1 0 0 1] true}
]
```
Add `:texture "name"` to the canvas map to draw a registered texture as the background:
```
set canvas_map {ui.canvas "viewport" 512 512 []}
set canvas_map.texture "scene_tex"
```
The texture name is resolved via `TextureRegistry`. The texture is drawn before any draw commands in the commands array.

### GPU Safety: Descriptor Set Lifetime

SceneTexture registers its texture with GuiSystem once (after the first `endScene()`) and only re-registers after `resize()`. The registered descriptor set is used by in-flight frames, so:

- Call `device->waitIdle()` before `resize()` to ensure no in-flight frames reference the old descriptor set.
- Call `device->waitIdle()` before destroying a SceneTexture if frames may still be in flight.
- Do NOT call `gui.unregisterTexture()` on a SceneTexture handle -- the SceneTexture destructor handles cleanup.
- After `resize()`, call `endScene()` at least once before displaying the texture (the handle is invalid between resize and the next endScene).

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
- ScriptEngine must outlive Vulkan resources (declare it first)
- ScriptGui::close() removes tree before destroying context (closure safety)
- Modal dialogs close on Escape key (fires onClose callback)
- `connectToInputManager()` replaces manual event polling for simpler integration
- Focus management: `focusable`, `autoFocus`, `onFocus`, `onBlur` on WidgetNode; `setFocus()` on renderers
- `findById()` on both renderers for widget search by `:id` string; `ui.find` in scripts
- Animation fields: `alpha`, `windowPosX`, `windowPosY`, `scaleX`, `scaleY`, `rotationY` on WidgetNode (FLT_MAX = auto-position for pos; 1.0 = normal scale; 0.0 = no rotation)
- TweenManager: call `update(dt)` before `renderAll()` each frame
- Vertex post-processing transform (scale/rotation) only affects the window's own draw list, not nested `BeginChild` regions
- Vulkan pipeline uses `cullNone` so Y-axis flips work without back-face culling issues
- `ContextMenu` must be placed immediately after its target widget in the children list; right-clicking the target opens the menu
- `MainMenuBar` must be a top-level tree (not inside a window); renders at the top of the screen
- `ui.close_popup` must be called during popup rendering (e.g., from a button callback inside a popup/modal)
- `ItemTooltip` must be placed immediately after the target widget in the children list (same rule as `Tooltip`)
- `ImageButton` requires a valid TextureHandle; skips rendering silently if texture is invalid
- `PlotLines`/`PlotHistogram`: `plotValues` stores float array data; `minFloat`/`maxFloat` = FLT_MAX for auto-scaling; `overlayText` for optional text overlay; `width`/`height` for graph size (0 = auto)
- `PushTheme`/`PopTheme` must be paired with matching preset names; pop count is determined by the preset name

## WidgetNode (Retained Mode)

```cpp
using WidgetCallback = std::function<void(WidgetNode& widget)>;

struct WidgetNode {
    enum class Type {
        // Phase 1 - Core widgets
        Window, Text, Button, Checkbox, Slider, SliderInt,
        InputText, InputInt, InputFloat, Combo, Separator, Group, Columns, Image,
        // Phase 3 - Layout & Display
        SameLine, Spacing, TextColored, TextWrapped, TextDisabled,
        ProgressBar, CollapsingHeader,
        // Phase 4 - Containers & Menus
        TabBar, TabItem, TreeNode, Child, MenuBar, Menu, MenuItem,
        // Phase 5 - Tables
        Table, TableColumn, TableRow,
        // Phase 6 - Advanced Input
        ColorEdit, ColorPicker, DragFloat, DragInt,
        DragFloat3, InputTextWithHint, SliderAngle, SmallButton, ColorButton,
        // Phase 7 - Misc
        ListBox, Popup, Modal,
        // Phase 8 - Custom
        Canvas, Tooltip,
        // Phase 9 - New widgets
        RadioButton, Selectable, InputTextMultiline,
        BulletText, SeparatorText, Indent, Dummy, NewLine,
        // Phase 10 - Style push/pop
        PushStyleColor, PopStyleColor, PushStyleVar, PopStyleVar,
        // Phase 13 - Menus & Popups
        ContextMenu, MainMenuBar,
        // Phase 14 - Tooltips & Images (continued)
        ItemTooltip, ImageButton,
        // Phase 15 - Plots
        PlotLines, PlotHistogram
    };

    Type type;

    // Display properties
    std::string label, textContent, id;

    // Value storage
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    std::string stringValue;
    int selectedIndex = -1;         // Combo, ListBox

    // Range constraints (sliders, drags)
    float minFloat = 0.0f, maxFloat = 1.0f;
    int minInt = 0, maxInt = 100;

    // Layout
    float width = 0.0f, height = 0.0f;  // 0 = auto
    int columnCount = 1;

    // Items (Combo, ListBox)
    std::vector<std::string> items;

    // Children (Window, Group, Columns, TabBar, etc.)
    std::vector<WidgetNode> children;

    // Visibility / enabled
    bool visible = true, enabled = true;

    // Callbacks
    WidgetCallback onClick;         // Button, MenuItem, Image
    WidgetCallback onChange;        // Checkbox, Slider, Input, Combo, ColorEdit
    WidgetCallback onSubmit;        // InputText (Enter pressed)
    WidgetCallback onClose;         // Window close button

    // Image
    TextureHandle texture{};
    float imageWidth = 0.0f, imageHeight = 0.0f;

    // Phase 3
    float offsetX = 0.0f;                    // SameLine offset
    float colorR=1, colorG=1, colorB=1, colorA=1; // TextColored, ColorEdit, ColorPicker
    std::string overlayText;                 // ProgressBar overlay
    bool defaultOpen = false;                // CollapsingHeader, TreeNode

    // Phase 4
    bool border = false, autoScroll = false; // Child
    std::string shortcutText;                // MenuItem
    bool checked = false, leaf = false;      // MenuItem, TreeNode

    // Phase 5
    int tableFlags = 0;                      // ImGuiTableFlags_*

    // Phase 6
    float dragSpeed = 1.0f;                  // DragFloat, DragInt, DragFloat3
    float floatX = 0.0f, floatY = 0.0f, floatZ = 0.0f; // DragFloat3
    std::string hintText;                    // InputTextWithHint placeholder

    // Phase 7
    int heightInItems = -1;                  // ListBox (-1 = auto)

    // Phase 8
    WidgetCallback onDraw;                   // Canvas custom draw

    // Phase 9
    int windowFlags = 0;                     // ImGuiWindowFlags_* (includes NoNav, NoInputs)

    // Drag-and-Drop (any widget)
    std::string dragType;           // non-empty = drag source
    std::string dragData;           // payload data string
    std::string dropAcceptType;     // non-empty = drop target
    WidgetCallback onDrop;          // called on drop target; node.dragData = delivered payload
    WidgetCallback onDragBegin;     // called on drag source when drag starts
    int dragMode = 0;               // 0=both, 1=drag-only, 2=click-to-pick-up only

    // Focus management
    bool focusable = true;          // false → skip in tab navigation
    bool autoFocus = false;         // focus when parent window first appears
    WidgetCallback onFocus;         // called when widget gains keyboard focus
    WidgetCallback onBlur;          // called when widget loses keyboard focus

    // Animation (used by TweenManager)
    float alpha = 1.0f;            // Window opacity (0.0=invisible, 1.0=opaque)
    float windowPosX = FLT_MAX;    // Explicit window position (FLT_MAX=auto)
    float windowPosY = FLT_MAX;
    float scaleX = 1.0f;           // Window scale X (1.0=normal, 0.0=collapsed to center)
    float scaleY = 1.0f;           // Window scale Y (1.0=normal, 0.0=collapsed to center)
    float rotationY = 0.0f;        // Y-axis rotation in radians (0=facing forward, PI=flipped)

    // Window control
    float windowSizeW = 0.0f;     // Programmatic window width (0 = auto)
    float windowSizeH = 0.0f;     // Programmatic window height (0 = auto)

    // Phase 15
    std::vector<float> plotValues; // PlotLines, PlotHistogram data

    // --- Static builders (Phase 1) ---
    static WidgetNode window(string title, vector<WidgetNode> children = {}, int flags = 0);
    static WidgetNode window(string title, float width, float height,
                             vector<WidgetNode> children = {}, int flags = 0);
    static WidgetNode text(string content);
    static WidgetNode button(string label, WidgetCallback onClick = {});
    static WidgetNode checkbox(string label, bool value, WidgetCallback onChange = {});
    static WidgetNode slider(string label, float value, float min, float max, WidgetCallback onChange = {});
    static WidgetNode sliderInt(string label, int value, int min, int max, WidgetCallback onChange = {});
    static WidgetNode inputText(string label, string value, WidgetCallback onChange = {}, WidgetCallback onSubmit = {});
    static WidgetNode inputInt(string label, int value, WidgetCallback onChange = {});
    static WidgetNode inputFloat(string label, float value, WidgetCallback onChange = {});
    static WidgetNode combo(string label, vector<string> items, int selected, WidgetCallback onChange = {});
    static WidgetNode separator();
    static WidgetNode group(vector<WidgetNode> children);
    static WidgetNode columns(int count, vector<WidgetNode> children);
    static WidgetNode image(TextureHandle texture, float width, float height);

    // --- Phase 3 builders ---
    static WidgetNode sameLine(float offset = 0);
    static WidgetNode spacing();
    static WidgetNode textColored(float r, float g, float b, float a, string content);
    static WidgetNode textWrapped(string content);
    static WidgetNode textDisabled(string content);
    static WidgetNode progressBar(float fraction, float w=0, float h=0, string overlay="");
    static WidgetNode collapsingHeader(string label, vector<WidgetNode> children={}, bool open=false);

    // --- Phase 4 builders ---
    static WidgetNode tabBar(string id, vector<WidgetNode> children={});
    static WidgetNode tabItem(string label, vector<WidgetNode> children={});
    static WidgetNode treeNode(string label, vector<WidgetNode> children={}, bool open=false, bool leaf=false);
    static WidgetNode child(string id, float w=0, float h=0, bool border=false, bool autoScroll=false,
                            vector<WidgetNode> children={});
    static WidgetNode menuBar(vector<WidgetNode> children={});
    static WidgetNode menu(string label, vector<WidgetNode> children={});
    static WidgetNode menuItem(string label, WidgetCallback onClick={}, string shortcut="", bool checked=false);

    // --- Phase 5 builders ---
    static WidgetNode table(string id, int cols, vector<string> headers={},
                            vector<WidgetNode> children={}, int flags=0);
    static WidgetNode tableRow(vector<WidgetNode> children={});
    static WidgetNode tableNextColumn();

    // --- Phase 6 builders ---
    static WidgetNode colorEdit(string label, float r=1, float g=1, float b=1, float a=1,
                                WidgetCallback onChange={});
    static WidgetNode colorPicker(string label, float r=1, float g=1, float b=1, float a=1,
                                  WidgetCallback onChange={});
    static WidgetNode dragFloat(string label, float val, float speed=1, float min=0, float max=0,
                                WidgetCallback onChange={});
    static WidgetNode dragInt(string label, int val, float speed=1, int min=0, int max=0,
                              WidgetCallback onChange={});
    static WidgetNode dragFloat3(string label, float x, float y, float z,
                                  float speed=1, float min=0, float max=0,
                                  WidgetCallback onChange={});
    static WidgetNode inputTextWithHint(string label, string hint, string value="",
                                         WidgetCallback onChange={}, WidgetCallback onSubmit={});
    static WidgetNode sliderAngle(string label, float valueRadians=0, float minDegrees=-360,
                                   float maxDegrees=360, WidgetCallback onChange={});
    static WidgetNode smallButton(string label, WidgetCallback onClick={});
    static WidgetNode colorButton(string label, float r=1, float g=1, float b=1, float a=1,
                                   WidgetCallback onClick={});

    // --- Phase 7 builders ---
    static WidgetNode listBox(string label, vector<string> items, int sel=0, int height=-1,
                              WidgetCallback onChange={});
    static WidgetNode popup(string id, vector<WidgetNode> children={});
    static WidgetNode modal(string title, vector<WidgetNode> children={}, WidgetCallback onClose={});

    // --- Phase 8 builders ---
    static WidgetNode canvas(string id, float w, float h, WidgetCallback onDraw={}, WidgetCallback onClick={});
    static WidgetNode canvas(string id, float w, float h, TextureHandle texture, WidgetCallback onClick={});
    static WidgetNode tooltip(string text);
    static WidgetNode tooltip(vector<WidgetNode> children);

    // --- Phase 9 builders ---
    static WidgetNode radioButton(string label, int activeVal, int myVal, WidgetCallback onChange={});
    static WidgetNode selectable(string label, bool selected=false, WidgetCallback onClick={});
    static WidgetNode inputTextMultiline(string label, string val="", float w=0, float h=0,
                                          WidgetCallback onChange={});
    static WidgetNode bulletText(string content);
    static WidgetNode separatorText(string label);
    static WidgetNode indent(float amount=0);
    static WidgetNode unindent(float amount=0);
    static WidgetNode dummy(float width, float height);
    static WidgetNode newLine();

    // --- Phase 10 builders ---
    static WidgetNode pushStyleColor(int colIdx, float r, float g, float b, float a);
    static WidgetNode popStyleColor(int count=1);
    static WidgetNode pushStyleVar(int varIdx, float val);
    static WidgetNode pushStyleVar(int varIdx, float x, float y);
    static WidgetNode popStyleVar(int count=1);

    // --- Phase 13 builders ---
    static WidgetNode contextMenu(std::vector<WidgetNode> children = {});
    static WidgetNode mainMenuBar(std::vector<WidgetNode> children = {});

    // --- Phase 14 builders ---
    static WidgetNode itemTooltip(string text);
    static WidgetNode itemTooltip(vector<WidgetNode> children);
    static WidgetNode imageButton(string id, TextureHandle texture, float width, float height, WidgetCallback onClick={});

    // --- Phase 15 builders ---
    static WidgetNode plotLines(std::string label, std::vector<float> values,
                                std::string overlay = "", float scaleMin = FLT_MAX, float scaleMax = FLT_MAX,
                                float width = 0.0f, float height = 0.0f);
    static WidgetNode plotHistogram(std::string label, std::vector<float> values,
                                   std::string overlay = "", float scaleMin = FLT_MAX, float scaleMax = FLT_MAX,
                                   float width = 0.0f, float height = 0.0f);

    // --- Style & Theming builders ---
    static WidgetNode pushTheme(std::string name);
    static WidgetNode popTheme(std::string name);
};
```

### Window Flags Reference

Common `ImGuiWindowFlags_*` values for `windowFlags` / `flags` parameter:
- `ImGuiWindowFlags_NoTitleBar` — hides the title bar
- `ImGuiWindowFlags_NoResize` — prevents user resizing
- `ImGuiWindowFlags_NoMove` — prevents user moving
- `ImGuiWindowFlags_NoBackground` — transparent window background
- `ImGuiWindowFlags_NoNav` — disables keyboard/gamepad navigation
- `ImGuiWindowFlags_NoInputs` — disables all inputs (`NoMouseInputs | NoNav`)

## GuiRenderer (Retained Mode)

```cpp
class GuiRenderer {
    explicit GuiRenderer(GuiSystem& gui);

    int show(WidgetNode tree);              // Register tree, returns ID
    void update(int guiId, WidgetNode tree); // Replace tree
    void hide(int guiId);                    // Remove tree
    void hideAll();                          // Remove all
    WidgetNode* get(int guiId);              // Get for direct mutation (nullptr if not found)
    void renderAll();                        // Call between beginFrame/endFrame
    void setDragDropManager(DragDropManager* manager); // Enable click-to-pick-up DnD
    void setFocus(const std::string& widgetId); // Programmatic focus by widget ID
    WidgetNode* findById(const std::string& widgetId); // Find widget by :id string (nullptr if not found)
};
```

Usage:
```cpp
GuiRenderer renderer(gui);
int id = renderer.show(WidgetNode::window("Title", { WidgetNode::text("Hello") }));

// Each frame:
gui.beginFrame();
renderer.renderAll();
gui.endFrame();

// Mutate:
auto* tree = renderer.get(id);
tree->children[0].textContent = "Updated";

// Find by ID (searches all trees recursively):
auto* widget = renderer.findById("hp_bar");
if (widget) widget->floatValue = 0.5f;

// Remove:
renderer.hide(id);
```

## DragDropManager

Global state for click-to-pick-up drag-and-drop mode. Traditional ImGui DnD (click-drag-release) works automatically. Click-to-pick-up mode needs this manager to track what the cursor is carrying.

```cpp
class DragDropManager {
public:
    struct CursorItem {
        std::string type, data, fallbackText;
        ImTextureID textureId = 0;  // icon texture (0 = use text fallback)
        float iconWidth = 32, iconHeight = 32;
    };

    void pickUp(const CursorItem& item);     // Pick up item (first click)
    CursorItem dropItem();                   // Returns + clears held item
    void cancel();                           // Cancel (right-click / Escape)
    bool isHolding() const;                  // Carrying anything?
    bool isHolding(const std::string& type) const; // Carrying specific type?
    const CursorItem& cursorItem() const;    // Read current held item
    void renderCursorItem();                 // Draw floating icon at cursor (once/frame)
};
```

Usage:
```cpp
DragDropManager dndMgr;
guiRenderer.setDragDropManager(&dndMgr);
// or: mapRenderer.setDragDropManager(&dndMgr);

// Each frame, after all renderers but before endFrame:
dndMgr.renderCursorItem();
```

DnD on widgets (no new widget types, just properties on any widget):
```cpp
auto slot = WidgetNode::image(tex, 48, 48);
slot.dragType = "item";          // non-empty = drag source
slot.dragData = "sword_01";      // payload
slot.dropAcceptType = "item";    // non-empty = drop target
slot.dragMode = 0;               // 0=both, 1=drag-only, 2=click-only
slot.onDrop = [](WidgetNode& w) { /* w.dragData = delivered payload */ };
slot.onDragBegin = [](WidgetNode& w) { /* drag started */ };
```

## TextureRegistry

Maps texture string names to TextureHandle values. Used by MapRenderer to resolve `ui.image` texture references from scripts.

```cpp
class TextureRegistry {
    void registerTexture(const std::string& name, TextureHandle handle);
    void unregisterTexture(const std::string& name);
    TextureHandle get(const std::string& name) const; // Invalid handle if not found
    bool has(const std::string& name) const;
    void clear();
    size_t size() const;
};
```

Usage:
```cpp
TextureRegistry reg;
reg.registerTexture("sword", swordHandle);
mapRenderer.setTextureRegistry(&reg);
// Script: {ui.image "sword" 48 48}
```

## MapRenderer

Alternative to GuiRenderer -- renders directly from finescript map data (no WidgetNode conversion). Used internally by ScriptGui. Maps ARE the widget data (shared_ptr semantics via finescript MapData), so mutations from script or C++ are visible immediately to the renderer.

Window animation fields on maps: `:alpha`, `:window_pos_x`, `:window_pos_y`, `:scale_x`, `:scale_y`, `:rotation_y`. Same semantics as WidgetNode fields (defaults: alpha=1.0, pos=FLT_MAX for auto, scale=1.0, rotation=0.0).

Window control symbols on maps: `:window_size_w` (programmatic width, 0=auto), `:window_size_h` (programmatic height, 0=auto).

Window flag symbols: `:no_nav` (NoNav), `:no_inputs` (NoInputs). Used in the `:flags` field of window maps alongside existing flag symbols.

```cpp
class MapRenderer {
    explicit MapRenderer(finescript::ScriptEngine& engine);

    int show(finescript::Value rootMap, finescript::ExecutionContext& ctx);
    void hide(int id);
    void hideAll();
    finescript::Value* get(int id);            // nullptr if not found
    void renderAll();                          // Call between beginFrame/endFrame
    void setDragDropManager(DragDropManager* mgr);
    void setTextureRegistry(TextureRegistry* registry);
    const ConverterSymbols& syms() const;      // Pre-interned symbols
    void setFocus(const std::string& widgetId); // Programmatic focus by widget ID
    finescript::Value findById(const std::string& widgetId); // Find by :id string (matches symbol/string fields)
    finescript::Value findById(uint32_t symbolId);           // Find by :id symbol (matches symbol/string fields)
};
```

Usage:
```cpp
MapRenderer mapRenderer(engine);
mapRenderer.setTextureRegistry(&textureRegistry);

// Show a map tree (built by ui.* or manually)
auto tree = /* finescript Value map */;
int id = mapRenderer.show(tree, executionCtx);

// Each frame:
gui.beginFrame();
mapRenderer.renderAll();
gui.endFrame();

// Direct mutation (shared_ptr semantics):
auto* root = mapRenderer.get(id);
// Changes to root's MapData are visible next frame
```

## ScriptGui (Script Integration)

```cpp
class ScriptGui {
    ScriptGui(finescript::ScriptEngine& engine, MapRenderer& renderer);
    ~ScriptGui();
    ScriptGui(ScriptGui&&) noexcept;

    // Load and run script. Pre-binds variables. Returns true on success.
    bool loadAndRun(string_view source, string_view name = "<gui>",
                    const vector<pair<string, Value>>& bindings = {});

    // Run pre-compiled script
    bool run(const CompiledScript& script, const vector<pair<string, Value>>& bindings = {});

    // Message delivery (synchronous, GUI thread)
    bool deliverMessage(uint32_t messageType, Value data);

    // Thread-safe queue
    void queueMessage(uint32_t messageType, Value data);

    // Drain queue (call once/frame on GUI thread)
    void processPendingMessages();

    void close();                    // Remove tree, deactivate
    bool isActive() const;
    int guiId() const;               // MapRenderer ID (-1 if not showing)
    Value* mapTree();                // Direct access to map tree (for MapRenderer path)
    Value navigateMap(int guiId, const Value& path); // Navigate to child map by path
    ExecutionContext* context();      // Script variable access
    const string& lastError() const;

    // Internal (called by script bindings via ctx.userData())
    Value scriptShow(const Value& map);
    void scriptHide();
    void scriptSetFocus(const std::string& widgetId);
    void registerMessageHandler(uint32_t messageType, Value handler);
};
```

## ScriptGuiManager

```cpp
class ScriptGuiManager {
    ScriptGuiManager(ScriptEngine& engine, MapRenderer& renderer);
    ~ScriptGuiManager(); // Calls closeAll()

    // Create and run. Returns nullptr on failure.
    ScriptGui* showFromSource(string_view source, string_view name = "<gui>",
                              const vector<pair<string, Value>>& bindings = {});

    bool deliverMessage(int guiId, uint32_t messageType, Value data);
    void broadcastMessage(uint32_t messageType, Value data);  // All active GUIs
    void queueBroadcast(uint32_t messageType, Value data);    // Thread-safe

    void processPendingMessages(); // Drains broadcast queue + per-GUI queues
    void close(int guiId);
    void closeAll();
    void cleanup();                // Remove inactive GUIs from list
    ScriptGui* findByGuiId(int guiId);
    size_t activeCount() const;
};
```

## Script Bindings

```cpp
void registerGuiBindings(ScriptEngine& engine);
```

Registers `ui` and `gui` as constant map objects on the engine.

### ui.* Builder Functions (return widget maps)

| Function | Script Syntax | Notes |
|----------|---------------|-------|
| `ui.window` | `ui.window "Title" [children]` | Supports `:window_size_w`/`:window_size_h` map keys for programmatic sizing |
| `ui.text` | `ui.text "content"` | |
| `ui.button` | `ui.button "label" [on_click]` | on_click: `fn [] do ... end` |
| `ui.checkbox` | `ui.checkbox "label" value [on_change]` | on_change receives bool |
| `ui.slider` | `ui.slider "label" value min max [on_change]` | on_change receives float |
| `ui.slider_int` | `ui.slider_int "label" value min max [on_change]` | on_change receives int |
| `ui.input` | `ui.input "label" value [on_change] [on_submit]` | |
| `ui.input_int` | `ui.input_int "label" value [on_change]` | |
| `ui.input_float` | `ui.input_float "label" value [on_change]` | |
| `ui.combo` | `ui.combo "label" [items] selected [on_change]` | on_change receives int index |
| `ui.separator` | `ui.separator` | Zero-arg call in `{}` |
| `ui.group` | `ui.group [children]` | |
| `ui.columns` | `ui.columns count [children]` | |
| `ui.image` | `ui.image "texture_name" [w] [h] [on_click]` | Resolves via TextureRegistry |
| `ui.same_line` | `ui.same_line [offset]` | |
| `ui.spacing` | `ui.spacing` | |
| `ui.text_colored` | `ui.text_colored [r g b a] "text"` | Color as array |
| `ui.text_wrapped` | `ui.text_wrapped "text"` | |
| `ui.text_disabled` | `ui.text_disabled "text"` | |
| `ui.progress_bar` | `ui.progress_bar fraction` | |
| `ui.collapsing_header` | `ui.collapsing_header "label" [children]` | |
| `ui.tab_bar` | `ui.tab_bar "id" [children]` | |
| `ui.tab` | `ui.tab "label" [children]` | |
| `ui.tree_node` | `ui.tree_node "label" [children]` | |
| `ui.child` | `ui.child "id" [children]` | |
| `ui.menu_bar` | `ui.menu_bar [children]` | |
| `ui.menu` | `ui.menu "label" [children]` | |
| `ui.menu_item` | `ui.menu_item "label" [on_click]` | |
| `ui.table` | `ui.table "id" num_columns [children]` | |
| `ui.table_row` | `ui.table_row [children]` | |
| `ui.table_next_column` | `ui.table_next_column` | |
| `ui.color_edit` | `ui.color_edit "label" [r g b a] [on_change]` | Color as array |
| `ui.color_picker` | `ui.color_picker "label" [r g b a] [on_change]` | Color as array |
| `ui.drag_float` | `ui.drag_float "label" val speed min max [on_change]` | |
| `ui.drag_int` | `ui.drag_int "label" val speed min max [on_change]` | |
| `ui.drag_float3` | `ui.drag_float3 "label" [x y z] speed min max [on_change]` | 3-component float vector |
| `ui.input_with_hint` | `ui.input_with_hint "label" "hint" "value" [on_change] [on_submit]` | Text input with placeholder |
| `ui.slider_angle` | `ui.slider_angle "label" value_rad min_deg max_deg [on_change]` | Radians stored, degrees displayed |
| `ui.small_button` | `ui.small_button "label" [on_click]` | Compact button variant |
| `ui.color_button` | `ui.color_button "label" [r g b a] [on_click]` | Color swatch display |
| `ui.listbox` | `ui.listbox "label" [items] [selected] [height] [on_change]` | |
| `ui.popup` | `ui.popup "id" [children]` | |
| `ui.modal` | `ui.modal "title" [children] [on_close]` | |
| `ui.open_popup` | `ui.open_popup popup_map` | Sets `:value` to true on map |
| `ui.close_popup` | `ui.close_popup` | Closes innermost open popup (call during popup rendering) |
| `ui.canvas` | `ui.canvas "id" w h [commands]` | commands: array of draw_* |
| `ui.tooltip` | `ui.tooltip "text"` or `ui.tooltip [children]` | |
| `ui.radio_button` | `ui.radio_button "label" value my_value [on_change]` | |
| `ui.selectable` | `ui.selectable "label" [selected] [on_click]` | |
| `ui.input_multiline` | `ui.input_multiline "label" value [w] [h] [on_change]` | |
| `ui.bullet_text` | `ui.bullet_text "text"` | |
| `ui.separator_text` | `ui.separator_text "label"` | |
| `ui.indent` | `ui.indent [amount]` | |
| `ui.unindent` | `ui.unindent [amount]` | |
| `ui.dummy` | `ui.dummy width height` | Invisible spacer with explicit size |
| `ui.new_line` | `ui.new_line` | Force line break |
| `ui.push_style_color` | `ui.push_style_color col_idx [r g b a]` | Push color override |
| `ui.pop_style_color` | `ui.pop_style_color [count]` | Pop color overrides |
| `ui.push_style_var` | `ui.push_style_var var_idx val` or `var_idx [x y]` | Push style var |
| `ui.pop_style_var` | `ui.pop_style_var [count]` | Pop style var overrides |
| `ui.context_menu` | `ui.context_menu [children]` | Right-click context menu; place after target widget in children list |
| `ui.main_menu_bar` | `ui.main_menu_bar [children]` | Top-level app menu bar; must be a top-level tree (not inside a window) |
| `ui.item_tooltip` | `ui.item_tooltip "text"` or `ui.item_tooltip [children]` | Hover tooltip on previous widget |
| `ui.image_button` | `ui.image_button "id" "texture_name" [w] [h] [on_click]` | Clickable image button |
| `ui.plot_lines` | `ui.plot_lines "label" [values] "overlay" min max width height` | Line graph from array of floats |
| `ui.plot_histogram` | `ui.plot_histogram "label" [values] "overlay" min max width height` | Histogram from array of floats |
| `ui.push_theme` | `ui.push_theme "name"` | Push named theme preset (see Style & Theming) |
| `ui.pop_theme` | `ui.pop_theme "name"` | Pop named theme preset (must match push) |

### Named Arguments (Keyword-Style Parameters)

All `ui.*` builders support **named arguments** (`=name value` syntax). Named args are merged into the widget map, overriding positional args for the same field:

```
# Positional only:
{ui.slider "Volume" 0.5 0.0 1.0}

# Named arguments (recommended):
{ui.slider "Volume" =value 0.5 =min 0.0 =max 1.0}

# Mix positional + named:
{ui.button "Save" =id "save_btn" =on_click fn [] do save() end}
```

An explicit options map as last positional arg also works: `{ui.slider "Volume" {=value 0.5}}`. Both forms are equivalent; prefer no-braces.

Useful fields: `:id`, `:enabled`, `:visible`, `:on_click`, `:on_change`, `:on_submit`, `:on_close`, `:drag_type`, `:drag_data`, `:drop_accept`, `:focusable`, `:window_flags`.

Named callbacks (recommended style):
```
{ui.slider "Volume" =value 0.5 =min 0.0 =max 1.0 =on_change fn [v] do set volume v end}
```

### String Interpolation in Widget Text

finescript supports string interpolation with `{expr}` in all widget string fields (labels, titles, text content):

```
set hp 42
ui.text "HP: {hp}"                  # produces "HP: 42"
ui.text "Result: {add 3 4}"         # produces "Result: 7"
ui.text "Name: {player.name}"       # any expression works
ui.text "Literal brace: \{ \}"      # escape with backslash
```

### Canvas Draw Commands (returned by ui.draw_*)

| Function | Script Syntax | Notes |
|----------|---------------|-------|
| `ui.draw_line` | `ui.draw_line [x1 y1] [x2 y2] [r g b a] [thickness]` | |
| `ui.draw_rect` | `ui.draw_rect [x1 y1] [x2 y2] [r g b a] [filled] [thickness]` | |
| `ui.draw_circle` | `ui.draw_circle [cx cy] radius [r g b a] [filled] [thickness]` | |
| `ui.draw_text` | `ui.draw_text [x y] "text" [r g b a]` | |
| `ui.draw_triangle` | `ui.draw_triangle [x1 y1] [x2 y2] [x3 y3] [r g b a] [filled] [thickness]` | |

### ui.* Action Functions (require ScriptGui context)

| Function | Script Syntax | Description |
|----------|---------------|-------------|
| `ui.show` | `ui.show widget_map` | Store map in MapRenderer, returns ID |
| `ui.update` | `ui.update id widget_map` | Replace an existing tree |
| `ui.hide` | `ui.hide` | Remove tree from renderer |
| `ui.node` | `ui.node gui_id [child_path]` | Navigate map tree, return child map |
| `ui.find` | `ui.find "id"` or `ui.find :id` | Find widget map by `:id` string or symbol (nil if not found) |
| `ui.set_text` | `ui.set_text id child_index text` | Mutate text content of a child node |
| `ui.set_value` | `ui.set_value id child_index value` | Mutate value field of a child node |
| `ui.set_label` | `ui.set_label id child_index label` | Mutate label of a child node |
| `ui.save_state` | `ui.save_state` | Save all widget states (by `:id`) as a map |
| `ui.load_state` | `ui.load_state state_map` | Restore widget states from a previously saved map |
| `ui.open_popup` | `ui.open_popup popup_map` | Open a popup/modal (sets `:value` to true on the map) |
| `ui.close_popup` | `ui.close_popup` | Close the innermost open popup |
| `ui.set_theme` | `ui.set_theme "dark"` / `"light"` / `"classic"` | Switch global ImGui theme (immediate, not a widget) |

`ui.node` path: integer (direct child index) or array of integers (nested path, e.g., `[2 0]`). No path returns root map.

### gui.* Functions

| Function | Script Syntax | Description |
|----------|---------------|-------------|
| `gui.on_message` | `gui.on_message :symbol handler` | Register message handler |
| `gui.set_focus` | `gui.set_focus "widget_id"` | Programmatically focus a widget |
| `gui.bind_key` | `gui.bind_key "chord" callback` | Bind keyboard shortcut, returns binding ID |
| `gui.unbind_key` | `gui.unbind_key id` | Unbind keyboard shortcut by ID |
| `gui.open_popup` | `gui.open_popup "id"` | Open a popup by string ID |
| `gui.close_popup` | `gui.close_popup` | Close current popup |

## Map-Based Mutation (Preferred)

Widget data is stored as finescript maps with shared_ptr semantics. Direct mutation is the **preferred** approach for updating widgets — changes are visible immediately to the renderer without calling mutation functions.

```
# Hold references to widgets you want to update later
set count 0
set count_text {ui.text "Count: 0"}
set gui_id {ui.show {ui.window "Counter" [
    count_text
    {ui.button "+" fn [] do
        set count (count + 1)
        set count_text.text ("Count: " + {to_str count})
    end}
]}}

# Navigation: get child maps by index (alternative to holding references)
set child {ui.node gui_id 0}         # First child of root
set nested {ui.node gui_id [2 0]}    # children[2].children[0]
```

Index-based mutation (`ui.set_text`, `ui.set_value`, `ui.set_label`) also works but is more fragile (breaks if tree structure changes). Full tree rebuild with `ui.update` is available for major layout changes but loses callbacks.

## Script Example

```
set count 0
set gui_id {ui.show {ui.window "Counter" [
    {ui.text "Count: 0"}
    {ui.button "Increment" fn [] do
        set count (count + 1)
        set child {ui.node gui_id 0}
        set child.text ("Count: " + {to_str count})
    end}
    {ui.button "Reset" fn [] do
        set count 0
        set child {ui.node gui_id 0}
        set child.text "Count: 0"
    end}
]}}

gui.on_message :reset fn [data] do
    set count 0
    set child {ui.node gui_id 0}
    set child.text "Count: 0"
end
```

Uses `ui.node` to get the live text map (child 0) and mutates `.text` directly (shared_ptr semantics).

## Script + C++ Integration Pattern

```cpp
// Setup
finescript::ScriptEngine engine;
finegui::registerGuiBindings(engine);
finegui::MapRenderer mapRenderer(engine);
finegui::TextureRegistry texRegistry;
finegui::DragDropManager dndMgr;
mapRenderer.setTextureRegistry(&texRegistry);
mapRenderer.setDragDropManager(&dndMgr);
finegui::ScriptGuiManager mgr(engine, mapRenderer);

// Register textures for scripts
texRegistry.registerTexture("sword", gui.registerTexture(swordTex));

// Launch scripts
auto* scriptGui = mgr.showFromSource(source, "name");

// C++ retained-mode alongside scripts (use GuiRenderer for WidgetNode trees)
finegui::GuiRenderer guiRenderer(gui);
guiRenderer.show(WidgetNode::window("C++ Panel", { ... }));

// Frame loop
gui.beginFrame();
mgr.processPendingMessages();
mapRenderer.renderAll();    // Renders script-driven maps
guiRenderer.renderAll();    // Renders C++ WidgetNode trees
dndMgr.renderCursorItem();  // Draw floating DnD icon
gui.endFrame();

// C++ -> script messaging
scriptGui->deliverMessage(engine.intern("event_name"), Value::string("data"));

// Read script state
auto val = scriptGui->context()->get("variable_name");

// Thread-safe messaging
scriptGui->queueMessage(engine.intern("event"), Value::nil());
mgr.queueBroadcast(engine.intern("global_event"), Value::nil());
```

## TweenManager (Animation)

Smooth property interpolation for retained-mode widgets. Animates WidgetNode fields over time with configurable easing. Works with GuiRenderer only (not MapRenderer).

```cpp
class TweenManager {
    explicit TweenManager(GuiRenderer& renderer);

    void update(float dt);  // Call each frame before renderAll()

    // Generic: animate property (reads current "from" automatically)
    int animate(int guiId, std::vector<int> childPath,
                TweenProperty prop, float toValue,
                float duration, Easing easing = Easing::EaseOut,
                TweenCallback onComplete = {});

    // Generic: explicit from/to
    int animate(int guiId, std::vector<int> childPath,
                TweenProperty prop, float fromValue, float toValue,
                float duration, Easing easing = Easing::EaseOut,
                TweenCallback onComplete = {});

    // Convenience
    int fadeIn(int guiId, float duration = 0.3f, Easing = EaseOut, TweenCallback = {});
    int fadeOut(int guiId, float duration = 0.3f, Easing = EaseIn, TweenCallback = {});
    int slideTo(int guiId, float x, float y, float duration = 0.4f, Easing = EaseOut, TweenCallback = {});
    int colorTo(int guiId, std::vector<int> childPath, float r, float g, float b, float a,
                float duration = 0.3f, Easing = EaseOut, TweenCallback = {});
    int zoomIn(int guiId, float duration = 0.3f, Easing = EaseOut, TweenCallback = {});
    int zoomOut(int guiId, float duration = 0.3f, Easing = EaseIn, TweenCallback = {});
    int flipY(int guiId, float duration = 0.5f, Easing = EaseInOut, TweenCallback = {});
    int flipYBack(int guiId, float duration = 0.5f, Easing = EaseInOut, TweenCallback = {});
    int shake(int guiId, float duration = 0.4f, float amplitude = 8.0f, float frequency = 15.0f,
              TweenCallback = {});

    // Cancellation
    void cancel(int tweenId);
    void cancelAll(int guiId);
    void cancelAll();
    bool isActive(int tweenId) const;
    int activeCount() const;
};
```

Easing: `Linear`, `EaseIn` (quad), `EaseOut` (quad), `EaseInOut` (quad), `CubicOut`, `ElasticOut`, `BounceOut`.

TweenProperty: `Alpha`, `PosX`, `PosY`, `FloatValue`, `IntValue`, `ColorR`, `ColorG`, `ColorB`, `ColorA`, `Width`, `Height`, `ScaleX`, `ScaleY`, `RotationY`.

`TweenCallback` = `std::function<void(int tweenId)>`.

Usage:
```cpp
TweenManager tweens(guiRenderer);

// Game loop:
gui.beginFrame();
tweens.update(ImGui::GetIO().DeltaTime);
guiRenderer.renderAll();
gui.endFrame();

// Fade in a window
tweens.fadeIn(guiId, 0.3f);

// Slide window to position
tweens.slideTo(guiId, 300.0f, 400.0f, 0.4f);

// Zoom in (scale 0 -> 1, window appears from center)
tweens.zoomIn(guiId, 0.3f);

// Zoom out (scale 1 -> 0, window collapses to center)
tweens.zoomOut(guiId, 0.3f, Easing::EaseIn, [&](int) { guiRenderer.hide(guiId); });

// Flip around Y-axis (0 -> PI, shows back side)
tweens.flipY(guiId, 0.5f);

// Flip back (PI -> 0, shows front side)
tweens.flipYBack(guiId, 0.5f);

// Shake effect
tweens.shake(guiId, 0.4f, 8.0f, 15.0f);

// Target child widget by index path: children[2].children[0]
tweens.animate(guiId, {2, 0}, TweenProperty::FloatValue, 1.0f, 0.5f);
```

## Style & Theming

### Named Theme Presets (push_theme / pop_theme)

Push/pop scoped style color overrides by preset name. Must be paired with matching names. Affects all widgets between push and pop.

Widget types: `PushTheme`, `PopTheme`

Builder:
```cpp
static WidgetNode pushTheme(std::string name);
static WidgetNode popTheme(std::string name);
```

Script:
```
ui.push_theme "danger"
ui.pop_theme "danger"
```

Map: `{:type :push_theme :label "danger"}`, `{:type :pop_theme :label "danger"}`

Available presets:

| Preset | Colors pushed | Description |
|--------|--------------|-------------|
| `"danger"` | 4 (Button, ButtonHovered, ButtonActive, Text) | Red buttons with light text |
| `"success"` | 4 (Button, ButtonHovered, ButtonActive, Text) | Green buttons with light text |
| `"warning"` | 4 (Button, ButtonHovered, ButtonActive, Text) | Orange buttons with light text |
| `"info"` | 4 (Button, ButtonHovered, ButtonActive, Text) | Blue buttons with light text |
| `"dark"` | 3 (WindowBg, FrameBg, Text) | Dark background with light text |
| `"light"` | 3 (WindowBg, FrameBg, Text) | Light background with dark text |

Unknown preset names push nothing (no-op). Pop with an unknown name also does nothing.

Usage (retained):
```cpp
auto win = WidgetNode::window("Themed", {
    WidgetNode::pushTheme("danger"),
    WidgetNode::button("Delete", [](auto&) { /* ... */ }),
    WidgetNode::popTheme("danger"),
    WidgetNode::pushTheme("success"),
    WidgetNode::button("Confirm", [](auto&) { /* ... */ }),
    WidgetNode::popTheme("success"),
});
```

Usage (script):
```
ui.show {ui.window "Actions" [
    {ui.push_theme "danger"}
    {ui.button "Delete" fn [] do ... end}
    {ui.pop_theme "danger"}
    {ui.push_theme "success"}
    {ui.button "Save" fn [] do ... end}
    {ui.pop_theme "success"}
]}
```

### Textured Skin System (Planned)

**Not yet implemented.** Full design: [SKIN_SYSTEM_PLAN.md](SKIN_SYSTEM_PLAN.md)

Replaces ImGui flat-color rendering with 9-slice textured draws from a sprite atlas. Opt-in, non-breaking. Key future API surface:

```cpp
// C++
SkinAtlas atlas;
atlas.loadTexture(gui, "atlas.png");
auto skin = Skin::loadFromFile("skin.fgs", atlas, engine);
SkinManager skinMgr;
skinMgr.registerSkin("name", std::move(skin));
skinMgr.setActiveSkin("name");
guiRenderer.setSkinManager(&skinMgr);  // or mapRenderer.setSkinManager(&skinMgr)
```

Script: `ui.load_skin "name" "path"`, `ui.set_skin "name"`, `ui.push_skin "name"`, `ui.pop_skin`.
Per-widget: `:skin "custom_element"` map field or `skinOverride` string on WidgetNode.

Unskinned elements fall back to standard ImGui rendering. Skins can be partial.

New widget types (future): `PushSkin`, `PopSkin`. New WidgetNode field: `skinOverride`.

### Global Theme Switching (set_theme)

Immediate action (not a widget) -- calls `ImGui::StyleColorsDark()`, `ImGui::StyleColorsLight()`, or `ImGui::StyleColorsClassic()`. Changes apply to the entire ImGui context. Script-only (no WidgetNode equivalent).

```
ui.set_theme "dark"
ui.set_theme "light"
ui.set_theme "classic"
```

Returns nil. Unknown theme names are ignored.

## State Serialization

Save/load widget values by ID. Only widgets with explicit `:id` fields are included.

### Retained Mode

```cpp
using WidgetStateMap = std::map<std::string, std::variant<bool, int, double, std::string, std::vector<float>>>;

WidgetStateMap state = renderer.saveState(guiId);   // Snapshot current values
renderer.loadState(guiId, state);                    // Restore values
```

### Script Mode

```
set state {ui.save_state}       # Returns finescript map of id->value
ui.load_state state              # Restore from map
```

`MapRenderer::serializeState(state, interner)` converts a saved state map to a parseable finescript string using `{=key value}` syntax.

### State Fields by Widget Type

| Widget | State type | Notes |
|--------|-----------|-------|
| `checkbox`, `selectable` | `bool` | |
| `slider`, `drag_float`, `slider_angle` | `float` | |
| `input_int`, `drag_int` | `int` | |
| `combo`, `listbox` | `int` | Selected index |
| `radio_button` | `int` | Active index |
| `input_text`, `input_multiline`, `input_with_hint` | `string` | |
| `color_edit`, `color_picker` | `[r,g,b,a]` float array | |
| `drag_float3` | `[x,y,z]` float array | |

## Keyboard Shortcuts (HotkeyManager)

`#include <finegui/hotkey_manager.hpp>`

Global keyboard shortcut manager. Binds key chords (modifier+key combos) to callbacks. Call `update()` each frame between `beginFrame()`/`endFrame()`.

```cpp
class HotkeyManager {
    HotkeyManager() = default;
    void update();  // Call each frame (checks ImGui input state)

    // Bind chord to callback. Returns unique ID. flags: ImGuiInputFlags_RouteGlobal (default), etc.
    int bind(ImGuiKeyChord chord, HotkeyCallback callback,
             ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal);
    void unbind(int id);
    void unbindChord(ImGuiKeyChord chord);  // Remove all bindings for chord
    void unbindAll();

    void setEnabled(int id, bool enabled);    // Per-binding enable/disable
    void setGlobalEnabled(bool enabled);      // Master enable/disable

    static ImGuiKeyChord parseChord(const std::string& str);  // "ctrl+s" -> ImGuiKeyChord
    static std::string formatChord(ImGuiKeyChord chord);       // ImGuiKeyChord -> "Ctrl+S"
};
```

`HotkeyCallback` = `std::function<void()>`.

Chord string format: modifiers (`ctrl`, `shift`, `alt`, `super`/`cmd`) + key name joined with `+`. Case-insensitive. Key names: `a`-`z`, `0`-`9`, `f1`-`f24`, `escape`, `enter`, `space`, `tab`, `backspace`, `delete`, `insert`, `home`, `end`, `pageup`, `pagedown`, `up`, `down`, `left`, `right`, etc.

Usage:
```cpp
HotkeyManager hotkeys;
int id = hotkeys.bind(HotkeyManager::parseChord("ctrl+s"), [&]() { save(); });
hotkeys.bind(HotkeyManager::parseChord("ctrl+shift+z"), [&]() { redo(); });

// Frame loop
gui.beginFrame();
hotkeys.update();
/* widgets */
gui.endFrame();

// Later
hotkeys.setEnabled(id, false);  // Temporarily disable
hotkeys.unbind(id);             // Remove
```

### Script Integration

Call `setHotkeyManager(&hotkeys)` on ScriptGui to enable `gui.bind_key` / `gui.unbind_key`:

```cpp
ScriptGui scriptGui(engine, mapRenderer);
scriptGui.setHotkeyManager(&hotkeys);
```

Script:
```
set save_id {gui.bind_key "ctrl+s" fn [] do
    # save logic
end}
gui.unbind_key save_id
```

`gui.bind_key` returns an integer binding ID. `gui.unbind_key` takes that ID.

## Widget Converter (Advanced)

For direct use outside of ScriptGui (e.g., custom script integration):

```cpp
ConverterSymbols syms;
syms.intern(engine);

// Convert finescript map -> WidgetNode
WidgetNode tree = convertToWidget(scriptMap, engine, ctx, syms);

// Get widget's current value as finescript Value
Value val = widgetValueToScriptValue(widgetNode);
// Checkbox -> bool, Slider -> float, SliderInt/InputInt -> int,
// InputText -> string, Combo -> int (selectedIndex)
```
