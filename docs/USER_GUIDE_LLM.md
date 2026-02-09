# finegui LLM Reference

Dense API reference for finegui (Dear ImGui + finevk Vulkan backend). Optimized for LLM context windows.

## Build

Three layered libraries, each static + shared:
- `finegui` — Core (immediate mode, finevk backend)
- `finegui-retained` — Retained-mode widgets (`FINEGUI_BUILD_RETAINED=ON`)
- `finegui-script` — Script integration (`FINEGUI_BUILD_SCRIPT=ON`, requires finescript)

Tests link shared. Examples link static.

## Architecture

finegui wraps Dear ImGui with a Vulkan backend using finevk. Three levels:
1. **Core**: Immediate-mode — call ImGui widget functions each frame
2. **Retained**: Declarative `WidgetNode` trees rendered by `GuiRenderer`
3. **Script**: finescript scripts build widget trees via `ui.*` functions

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
- ScriptEngine must outlive Vulkan resources (declare it first)
- ScriptGui::close() removes tree before destroying context (closure safety)

## WidgetNode (Retained Mode)

```cpp
using WidgetCallback = std::function<void(WidgetNode& widget)>;

struct WidgetNode {
    enum class Type {
        Window, Text, Button, Checkbox, Slider, SliderInt,
        InputText, InputInt, InputFloat, Combo, Separator,
        Group, Columns, Image,
        // Future: TabBar, TreeNode, MenuBar, Table, etc.
    };

    Type type;
    std::string label, textContent, id;
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    std::string stringValue;
    int selectedIndex = -1;
    float minFloat = 0.0f, maxFloat = 1.0f;
    int minInt = 0, maxInt = 100;
    float width = 0.0f, height = 0.0f;
    int columnCount = 1;
    std::vector<std::string> items;
    std::vector<WidgetNode> children;
    bool visible = true, enabled = true;
    WidgetCallback onClick, onChange, onSubmit, onClose;
    TextureHandle texture{};
    float imageWidth = 0.0f, imageHeight = 0.0f;

    // Static builders
    static WidgetNode window(string title, vector<WidgetNode> children = {});
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

// Remove:
renderer.hide(id);
```

## ScriptGui (Script Integration)

```cpp
class ScriptGui {
    ScriptGui(finescript::ScriptEngine& engine, GuiRenderer& renderer);
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
    int guiId() const;               // GuiRenderer ID (-1 if not showing)
    WidgetNode* widgetTree();        // Direct access to tree
    ExecutionContext* context();      // Script variable access
    const string& lastError() const;

    // Internal (called by script bindings via ctx.userData())
    Value scriptShow(const Value& map, ScriptEngine& engine, ExecutionContext& ctx);
    void scriptUpdate(int id, const Value& map, ScriptEngine& engine, ExecutionContext& ctx);
    bool scriptSetText(int id, const Value& path, const string& text);
    bool scriptSetValue(int id, const Value& path, const Value& value);
    bool scriptSetLabel(int id, const Value& path, const string& label);
    void scriptHide(int id);
    void registerMessageHandler(uint32_t messageType, Value handler);
};
```

## ScriptGuiManager

```cpp
class ScriptGuiManager {
    ScriptGuiManager(ScriptEngine& engine, GuiRenderer& renderer);
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

### ui.* Action Functions (require ScriptGui context)

| Function | Script Syntax | Description |
|----------|---------------|-------------|
| `ui.show` | `ui.show widget_map` | Convert map→tree, show, return ID |
| `ui.update` | `ui.update id widget_map` | Replace existing tree (loses callbacks) |
| `ui.set_text` | `ui.set_text id child_idx text` | Mutate textContent of child node |
| `ui.set_value` | `ui.set_value id child_idx value` | Mutate value (bool/int/float/string) |
| `ui.set_label` | `ui.set_label id child_idx label` | Mutate label of child node |
| `ui.hide` | `ui.hide [id]` | Hide tree (or close GUI if no ID) |

`child_idx` can be an integer (direct child) or array of integers (nested path, e.g., `[0 1]`).

### gui.* Functions

| Function | Script Syntax | Description |
|----------|---------------|-------------|
| `gui.on_message` | `gui.on_message :symbol handler` | Register message handler |

## Script Example

```
set count 0
set gui_id {ui.show {ui.window "Counter" [
    {ui.text "Count: 0"}
    {ui.button "Increment" fn [] do
        set count (count + 1)
        ui.set_text gui_id 0 ("Count: " + {to_str count})
    end}
    {ui.button "Reset" fn [] do
        set count 0
        ui.set_text gui_id 0 "Count: 0"
    end}
]}}

gui.on_message :reset fn [data] do
    set count 0
    ui.set_text gui_id 0 "Count: 0"
end
```

Uses `ui.set_text` to mutate the text node (child 0) directly, preserving button callbacks.

## Script + C++ Integration Pattern

```cpp
// Setup
finescript::ScriptEngine engine;
finegui::registerGuiBindings(engine);
finegui::GuiRenderer guiRenderer(gui);
finegui::ScriptGuiManager mgr(engine, guiRenderer);

// Launch scripts
auto* scriptGui = mgr.showFromSource(source, "name");

// C++ retained-mode alongside scripts
guiRenderer.show(WidgetNode::window("C++ Panel", { ... }));

// Frame loop
gui.beginFrame();
mgr.processPendingMessages();
guiRenderer.renderAll();  // Renders both C++ and script trees
gui.endFrame();

// C++ → script messaging
scriptGui->deliverMessage(engine.intern("event_name"), Value::string("data"));

// Read script state
auto val = scriptGui->context()->get("variable_name");

// Thread-safe messaging
scriptGui->queueMessage(engine.intern("event"), Value::nil());
mgr.queueBroadcast(engine.intern("global_event"), Value::nil());
```

## Widget Converter (Advanced)

For direct use outside of ScriptGui (e.g., custom script integration):

```cpp
ConverterSymbols syms;
syms.intern(engine);

// Convert finescript map → WidgetNode
WidgetNode tree = convertToWidget(scriptMap, engine, ctx, syms);

// Get widget's current value as finescript Value
Value val = widgetValueToScriptValue(widgetNode);
// Checkbox → bool, Slider → float, SliderInt/InputInt → int,
// InputText → string, Combo → int (selectedIndex)
```
