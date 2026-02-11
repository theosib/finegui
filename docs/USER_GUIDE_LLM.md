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
- Animation fields: `alpha`, `windowPosX`, `windowPosY` on WidgetNode (FLT_MAX = auto-position)
- TweenManager: call `update(dt)` before `renderAll()` each frame

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
        // Phase 7 - Misc
        ListBox, Popup, Modal,
        // Phase 8 - Custom
        Canvas, Tooltip,
        // Phase 9 - New widgets
        RadioButton, Selectable, InputTextMultiline,
        BulletText, SeparatorText, Indent,
        // Phase 10 - Style push/pop
        PushStyleColor, PopStyleColor, PushStyleVar, PopStyleVar
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
    float dragSpeed = 1.0f;                  // DragFloat, DragInt

    // Phase 7
    int heightInItems = -1;                  // ListBox (-1 = auto)

    // Phase 8
    WidgetCallback onDraw;                   // Canvas custom draw

    // Phase 9
    int windowFlags = 0;                     // ImGuiWindowFlags_*

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

    // --- Static builders (Phase 1) ---
    static WidgetNode window(string title, vector<WidgetNode> children = {}, int flags = 0);
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

    // --- Phase 7 builders ---
    static WidgetNode listBox(string label, vector<string> items, int sel=0, int height=-1,
                              WidgetCallback onChange={});
    static WidgetNode popup(string id, vector<WidgetNode> children={});
    static WidgetNode modal(string title, vector<WidgetNode> children={}, WidgetCallback onClose={});

    // --- Phase 8 builders ---
    static WidgetNode canvas(string id, float w, float h, WidgetCallback onDraw={}, WidgetCallback onClick={});
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

    // --- Phase 10 builders ---
    static WidgetNode pushStyleColor(int colIdx, float r, float g, float b, float a);
    static WidgetNode popStyleColor(int count=1);
    static WidgetNode pushStyleVar(int varIdx, float val);
    static WidgetNode pushStyleVar(int varIdx, float x, float y);
    static WidgetNode popStyleVar(int count=1);
};
```

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
| `ui.window` | `ui.window "Title" [children]` | |
| `ui.text` | `ui.text "content"` | |
| `ui.button` | `ui.button "label" [on_click]` | on_click: `fn [] do ... end` |
| `ui.checkbox` | `ui.checkbox "label" value [on_change]` | on_change receives bool |
| `ui.slider` | `ui.slider "label" min max value [on_change]` | on_change receives float |
| `ui.slider_int` | `ui.slider_int "label" min max value [on_change]` | on_change receives int |
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
| `ui.listbox` | `ui.listbox "label" [items] [selected] [height] [on_change]` | |
| `ui.popup` | `ui.popup "id" [children]` | |
| `ui.modal` | `ui.modal "title" [children] [on_close]` | |
| `ui.open_popup` | `ui.open_popup popup_map` | Sets `:value` to true on map |
| `ui.canvas` | `ui.canvas "id" w h [commands]` | commands: array of draw_* |
| `ui.tooltip` | `ui.tooltip "text"` or `ui.tooltip [children]` | |
| `ui.radio_button` | `ui.radio_button "label" value my_value [on_change]` | |
| `ui.selectable` | `ui.selectable "label" [selected] [on_click]` | |
| `ui.input_multiline` | `ui.input_multiline "label" value [w] [h] [on_change]` | |
| `ui.bullet_text` | `ui.bullet_text "text"` | |
| `ui.separator_text` | `ui.separator_text "label"` | |
| `ui.indent` | `ui.indent [amount]` | |
| `ui.unindent` | `ui.unindent [amount]` | |
| `ui.push_style_color` | `ui.push_style_color col_idx [r g b a]` | Push color override |
| `ui.pop_style_color` | `ui.pop_style_color [count]` | Pop color overrides |
| `ui.push_style_var` | `ui.push_style_var var_idx val` or `var_idx [x y]` | Push style var |
| `ui.pop_style_var` | `ui.pop_style_var [count]` | Pop style var overrides |

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
| `ui.hide` | `ui.hide` | Remove tree from renderer |
| `ui.node` | `ui.node gui_id [child_path]` | Navigate map tree, return child map |
| `ui.find` | `ui.find "id"` or `ui.find :id` | Find widget map by `:id` string or symbol (nil if not found) |

`ui.node` path: integer (direct child index) or array of integers (nested path, e.g., `[2 0]`). No path returns root map.

### gui.* Functions

| Function | Script Syntax | Description |
|----------|---------------|-------------|
| `gui.on_message` | `gui.on_message :symbol handler` | Register message handler |
| `gui.set_focus` | `gui.set_focus "widget_id"` | Programmatically focus a widget |

## Map-Based Mutation

With MapRenderer, widget data is stored as finescript maps with shared_ptr semantics. Mutations to maps are visible immediately to the renderer (no `ui.update` needed).

```
# Build and show
set text_widget {ui.text "Initial"}
set gui_id {ui.show {ui.window "W" [text_widget]}}

# Direct mutation -- changes are visible next frame
set text_widget.text "Updated!"

# Navigation: get child maps by index
set child {ui.node gui_id 0}         # First child of root
set nested {ui.node gui_id [2 0]}    # children[2].children[0]
```

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

TweenProperty: `Alpha`, `PosX`, `PosY`, `FloatValue`, `IntValue`, `ColorR`, `ColorG`, `ColorB`, `ColorA`, `Width`, `Height`.

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

// Shake effect
tweens.shake(guiId, 0.4f, 8.0f, 15.0f);

// Target child widget by index path: children[2].children[0]
tweens.animate(guiId, {2, 0}, TweenProperty::FloatValue, 1.0f, 0.5f);
```

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
