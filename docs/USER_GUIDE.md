# finegui User Guide

finegui is a GUI toolkit built on Dear ImGui with a finevk (Vulkan) backend. It provides immediate-mode GUI widgets for games and applications using finevk for rendering.

The build produces both a **static library** (`libfinegui.a`) and a **shared library** (`libfinegui.dylib`). Link against whichever suits your project.

## Quick Start

```cpp
#include <finegui/finegui.hpp>
#include <finevk/finevk.hpp>

// After creating your finevk device and renderer...

// 1. Configure
finegui::GuiConfig guiConfig;
guiConfig.msaaSamples = renderer->msaaSamples();  // Must match render pass
guiConfig.dpiScale = window->contentScale().x;     // High-DPI support
guiConfig.fontSize = 16.0f;                        // Logical pixels

// 2. Create and initialize
finegui::GuiSystem gui(device.get(), guiConfig);
gui.initialize(renderer.get());  // Or any RenderSurface*

// 3. Use in your game loop
gui.beginFrame();

ImGui::Begin("My Window");
ImGui::Text("Hello from finegui!");
ImGui::End();

gui.endFrame();

// 4. Render inside a render pass
frame.beginRenderPass({0.1f, 0.1f, 0.15f, 1.0f});
// ... your 3D rendering here ...
gui.render(frame);
frame.endRenderPass();
renderer->endFrame();
```

---

## Configuration

`GuiConfig` controls how the GUI system behaves. Set it before constructing `GuiSystem`.

| Field | Default | Description |
|-------|---------|-------------|
| `dpiScale` | `0.0f` | DPI scale factor. 0 = use 1.0. Set to `window->contentScale().x` for high-DPI. |
| `fontScale` | `1.0f` | Additional font size multiplier on top of fontSize. |
| `fontSize` | `16.0f` | Base font size in logical pixels. Automatically rasterized at high resolution on Retina displays via `RasterizerDensity`. |
| `fontPath` | `""` | Path to a TTF font file. Empty = use ImGui's built-in ProggyVector font. |
| `fontData` / `fontDataSize` | `nullptr` / `0` | Alternative: load font from memory. |
| `msaaSamples` | `VK_SAMPLE_COUNT_1_BIT` | MSAA sample count. **Must match your render pass.** |
| `framesInFlight` | `0` | 0 = auto-detect from device. |
| `enableDrawDataCapture` | `false` | Enable for threaded rendering mode. |
| `enableKeyboard` | `true` | ImGui keyboard navigation. |
| `enableGamepad` | `false` | ImGui gamepad navigation. |

### High-DPI Displays

finegui handles high-DPI automatically when you set `dpiScale`:

```cpp
auto contentScale = window->contentScale();
guiConfig.dpiScale = contentScale.x;  // e.g., 2.0 on Retina
```

This configures three things:
- **Display size**: Reported to ImGui in logical (screen) coordinates, not framebuffer pixels
- **Framebuffer scale**: Tells ImGui the pixel density ratio
- **Font rasterization**: Uses `RasterizerDensity` to rasterize fonts at native resolution while displaying at logical size

You don't need to scale font sizes manually.

---

## Input Handling

finegui uses its own `InputEvent` type, decoupled from GLFW/finevk. This enables testing, replay, and network input.

### Converting from finevk InputManager

The simplest approach uses `InputAdapter::fromFineVK()` to convert events one at a time:

```cpp
auto input = finevk::InputManager::create(window.get());

// In your game loop:
input->update();
finevk::InputEvent event;
while (input->pollEvent(event)) {
    gui.processInput(finegui::InputAdapter::fromFineVK(event));
}
```

### InputManager Integration

The simplest way to connect GUI input is via `connectToInputManager()`, which registers the GUI as a prioritized listener:

```cpp
auto input = finevk::InputManager::create(window.get());
gui.connectToInputManager(*input);

// In your game loop — no manual polling needed:
input->update();
// Events are automatically forwarded to the GUI
```

The GUI listener uses `GuiMode` to control input consumption:

| Mode | Behavior |
|------|----------|
| `GuiMode::Auto` (default) | Uses ImGui's `WantCapture*` flags — consumes only when GUI is active |
| `GuiMode::Passive` | Feeds events to ImGui but never blocks game input |
| `GuiMode::Exclusive` | Consumes all input (for menus, inventory screens, etc.) |

```cpp
// Switch to exclusive mode when opening inventory
gui.setGuiMode(finegui::GuiMode::Exclusive);

// Back to auto when closing
gui.setGuiMode(finegui::GuiMode::Auto);
```

### Input Capture

When using manual event polling instead of `connectToInputManager()`, check whether the GUI wants to consume mouse/keyboard input before forwarding to your game:

```cpp
if (!gui.wantCaptureMouse()) {
    // Handle game mouse input (camera look, block breaking, etc.)
}

if (!gui.wantCaptureKeyboard()) {
    // Handle game keyboard input (movement, hotkeys, etc.)
}
```

This prevents clicking a GUI button from also firing a weapon, or typing in a text field from moving the character.

---

## Integrating GUI into a 3D Game Loop

The key insight: **GUI renders last, on top of everything else**, within the same render pass. Here's how it fits into a typical game loop:

### Basic Structure

```
pollEvents()
inputManager->update()
    forward events to gui.processInput()
    forward events to game (camera, movement, etc.)
game logic update (physics, entities, AI)
camera update

gui.beginFrame()
    ImGui widgets (windows, menus, HUD, inventory)
gui.endFrame()

if (auto frame = renderer->beginFrame()) {
    frame.beginRenderPass(clearColor);

    worldRenderer.render(frame);       // 3D world
    entityRenderer.render(frame);      // 3D entities
    particleRenderer.render(frame);    // Particles
    overlay.render(frame);             // 2D overlays (crosshair, minimaps)
    gui.render(frame);                 // GUI on top of everything

    frame.endRenderPass();
    renderer->endFrame();
}
```

### Render Order Within the Same Render Pass

All rendering happens in a single render pass. The order determines what draws on top:

1. **3D world** (terrain, chunks) - drawn first, depth-tested
2. **3D entities** (players, mobs, items) - depth-tested against world
3. **Particles / effects** - may use alpha blending
4. **2D overlays** (crosshair, minimap borders) - drawn without depth test
5. **GUI** (ImGui widgets) - drawn last, always on top

finegui handles its own depth/blend state via its pipeline, so you don't need to worry about state leaking.

### Full Example with Game Systems

```cpp
// Game loop
while (window->isOpen()) {
    window->pollEvents();

    // --- Input Phase ---
    inputManager->update();
    finevk::InputEvent event;
    while (inputManager->pollEvent(event)) {
        gui.processInput(finegui::InputAdapter::fromFineVK(event));
    }

    // Only process game input when GUI doesn't want it
    if (!gui.wantCaptureMouse() && inputManager->isMouseCaptured()) {
        cameraInput.look(inputManager->mouseDelta());
    }
    if (!gui.wantCaptureKeyboard()) {
        cameraInput.updateMovement(inputManager);
    }

    // --- Game Logic Phase ---
    float dt = calculateDeltaTime();
    physics.update(dt);
    camera.update(dt);
    worldRenderer.updateCamera(camera);
    worldRenderer.updateMeshes(16);

    // --- GUI Phase ---
    gui.beginFrame();

    // HUD overlay
    drawHUD(playerHealth, playerHunger, hotbarSlots);

    // Inventory (if open)
    if (inventoryOpen) {
        drawInventoryWindow(inventory);
    }

    // Debug info
    if (showDebugInfo) {
        ImGui::Begin("Debug");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Chunks: %d", worldRenderer.loadedChunkCount());
        ImGui::End();
    }

    gui.endFrame();

    // --- Render Phase ---
    if (auto frame = renderer->beginFrame()) {
        frame.beginRenderPass({0.2f, 0.3f, 0.4f, 1.0f});

        worldRenderer.render(frame);
        gui.render(frame);

        frame.endRenderPass();
        renderer->endFrame();
    }
}
```

---

## 2D GUI Widgets for Games

### HUD Overlay

Use ImGui's window flags to create borderless, transparent overlays:

```cpp
void drawHUD(float health, float hunger, int selectedSlot) {
    ImGuiIO& io = ImGui::GetIO();

    // Health bar - top left
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(200, 0));
    ImGui::Begin("##health", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs);
    ImGui::ProgressBar(health / 100.0f, ImVec2(-1, 20), "");
    ImGui::End();

    // Hotbar - bottom center
    float hotbarWidth = 9 * 52.0f;
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - hotbarWidth) / 2, io.DisplaySize.y - 60));
    ImGui::Begin("##hotbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    for (int i = 0; i < 9; i++) {
        if (i > 0) ImGui::SameLine();

        bool selected = (i == selectedSlot);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        ImGui::Button(slotLabels[i], ImVec2(48, 48));
        if (selected) ImGui::PopStyleColor();
    }
    ImGui::End();
}
```

### Inventory Window

```cpp
void drawInventoryWindow(Inventory& inv) {
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inventory", &inventoryOpen);

    // Grid of inventory slots
    int columns = 9;
    for (int i = 0; i < inv.slotCount(); i++) {
        if (i % columns != 0) ImGui::SameLine();

        ImGui::PushID(i);
        auto& slot = inv.slot(i);

        if (slot.empty()) {
            ImGui::Button("##empty", ImVec2(48, 48));
        } else {
            // Render item icon as a texture (see "3D Items in GUI" below)
            ImGui::ImageButton("##item", slot.iconHandle,
                ImVec2(48, 48));

            // Tooltip on hover
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s x%d", slot.name.c_str(), slot.count);
            }
        }
        ImGui::PopID();
    }

    ImGui::End();
}
```

### Minimap

```cpp
void drawMinimap(TextureHandle minimapTexture) {
    ImGuiIO& io = ImGui::GetIO();
    float size = 200.0f;

    // Top-right corner
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - size - 10, 10));
    ImGui::SetNextWindowSize(ImVec2(size + 16, size + 16));
    ImGui::Begin("##minimap", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::Image(minimapTexture, ImVec2(size, size));
    ImGui::End();
}
```

---

## 3D Items in GUI (Inventory Blocks, Item Preview)

To show 3D objects (like blocks) inside GUI widgets, you render them to an offscreen texture, then display that texture in ImGui.

### Overview

1. Create an `OffscreenSurface` to render 3D content to a texture
2. Set up a camera, lighting, and your 3D rendering pipeline for that surface
3. Render the 3D object to the offscreen surface
4. Register the resulting texture with `gui.registerTexture()`
5. Use the `TextureHandle` with `ImGui::Image()` in your GUI code

### Step-by-Step

```cpp
// --- Setup (once) ---

// Create an offscreen surface for rendering item icons
auto itemSurface = finevk::OffscreenSurface::create(device.get())
    .size(128, 128)               // Icon resolution
    .colorFormat(VK_FORMAT_R8G8B8A8_UNORM)
    .enableDepthBuffer(true)
    .build();

// Set up a small camera looking at the item
finevk::Camera itemCamera;
itemCamera.setPerspective(45.0f, 1.0f, 0.1f, 10.0f);
itemCamera.moveTo({2.0, 1.5, 2.0});
itemCamera.lookAt({0.0, 0.0, 0.0});

// --- Render an item icon (when inventory changes) ---

// Render to offscreen surface
itemSurface->beginFrame();
itemSurface->beginRenderPass({0.0f, 0.0f, 0.0f, 0.0f});  // Transparent bg

blockRenderer.render(itemSurface->commandBuffer(), stoneBlock, itemCamera);

itemSurface->endRenderPass();
itemSurface->endFrame();

// Register the rendered texture with the GUI
TextureHandle stoneIcon = gui.registerTexture(
    itemSurface->colorTexture());

// --- Use in GUI ---
ImGui::Image(stoneIcon, ImVec2(48, 48));
```

### Icon Cache Pattern

For a game with many block/item types, pre-render all icons at startup:

```cpp
class ItemIconCache {
    std::unordered_map<BlockTypeId, TextureHandle> icons_;
    finevk::OffscreenSurface* surface_;
    finegui::GuiSystem* gui_;

public:
    void renderIcon(BlockTypeId blockType) {
        // Render block to offscreen surface
        surface_->beginFrame();
        surface_->beginRenderPass({0, 0, 0, 0});
        blockRenderer_.renderBlock(surface_->commandBuffer(), blockType, camera_);
        surface_->endRenderPass();
        surface_->endFrame();

        // Register and cache
        icons_[blockType] = gui_->registerTexture(surface_->colorTexture());
    }

    TextureHandle getIcon(BlockTypeId blockType) {
        auto it = icons_.find(blockType);
        if (it == icons_.end()) {
            renderIcon(blockType);
            it = icons_.find(blockType);
        }
        return it->second;
    }
};
```

### Animated 3D Preview

For a spinning item preview (e.g., in a tooltip or crafting result), re-render each frame:

```cpp
void drawItemPreview(BlockTypeId blockType) {
    static float rotation = 0.0f;
    rotation += ImGui::GetIO().DeltaTime * 45.0f;  // 45 deg/sec

    // Update camera orbit
    float r = 2.5f;
    itemCamera.moveTo({
        r * std::cos(glm::radians(rotation)),
        1.5,
        r * std::sin(glm::radians(rotation))
    });
    itemCamera.lookAt({0, 0, 0});

    // Render to offscreen
    itemSurface->beginFrame();
    itemSurface->beginRenderPass({0, 0, 0, 0});
    blockRenderer.renderBlock(itemSurface->commandBuffer(), blockType, itemCamera);
    itemSurface->endRenderPass();
    itemSurface->endFrame();

    // Update the registered texture
    gui.unregisterTexture(previewHandle);
    previewHandle = gui.registerTexture(itemSurface->colorTexture());

    // Display in GUI
    ImGui::Image(previewHandle, ImVec2(128, 128));
}
```

---

## Threaded Rendering Mode

For games where GUI logic runs on a separate thread from rendering:

```cpp
// Setup
guiConfig.enableDrawDataCapture = true;
finegui::GuiSystem gui(device.get(), guiConfig);
gui.initialize(renderer.get());

// --- Game thread ---
gui.beginFrame();
// ... ImGui widgets ...
gui.endFrame();

// Copy draw data for the render thread
GuiDrawData drawData = gui.getDrawData();
// Send drawData to render thread (e.g., via queue)

// --- Render thread ---
frame.beginRenderPass(clearColor);
worldRenderer.render(frame);
gui.renderDrawData(frame.commandBuffer(), drawData);
frame.endRenderPass();
```

---

## State Updates

finegui supports a message-passing pattern for pushing game state to GUI:

```cpp
// Define a state update type
struct PlayerStats : finegui::TypedStateUpdate<PlayerStats> {
    float health;
    float hunger;
    int level;
};

// Register a handler
gui.onStateUpdate<PlayerStats>([](const PlayerStats& stats) {
    // Store for use in GUI drawing
    currentHealth = stats.health;
    currentHunger = stats.hunger;
});

// Push updates from game logic
PlayerStats stats{playerHealth, playerHunger, playerLevel};
gui.applyState(stats);
```

---

## API Reference Summary

### GuiSystem Methods

| Method | Description |
|--------|-------------|
| `GuiSystem(device, config)` | Construct with device and config |
| `initialize(surface, subpass)` | Initialize with a RenderSurface |
| `initialize(renderer, subpass)` | Initialize with a SimpleRenderer (backward compat) |
| `processInput(event)` | Forward an input event |
| `beginFrame()` | Start a new GUI frame (auto delta time) |
| `beginFrame(deltaTime)` | Start with explicit delta time |
| `beginFrame(frameIndex, deltaTime)` | Start with explicit frame index and delta time |
| `endFrame()` | Finalize the GUI frame |
| `render(cmd)` | Record draw commands (auto frame index) |
| `render(cmd, frameIndex)` | Record draw commands (explicit frame index) |
| `registerTexture(texture, sampler)` | Register a texture, returns `TextureHandle` |
| `unregisterTexture(handle)` | Unregister a texture |
| `connectToInputManager(input, priority)` | Register as a listener on a finevk InputManager |
| `disconnectFromInputManager()` | Disconnect from the InputManager |
| `handleInputEvent(event)` | Handle a finevk event directly (returns ListenerResult) |
| `setGuiMode(mode)` | Set input consumption mode (Auto/Passive/Exclusive) |
| `guiMode()` | Get current GuiMode |
| `wantCaptureMouse()` | Does the GUI want mouse input? |
| `wantCaptureKeyboard()` | Does the GUI want keyboard input? |
| `imguiContext()` | Access the raw ImGui context |
| `rebuildFontAtlas()` | Trigger font atlas rebuild |

### InputAdapter Static Methods

| Method | Description |
|--------|-------------|
| `fromFineVK(finevkEvent)` | Convert a single finevk event to finegui event |
| `fromInputManager(manager)` | Extract all pending events from an InputManager |
| `glfwKeyToImGui(key)` | Convert GLFW key code to ImGui key code |
| `glfwMouseButtonToImGui(btn)` | Convert GLFW mouse button to ImGui button |

---

## Retained-Mode Widgets

The retained-mode layer (`finegui-retained`) lets you build widget trees declaratively in C++ and render them each frame, without writing ImGui calls directly.

### Setup

```cpp
#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>

// After creating GuiSystem...
finegui::GuiRenderer guiRenderer(gui);
```

### Building Widget Trees

Use `WidgetNode` static builders to construct trees:

```cpp
int mainId = guiRenderer.show(finegui::WidgetNode::window("Settings", {
    finegui::WidgetNode::text("Welcome!"),
    finegui::WidgetNode::separator(),
    finegui::WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f,
        [](finegui::WidgetNode& w) {
            setVolume(w.floatValue);
        }),
    finegui::WidgetNode::checkbox("Fullscreen", false),
    finegui::WidgetNode::button("Apply", [](finegui::WidgetNode&) {
        applySettings();
    }),
    finegui::WidgetNode::combo("Quality", {"Low", "Medium", "High"}, 1),
    finegui::WidgetNode::inputText("Name", "Player"),
}));
```

### Rendering

Call `renderAll()` each frame between `beginFrame()` and `endFrame()`:

```cpp
gui.beginFrame();
guiRenderer.renderAll();
gui.endFrame();
```

### Updating Trees

You can replace a tree wholesale or mutate nodes directly:

```cpp
// Replace entire tree
guiRenderer.update(mainId, finegui::WidgetNode::window("Settings", { ... }));

// Or mutate in place
auto* tree = guiRenderer.get(mainId);
if (tree) {
    tree->children[2].floatValue = 0.8f;  // Update slider value
    tree->children[3].enabled = false;     // Disable checkbox
}

// Remove a tree
guiRenderer.hide(mainId);
```

### Available Widget Types

| Builder | Description |
|---------|-------------|
| `WidgetNode::window(title, children)` | Top-level window |
| `WidgetNode::text(content)` | Static text |
| `WidgetNode::button(label, onClick)` | Click button |
| `WidgetNode::checkbox(label, value, onChange)` | Boolean toggle |
| `WidgetNode::slider(label, value, min, max, onChange)` | Float slider |
| `WidgetNode::sliderInt(label, value, min, max, onChange)` | Integer slider |
| `WidgetNode::inputText(label, value, onChange, onSubmit)` | Text input |
| `WidgetNode::inputInt(label, value, onChange)` | Integer input |
| `WidgetNode::inputFloat(label, value, onChange)` | Float input |
| `WidgetNode::combo(label, items, selected, onChange)` | Dropdown combo |
| `WidgetNode::separator()` | Horizontal line |
| `WidgetNode::group(children)` | Group children |
| `WidgetNode::columns(count, children)` | Multi-column layout |
| `WidgetNode::image(texture, width, height)` | Texture image |

**Phase 3 - Layout & Display:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::sameLine(offsetX)` | Put next widget on same line |
| `WidgetNode::spacing()` | Vertical spacing |
| `WidgetNode::dummy(width, height)` | Invisible spacer of given size |
| `WidgetNode::newLine()` | Force line break after SameLine |
| `WidgetNode::textColored(r, g, b, a, content)` | Colored text |
| `WidgetNode::textWrapped(content)` | Auto-wrapping text |
| `WidgetNode::textDisabled(content)` | Grayed-out text |
| `WidgetNode::progressBar(fraction, width, height, overlay)` | Progress bar |
| `WidgetNode::plotLines(label, values, overlay, scaleMin, scaleMax, width, height)` | Line chart sparkline from float array |
| `WidgetNode::plotHistogram(label, values, overlay, scaleMin, scaleMax, width, height)` | Bar chart from float array |
| `WidgetNode::collapsingHeader(label, children, defaultOpen)` | Expandable section |

**Phase 4 - Containers & Menus:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::tabBar(id, children)` | Tab container |
| `WidgetNode::tabItem(label, children)` | Tab page |
| `WidgetNode::treeNode(label, children, defaultOpen, leaf)` | Tree hierarchy node |
| `WidgetNode::child(id, width, height, border, autoScroll, children)` | Scrollable child region |
| `WidgetNode::menuBar(children)` | Menu bar |
| `WidgetNode::menu(label, children)` | Dropdown menu |
| `WidgetNode::menuItem(label, onClick, shortcut, checked)` | Menu entry |

**Phase 5 - Tables:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::table(id, columns, headers, children, flags)` | Table |
| `WidgetNode::tableRow(children)` | Table row |
| `WidgetNode::tableNextColumn()` | Advance to next column |

**Phase 6 - Advanced Input:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::colorEdit(label, r, g, b, a)` | RGBA color editor |
| `WidgetNode::colorPicker(label, r, g, b, a)` | Color picker wheel |
| `WidgetNode::dragFloat(label, value, speed, min, max)` | Drag-to-adjust float |
| `WidgetNode::dragInt(label, value, speed, min, max)` | Drag-to-adjust int |
| `WidgetNode::dragFloat3(label, x, y, z, speed, min, max, onChange)` | Drag-to-adjust 3-component float vector (x, y, z) |
| `WidgetNode::inputTextWithHint(label, hint, value, onChange, onSubmit)` | Text input with placeholder hint |
| `WidgetNode::sliderAngle(label, valueRadians, minDegrees, maxDegrees, onChange)` | Angle slider (radians internally, degrees displayed) |
| `WidgetNode::smallButton(label, onClick)` | Compact button (no frame padding) |
| `WidgetNode::colorButton(label, r, g, b, a, onClick)` | Color swatch display button |

**Phase 7 - Misc:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::listBox(label, items, selected, heightInItems)` | List selection |
| `WidgetNode::popup(id, children)` | Context popup |
| `WidgetNode::modal(label, children, onClose)` | Modal dialog |

**Phase 8 - Custom:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::canvas(id, width, height, onDraw, onClick)` | Custom draw area |
| `WidgetNode::tooltip(text)` | Text tooltip for previous widget |
| `WidgetNode::tooltip(children)` | Rich tooltip with children |

**Phase 9 - Misc:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::radioButton(label, activeValue, myValue, onChange)` | Radio button |
| `WidgetNode::selectable(label, selected, onClick)` | Selectable item |
| `WidgetNode::inputTextMultiline(label, value, width, height)` | Multi-line text input |
| `WidgetNode::bulletText(content)` | Bulleted text |
| `WidgetNode::separatorText(label)` | Labeled separator |
| `WidgetNode::indent(amount)` / `WidgetNode::unindent(amount)` | Indentation |
| `WidgetNode::window(title, children, flags)` | Window with ImGuiWindowFlags (auto-sized) |
| `WidgetNode::window(title, width, height, children, flags)` | Window with initial size (`SetNextWindowSize` with `ImGuiCond_FirstUseEver`) |

### Window Control

**Programmatic window size.** Use the sized `window()` overload to set the initial window dimensions. The size is applied with `ImGuiCond_FirstUseEver`, so user resizing is preserved across frames:

```cpp
// Auto-sized window (existing behavior)
auto win = WidgetNode::window("Settings", { ... });

// Window with initial size of 400x300
auto win = WidgetNode::window("Settings", 400.0f, 300.0f, { ... });

// Sized window with flags
auto win = WidgetNode::window("HUD", 200.0f, 100.0f, { ... },
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
```

**Window flags.** Both `window()` overloads accept an `int flags` parameter for ImGui window flags. The supported flags in the retained-mode layer:

| Flag | Effect |
|------|--------|
| `ImGuiWindowFlags_NoTitleBar` | Hide the title bar |
| `ImGuiWindowFlags_NoResize` | Prevent user resizing |
| `ImGuiWindowFlags_NoMove` | Prevent user moving |
| `ImGuiWindowFlags_NoScrollbar` | Hide scrollbars |
| `ImGuiWindowFlags_NoCollapse` | Disable collapse (double-click title bar) |
| `ImGuiWindowFlags_AlwaysAutoResize` | Auto-fit to content every frame |
| `ImGuiWindowFlags_NoBackground` | Transparent window background |
| `ImGuiWindowFlags_MenuBar` | Enable window menu bar |
| `ImGuiWindowFlags_NoNav` | Disable keyboard/gamepad navigation within the window |
| `ImGuiWindowFlags_NoInputs` | Disable all inputs (equivalent to `NoMouseInputs \| NoNav`) |

**In scripts**, set `:window_flags` to an array of flag symbols:

```
ui.show {ui.window "Overlay" [
    {ui.text "No input here"}
] :window_flags [:no_title_bar :no_resize :no_move :no_background :no_inputs]}
```

Available script flag symbols: `:no_title_bar`, `:no_resize`, `:no_move`, `:no_scrollbar`, `:no_collapse`, `:always_auto_resize`, `:no_background`, `:menu_bar`, `:no_nav`, `:no_inputs`.

**Phase 10 - Style Push/Pop:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::pushStyleColor(colIdx, r, g, b, a)` | Push ImGui color override |
| `WidgetNode::popStyleColor(count)` | Pop color overrides |
| `WidgetNode::pushStyleVar(varIdx, val)` | Push style var (single float) |
| `WidgetNode::pushStyleVar(varIdx, x, y)` | Push style var (ImVec2) |
| `WidgetNode::popStyleVar(count)` | Pop style var overrides |
| `WidgetNode::pushTheme(name)` | Push a named theme preset (see Style & Theming below) |
| `WidgetNode::popTheme(name)` | Pop a named theme preset (must match the push) |

**Phase 13 - Menus & Popups:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::contextMenu(children)` | Right-click context menu for the previous widget. Place immediately after the target widget in the children list. Children are typically `menuItem` and `separator` widgets. |
| `WidgetNode::mainMenuBar(children)` | Top-level application menu bar (renders at the top of the screen, outside any window). Must be shown as a top-level tree via `guiRenderer.show()`, not inside a window. Children are typically `menu` widgets. |

**Phase 14 - Tooltips & Images:**

| Builder | Description |
|---------|-------------|
| `WidgetNode::itemTooltip(text)` or `WidgetNode::itemTooltip(children)` | Hover tooltip on the previous widget. Place immediately after the target widget in the children list. Supports text-only or rich (multi-widget) content. Functionally similar to `tooltip` but uses `BeginItemTooltip()`. |
| `WidgetNode::imageButton(id, texture, width, height, onClick)` | Clickable image button. Requires TextureHandle and explicit string ID. |

### Callbacks

Callbacks receive a reference to the widget node that triggered them:

```cpp
finegui::WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f,
    [](finegui::WidgetNode& widget) {
        // widget.floatValue is the current slider value
        setVolume(widget.floatValue);
    });
```

| Callback | Triggers on | Available fields |
|----------|-------------|------------------|
| `onClick` | Button click, Image click | - |
| `onChange` | Value change | `floatValue`, `intValue`, `boolValue`, `stringValue`, `selectedIndex`, `floatX/Y/Z`, `colorR/G/B/A` |
| `onSubmit` | Enter in InputText | `stringValue` |
| `onClose` | Window close button | - |
| `onFocus` | Widget gains keyboard focus | - |
| `onBlur` | Widget loses keyboard focus | - |

### Focus Management

Widgets support keyboard focus control through tab navigation and programmatic focus:

```cpp
// Skip a widget in tab navigation
auto btn = WidgetNode::button("Background");
btn.focusable = false;  // Tab skips this widget

// Auto-focus when window first appears
auto input = WidgetNode::inputText("Name", "");
input.autoFocus = true;

// Focus/blur callbacks
input.onFocus = [](WidgetNode& w) { /* widget gained focus */ };
input.onBlur = [](WidgetNode& w) { /* widget lost focus */ };

// Programmatic focus by widget ID
input.id = "name_input";
guiRenderer.setFocus("name_input");  // Focuses on next renderAll()
```

| Field | Default | Description |
|-------|---------|-------------|
| `focusable` | `true` | Set false to skip in tab navigation |
| `autoFocus` | `false` | Focus when parent window first appears |
| `onFocus` | *(none)* | Callback when widget gains keyboard focus |
| `onBlur` | *(none)* | Callback when widget loses keyboard focus |

### Animation Fields

WidgetNode supports animation-related fields for smooth transitions (used by TweenManager):

| Field | Default | Description |
|-------|---------|-------------|
| `alpha` | `1.0f` | Window opacity (0.0 = invisible, 1.0 = fully opaque) |
| `windowPosX` | `FLT_MAX` | Explicit window position X (`FLT_MAX` = ImGui auto-positioning) |
| `windowPosY` | `FLT_MAX` | Explicit window position Y (`FLT_MAX` = ImGui auto-positioning) |
| `scaleX` | `1.0f` | Horizontal scale factor (0.0 = collapsed, 1.0 = normal size) |
| `scaleY` | `1.0f` | Vertical scale factor (0.0 = collapsed, 1.0 = normal size) |
| `rotationY` | `0.0f` | Rotation around the Y-axis in radians (0 = front face, PI = back face) |

```cpp
auto win = WidgetNode::window("Fading", { WidgetNode::text("Semi-transparent") });
win.alpha = 0.5f;          // 50% opacity
win.windowPosX = 100.0f;   // Position at (100, 200)
win.windowPosY = 200.0f;

auto popup = WidgetNode::window("ZoomPopup", { WidgetNode::text("Zoomed!") });
popup.scaleX = 0.0f;       // Start collapsed (animate to 1.0 with TweenManager)
popup.scaleY = 0.0f;
```

### Widget Search by ID

Both `GuiRenderer` and `MapRenderer` support finding widgets by their `id` string. This is more robust than navigating by child index, which breaks when the tree structure changes.

**GuiRenderer:**
```cpp
auto hp = WidgetNode::progressBar(0.85f, 200.0f, 20.0f);
hp.id = "hp_bar";
guiRenderer.show(WidgetNode::window("HUD", { std::move(hp) }));

// Find by ID (searches all trees, returns first match)
WidgetNode* found = guiRenderer.findById("hp_bar");
if (found) {
    found->floatValue = 0.5f;  // Mutate directly
}
```

**MapRenderer:**
```cpp
finescript::Value found = mapRenderer.findById("hp_bar");
if (!found.isNil()) {
    found.asMap().set(engine.intern("value"), Value::number(0.5));
}
```

**From scripts** (using `ui.find` — accepts strings or symbols):
```
set widget {ui.find "hp_bar"}
set widget.value 0.5

# Or with symbol syntax (faster if :id is also a symbol):
set widget {ui.find :hp_bar}
```

The `:id` field on a widget map can be a string or a symbol. `findById` matches both — when searching by string it interns to compare with symbol IDs, and when searching by symbol it resolves to string to compare with string IDs.

### Modals

Modal dialogs block interaction with the rest of the UI. Press **Escape** to close any modal (equivalent to clicking the X button). The `onClose` callback fires in both cases.

### Drag-and-Drop

Any widget can act as a drag source and/or drop target by setting drag-and-drop properties on the `WidgetNode`:

```cpp
auto slot = WidgetNode::image(tex, 48, 48);
slot.dragType = "item";           // DnD type string (empty = not a source)
slot.dragData = "sword_01";       // payload data string
slot.dropAcceptType = "item";     // accepted type (empty = not a target)
slot.dragMode = 0;                // 0=both, 1=drag-only, 2=click-only
slot.onDrop = [](WidgetNode& w) { /* w.dragData has delivered payload */ };
slot.onDragBegin = [](WidgetNode& w) { /* drag started */ };
```

**Two interaction modes** are supported:

- **Traditional drag-and-drop** (ImGui DnD): Click-drag-release. Set `dragMode = 1` for this mode only.
- **Click-to-pick-up** (game inventory style): Click to pick up an item, click again to drop it. Set `dragMode = 2` for this mode only.
- **Both modes** (default): Set `dragMode = 0` to allow either interaction style.

**DragDropManager** is required for the click-to-pick-up mode. It tracks the "held" item and renders a floating icon at the cursor:

```cpp
DragDropManager dndManager;
guiRenderer.setDragDropManager(&dndManager);

// Each frame after renderAll():
dndManager.renderCursorItem();  // draws floating icon at cursor
```

Image widgets with valid textures automatically show image previews during drag.

### Style & Theming

#### Named Theme Presets (pushTheme / popTheme)

Named theme presets let you apply predefined color schemes to a group of widgets without manually specifying individual `ImGuiCol` values. Each preset pushes a set of ImGui style colors that are popped when the matching `popTheme` is called.

Available presets:

| Preset | Colors Pushed | Effect |
|--------|--------------|--------|
| `"danger"` | Button, ButtonHovered, ButtonActive, Text (4 colors) | Red buttons and text |
| `"success"` | Button, ButtonHovered, ButtonActive, Text (4 colors) | Green buttons and text |
| `"warning"` | Button, ButtonHovered, ButtonActive, Text (4 colors) | Orange buttons and text |
| `"info"` | Button, ButtonHovered, ButtonActive, Text (4 colors) | Blue buttons and text |
| `"dark"` | WindowBg, FrameBg, Text (3 colors) | Dark background with light text |
| `"light"` | WindowBg, FrameBg, Text (3 colors) | Light background with dark text |

Push and pop calls must be paired with matching preset names.

**Retained-mode example:**

```cpp
int id = guiRenderer.show(finegui::WidgetNode::window("Themed Buttons", {
    finegui::WidgetNode::pushTheme("danger"),
    finegui::WidgetNode::button("Delete", [](finegui::WidgetNode&) {
        deleteItem();
    }),
    finegui::WidgetNode::popTheme("danger"),

    finegui::WidgetNode::pushTheme("success"),
    finegui::WidgetNode::button("Confirm", [](finegui::WidgetNode&) {
        confirm();
    }),
    finegui::WidgetNode::popTheme("success"),

    finegui::WidgetNode::pushTheme("warning"),
    finegui::WidgetNode::text("Caution: this action is irreversible"),
    finegui::WidgetNode::popTheme("warning"),
}));
```

**Script-mode example:**

```
ui.show {ui.window "Themed Buttons" [
    {ui.push_theme "danger"}
    {ui.button "Delete" fn [] do
        print "Deleted!"
    end}
    {ui.pop_theme "danger"}

    {ui.push_theme "success"}
    {ui.button "Confirm" fn [] do
        print "Confirmed!"
    end}
    {ui.pop_theme "success"}

    {ui.push_theme "warning"}
    {ui.text "Caution: this action is irreversible"}
    {ui.pop_theme "warning"}
]}
```

#### Global Theme Switching (set_theme)

`ui.set_theme` is an immediate action function (not a widget) that switches the entire ImGui color scheme. It calls ImGui's built-in theme functions and affects the entire context.

Available themes:

| Theme | ImGui Function Called |
|-------|---------------------|
| `"dark"` | `ImGui::StyleColorsDark()` |
| `"light"` | `ImGui::StyleColorsLight()` |
| `"classic"` | `ImGui::StyleColorsClassic()` |

This is a script-only action function. To call it from C++, use ImGui's style functions directly.

**Script-mode example:**

```
# Switch to dark theme at startup
ui.set_theme "dark"

ui.show {ui.window "Settings" [
    {ui.text "Choose a theme:"}
    {ui.button "Dark" fn [] do
        ui.set_theme "dark"
    end}
    {ui.button "Light" fn [] do
        ui.set_theme "light"
    end}
    {ui.button "Classic" fn [] do
        ui.set_theme "classic"
    end}
]}
```

---

## Textured Skin System (Planned)

> **Status:** Not yet implemented. See [SKIN_SYSTEM_PLAN.md](SKIN_SYSTEM_PLAN.md) for the full design.

The textured skin system will replace ImGui's flat-colored rectangles with artist-authored textures from a sprite atlas, using 9-slice rendering. This enables game UIs to match the visual style of the rest of the game — ornate window borders, beveled buttons, themed scrollbars, etc.

**Key points for projects planning ahead:**

- The system is **opt-in and non-breaking**. All existing code continues to work unchanged.
- A `SkinManager` will be passed to `GuiRenderer` and/or `MapRenderer` via `setSkinManager()`.
- Skins are defined in finescript files (`.fgs`) mapping widget elements to atlas regions.
- Each element supports per-state textures (normal, hovered, active, disabled) with automatic fallback.
- Per-widget skin overrides will be available via a `skinOverride` field (C++) or `:skin` map field (script).
- `pushSkin` / `popSkin` widgets will allow scoped skin changes within a tree.
- Widgets without skin definitions fall back to standard ImGui rendering, so skins can be partial.

**Planned C++ usage:**
```cpp
SkinAtlas atlas;
atlas.loadTexture(gui, "assets/ui/medieval_atlas.png");
auto skin = Skin::loadFromFile("assets/ui/medieval.fgs", atlas, engine);

SkinManager skinMgr;
skinMgr.registerSkin("medieval", std::move(skin));
skinMgr.setActiveSkin("medieval");

guiRenderer.setSkinManager(&skinMgr);
```

**Planned script usage:**
```
ui.load_skin "medieval" "assets/ui/medieval.fgs"
ui.set_skin "medieval"
```

If you are building a project on finegui now, you can use the default ImGui style and adopt the skin system later without changing any widget code — only add skin loading and activation at startup.

---

## 3D in GUI

### SceneTexture (Offscreen Render to Texture)

SceneTexture wraps a finevk OffscreenSurface and manages texture registration with GuiSystem. It lets you render a 3D scene (or any Vulkan drawing) offscreen and display the result in Image or Canvas widgets.

**Basic Usage (C++):**
```cpp
#include <finegui/scene_texture.hpp>

// Create a 512x512 offscreen render target with depth buffer
finegui::SceneTexture scene(gui, 512, 512);

// Each frame:
scene.beginScene(0, 0, 0, 1);   // clear to black
auto& cmd = scene.commandBuffer();
// ... record Vulkan draw commands ...
scene.endScene();

// Display via Image widget:
auto node = WidgetNode::image(scene.textureHandle(), 512, 512);

// Or display via Canvas widget (with click handling, border, etc.):
auto canvas = WidgetNode::canvas("##viewport", 512, 512, scene.textureHandle());
```

**Resize:**
```cpp
renderer->waitIdle();           // ensure GPU is done with old texture
scene.resize(1024, 1024);
scene.beginScene(0, 0, 0, 1);  // render at new size
scene.endScene();
// textureHandle() now returns a new handle at 1024x1024
```

**Pipeline Creation:** Use `scene.renderTarget()` to get the RenderTarget for creating compatible pipelines:
```cpp
auto pipeline = Pipeline::create(device)
    .renderPass(scene.renderTarget()->renderPass())
    // ... vertex input, shaders, etc.
    .build();
```

### Canvas with Texture

Canvas widgets can now display a texture (e.g. from SceneTexture). The texture fills the canvas area and is drawn before any draw commands or the onDraw callback:

```cpp
// Retained mode:
auto canvas = WidgetNode::canvas("##vp", 256, 256, sceneTexture.textureHandle());

// Script mode - reference a registered texture by name:
// ui.canvas "##vp" 256 256 {:texture "my_scene"}
```

In script mode, register the SceneTexture's handle in the TextureRegistry:
```cpp
textureRegistry.registerTexture("my_scene", scene.textureHandle());
```

**Note:** When using SceneTexture, ensure the GPU has finished using the old texture handle before calling `resize()` or destroying the SceneTexture. Call `renderer->waitIdle()` or equivalent before these operations.

---

## Script-Driven GUI

The script layer (`finegui-script`) lets finescript scripts build and manage widget trees. Scripts use `ui.*` functions to create widget maps, which are converted to `WidgetNode` trees.

Requires: `FINEGUI_BUILD_SCRIPT=ON` in CMake, plus a built finescript library.

### Setup

```cpp
#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/script_gui.hpp>
#include <finegui/script_gui_manager.hpp>
#include <finegui/script_bindings.hpp>

#include <finescript/script_engine.h>

// Create script engine (must outlive Vulkan resources)
finescript::ScriptEngine engine;
finegui::registerGuiBindings(engine);

// After creating GuiSystem and GuiRenderer...
finegui::GuiRenderer guiRenderer(gui);
```

### Single Script GUI

`ScriptGui` is the single-object abstraction: it owns the script context, the widget tree, and message handlers.

```cpp
finegui::ScriptGui scriptGui(engine, guiRenderer);

bool ok = scriptGui.loadAndRun(R"SCRIPT(
    ui.show {ui.window "Hello" [
        {ui.text "Hello from script!"}
        {ui.button "Click me" fn [] do
            print "Clicked!"
        end}
    ]}
)SCRIPT");

// Each frame:
gui.beginFrame();
scriptGui.processPendingMessages();
guiRenderer.renderAll();
gui.endFrame();

// When done:
scriptGui.close();
```

### Script GUI Manager

`ScriptGuiManager` manages multiple `ScriptGui` instances with broadcast messaging:

```cpp
finegui::ScriptGuiManager mgr(engine, guiRenderer);

auto* settings = mgr.showFromSource(settingsScript, "settings");
auto* hud = mgr.showFromSource(hudScript, "hud");

// Each frame:
gui.beginFrame();
mgr.processPendingMessages();  // Drains all queues
guiRenderer.renderAll();
gui.endFrame();

// Clean up inactive GUIs periodically
mgr.cleanup();

// Close all on shutdown
mgr.closeAll();
```

### Writing finegui Scripts

Scripts use `ui.*` functions to build widget maps, and `ui.show` to display them:

```
# Variables
set volume 0.5
set name "Player"

# Build and show a window
ui.show {ui.window "Settings" [
    {ui.text "Game Settings"}
    {ui.separator}
    {ui.slider "Volume" 0.0 1.0 volume fn [v] do
        set volume v
    end}
    {ui.input "Name" name fn [v] do
        set name v
    end}
    {ui.checkbox "Fullscreen" false}
    {ui.combo "Quality" ["Low" "Medium" "High"] 1}
    {ui.button "Apply" fn [] do
        print "Applied!"
    end}
]}
```

### String Interpolation

finescript natively supports string interpolation using `{expression}` syntax inside double-quoted strings. This works in all widget text: titles, labels, text content, and anywhere else a string is accepted.

```
set player_name "Alice"
ui.text "Hello {player_name}!"

set score 100
ui.button "Score: {score}"

set level 5
set area "Dungeon"
ui.window "Level {level} - {area}" [...]
```

Any finescript expression works inside `{}`, including function calls:

```
ui.text "HP: {format \"%d/%d\" hp max_hp}"
```

Use `\{` and `\}` for literal braces in strings.

### Script Widget Functions

Builder functions (return widget maps):

| Function | Arguments | Description |
|----------|-----------|-------------|
| `ui.window` | `title children` | Window with title and child array. Set `:window_size_w` and `:window_size_h` map fields for initial size. Set `:window_flags` to a flag symbol array (see [Window Control](#window-control)). |
| `ui.text` | `content` | Static text |
| `ui.button` | `label [on_click]` | Button with optional callback |
| `ui.checkbox` | `label value [on_change]` | Boolean toggle |
| `ui.slider` | `label min max value [on_change]` | Float slider |
| `ui.slider_int` | `label min max value [on_change]` | Integer slider |
| `ui.input` | `label value [on_change] [on_submit]` | Text input |
| `ui.input_int` | `label value [on_change]` | Integer input |
| `ui.input_float` | `label value [on_change]` | Float input |
| `ui.combo` | `label items selected [on_change]` | Dropdown combo |
| `ui.separator` | *(none)* | Horizontal separator |
| `ui.group` | `children` | Group of children |
| `ui.columns` | `count children` | Multi-column layout |
| `ui.same_line` | `[offset]` | Same-line layout |
| `ui.spacing` | *(none)* | Vertical spacing |
| `ui.dummy` | `width height` | Invisible spacer of given size |
| `ui.new_line` | *(none)* | Force line break after SameLine |
| `ui.text_colored` | `color_array text` | Colored text |
| `ui.text_wrapped` | `text` | Wrapping text |
| `ui.text_disabled` | `text` | Grayed text |
| `ui.progress_bar` | `fraction` | Progress bar |
| `ui.plot_lines` | `label [values] [overlay] [min] [max] [width] [height]` | Line chart sparkline |
| `ui.plot_histogram` | `label [values] [overlay] [min] [max] [width] [height]` | Bar chart |
| `ui.collapsing_header` | `label children` | Expandable section |
| `ui.tab_bar` | `id children` | Tab container |
| `ui.tab` | `label children` | Tab page |
| `ui.tree_node` | `label children` | Tree node |
| `ui.child` | `id children` | Scrollable child |
| `ui.menu_bar` | `children` | Menu bar |
| `ui.menu` | `label children` | Dropdown menu |
| `ui.menu_item` | `label [on_click]` | Menu entry |
| `ui.table` | `id num_columns children` | Table |
| `ui.table_row` | `[children]` | Table row |
| `ui.table_next_column` | *(none)* | Next column |
| `ui.color_edit` | `label color_array` | Color editor |
| `ui.color_picker` | `label color_array` | Color picker |
| `ui.drag_float` | `label value speed min max` | Drag float |
| `ui.drag_int` | `label value speed min max` | Drag int |
| `ui.drag_float3` | `label [x y z] speed min max [on_change]` | Drag 3-component float vector |
| `ui.input_with_hint` | `label hint value [on_change] [on_submit]` | Text input with placeholder hint |
| `ui.slider_angle` | `label value_rad min_deg max_deg [on_change]` | Angle slider (radians/degrees) |
| `ui.small_button` | `label [on_click]` | Compact button (no frame padding) |
| `ui.color_button` | `label [r g b a] [on_click]` | Color swatch display button |
| `ui.listbox` | `label items selected [height]` | List box |
| `ui.popup` | `id children` | Popup |
| `ui.modal` | `title children` | Modal dialog |
| `ui.canvas` | `id width height [commands]` | Custom draw canvas |
| `ui.tooltip` | `text_or_children` | Tooltip |
| `ui.draw_line` | `p1 p2 color [thickness]` | Canvas draw line |
| `ui.draw_rect` | `p1 p2 color [filled]` | Canvas draw rect |
| `ui.draw_circle` | `center radius color [filled] [thickness]` | Canvas draw circle |
| `ui.draw_text` | `pos text color` | Canvas draw text |
| `ui.draw_triangle` | `p1 p2 p3 color [filled]` | Canvas draw triangle |
| `ui.radio_button` | `label value my_value` | Radio button |
| `ui.selectable` | `label selected` | Selectable item |
| `ui.input_multiline` | `label value [width] [height]` | Multiline text input |
| `ui.bullet_text` | `text` | Bulleted text |
| `ui.separator_text` | `label` | Labeled separator |
| `ui.indent` | `[amount]` | Indent |
| `ui.unindent` | `[amount]` | Unindent |
| `ui.image` | `texture_name [width] [height] [on_click]` | Image from TextureRegistry |
| `ui.push_style_color` | `col_idx [r g b a]` | Push ImGui color override |
| `ui.pop_style_color` | `[count]` | Pop color overrides |
| `ui.push_style_var` | `var_idx value` | Push style var (float or `[x y]`) |
| `ui.pop_style_var` | `[count]` | Pop style var overrides |
| `ui.push_theme` | `name` | Push a named theme preset ("danger", "success", "warning", "info", "dark", "light") |
| `ui.pop_theme` | `name` | Pop a named theme preset (must match the push) |
| `ui.context_menu` | `[children]` | Right-click context menu for the previous widget. Place immediately after the target widget in the children list. Children are typically `menu_item` and `separator` widgets. |
| `ui.main_menu_bar` | `[children]` | Top-level application menu bar (renders at the top of the screen, outside any window). Must be shown as a top-level tree via `ui.show`, not inside a window. Children are typically `menu` widgets. |
| `ui.item_tooltip` | `text_or_children` | Hover tooltip on previous widget (text string or array of children) |
| `ui.image_button` | `id texture_name [width] [height] [on_click]` | Clickable image button (resolves texture via TextureRegistry) |

Action functions (require active ScriptGui context):

| Function | Arguments | Description |
|----------|-----------|-------------|
| `ui.show` | `widget_map` | Convert map to widget tree, show it, return ID |
| `ui.update` | `id widget_map` | Replace an existing tree |
| `ui.set_text` | `id child_index text` | Mutate text content of a child node |
| `ui.set_value` | `id child_index value` | Mutate value field of a child node |
| `ui.set_label` | `id child_index label` | Mutate label of a child node |
| `ui.hide` | `[id]` | Hide a tree (or close this GUI if no ID) |
| `ui.node` | `gui_id child_index` | Navigate to child map (returns map ref) |
| `gui.on_message` | `:symbol handler` | Register a message handler |
| `gui.set_focus` | `"widget_id"` | Programmatically focus a widget by ID |
| `ui.find` | `"widget_id"` or `:widget_id` | Find widget map by `:id` string or symbol (nil if not found) |
| `ui.open_popup` | `popup_map` | Open a popup or modal programmatically (sets `:value` to true on the popup/modal map) |
| `ui.close_popup` | *(none)* | Close the innermost open popup (must be called during popup rendering) |
| `ui.save_state` | *(none)* | Save all widget states (by `:id`) as a map. Returns finescript map. |
| `ui.load_state` | `state_map` | Restore widget states from a previously saved map. |
| `ui.set_theme` | `name` | Switch global ImGui theme ("dark", "light", "classic"). Immediate action, not a widget. |
| `gui.bind_key` | `"chord" callback` | Bind a keyboard shortcut. Returns integer binding ID. Requires `setHotkeyManager()`. |
| `gui.unbind_key` | `id` | Unbind a keyboard shortcut by binding ID. |
| `gui.open_popup` | `"id"` | Open a popup by string ID (calls `ImGui::OpenPopup`). |
| `gui.close_popup` | *(none)* | Close the current popup (calls `ImGui::CloseCurrentPopup`). |

### Dynamic Updates from Scripts

**Preferred: Node mutation.** Use `ui.set_text`, `ui.set_value`, or `ui.set_label` to change individual widget fields without rebuilding the tree. This preserves callbacks and is more efficient:

```
set count 0
set gui_id {ui.show {ui.window "Counter" [
    {ui.text "Count: 0"}
    {ui.button "Increment" fn [] do
        set count (count + 1)
        ui.set_text gui_id 0 ("Count: " + {to_str count})
    end}
]}}
```

The second argument is the child index (0 = first child of the window). For nested trees, pass an array of indices: `ui.set_text gui_id [0 1] "text"`.

Mutation functions:

| Function | Arguments | Description |
|----------|-----------|-------------|
| `ui.set_text` | `id child_index text` | Set text content of a child node |
| `ui.set_value` | `id child_index value` | Set value (bool/int/float/string) of a child node |
| `ui.set_label` | `id child_index label` | Set label of a child node |

**Alternative: Full tree rebuild.** Use `ui.update` to replace an entire tree. This is simpler for major layout changes but loses callbacks on the rebuilt nodes:

```
ui.update gui_id {ui.window "Counter" [
    {ui.text ("Count: " + {to_str count})}
    {ui.button "Increment" fn [] do ... end}
]}
```

**Map-direct-mutation (MapRenderer path).** When using MapRenderer (see below), scripts create maps that ARE the widget data (shared_ptr semantics). You can mutate widget properties directly without calling mutation functions:

```
set text_widget {ui.text "Initial"}
ui.show {ui.window "Dynamic" [
    text_widget
    {ui.button "Update" fn [] do
        set text_widget.text "Updated!"
    end}
]}
```

Since maps are shared by reference, `set text_widget.text "Updated!"` directly mutates the rendered data. This is simpler and more efficient than `ui.set_text`.

**Navigation with `ui.node`:** You can navigate into a displayed tree to get references to child maps by index, then mutate them directly:

```
set gui_id {ui.show {ui.window "Nav" [
    {ui.text "Child 0"}
    {ui.group [{ui.text "Nested"}]}
]}}
set child0 {ui.node gui_id 0}       # Get child by index
set nested {ui.node gui_id [1 0]}    # Nested path
set child0.text "Modified!"          # Mutate via reference
```

### Message Passing

Scripts register message handlers with `gui.on_message`. C++ delivers messages:

**Script side:**
```
gui.on_message :player_died fn [data] do
    ui.update gui_id {ui.window "Game Over" [
        {ui.text "You died!"}
        {ui.button "Respawn"}
    ]}
end
```

**C++ side:**
```cpp
// Direct delivery (synchronous, on GUI thread)
scriptGui.deliverMessage(engine.intern("player_died"), Value::nil());

// Queued delivery (thread-safe, from any thread)
scriptGui.queueMessage(engine.intern("player_died"), Value::nil());

// Broadcast to all GUIs
mgr.broadcastMessage(engine.intern("player_died"), Value::nil());
```

### Pre-binding Variables

Pass initial values from C++ into the script context:

```cpp
scriptGui.loadAndRun(source, "hud", {
    {"player_name", finescript::Value::string("Alice")},
    {"gold", finescript::Value::integer(100)},
});
```

The script can then use `player_name` and `gold` as variables.

### Reading Script State from C++

Access script variables through the execution context:

```cpp
auto* ctx = scriptGui.context();
auto val = ctx->get("volume");
if (val.isNumeric()) {
    float volume = static_cast<float>(val.asFloat());
}
```

### Mixing C++ and Script GUIs

Both C++ retained-mode widgets and script-driven widgets share the same `GuiRenderer`, so they coexist naturally:

```cpp
// C++ retained-mode window
guiRenderer.show(finegui::WidgetNode::window("Control Panel", {
    finegui::WidgetNode::button("Reset", [&](finegui::WidgetNode&) {
        scriptGui.deliverMessage(engine.intern("reset"), Value::nil());
    }),
}));

// Script-driven window
mgr.showFromSource(R"SCRIPT(
    ui.show {ui.window "Settings" [{ui.slider "Volume" 0.0 1.0 0.5}]}
)SCRIPT", "settings");

// Both render together
gui.beginFrame();
mgr.processPendingMessages();
guiRenderer.renderAll();  // Renders all trees: C++ and script
gui.endFrame();
```

---

## Map-Based Rendering (MapRenderer)

The `MapRenderer` is an alternative to the `GuiRenderer` + `WidgetNode` approach. Instead of converting script maps to `WidgetNode` trees, `MapRenderer` renders directly from finescript map data. This is the path used by `ScriptGui` internally.

The key advantage is that script maps use shared_ptr semantics, so direct mutation of map fields immediately affects the rendered output without needing explicit mutation functions.

```cpp
#include <finegui/map_renderer.hpp>

MapRenderer mapRenderer(engine);

// Show a map tree
int id = mapRenderer.show(mapValue, ctx);

// Each frame:
gui.beginFrame();
mapRenderer.renderAll();
gui.endFrame();
```

MapRenderer supports drag-and-drop and texture registries:

```cpp
DragDropManager dndMgr;
mapRenderer.setDragDropManager(&dndMgr);

TextureRegistry texRegistry;
texRegistry.registerTexture("sword_icon", swordHandle);
mapRenderer.setTextureRegistry(&texRegistry);
```

---

## TextureRegistry

The `TextureRegistry` provides a name-to-handle mapping so that scripts (and MapRenderer) can reference textures by string name rather than raw handles. If a texture name is not found in the registry, a placeholder text is shown instead.

```cpp
#include <finegui/texture_registry.hpp>

TextureRegistry texRegistry;
texRegistry.registerTexture("sword_icon", swordHandle);
texRegistry.registerTexture("shield_icon", shieldHandle);
mapRenderer.setTextureRegistry(&texRegistry);
```

Scripts can then use `ui.image` with texture names:

```
ui.show {ui.window "Inventory" [
    {ui.image "sword_icon" 48 48}
    {ui.image "shield_icon" 48 48}
]}
```

---

## TweenManager (Animation)

`TweenManager` provides smooth property interpolation for retained-mode widgets. It animates `WidgetNode` fields (alpha, position, value, color) over time with configurable easing.

```cpp
#include <finegui/tween_manager.hpp>

TweenManager tweens(guiRenderer);

// Game loop:
gui.beginFrame();
tweens.update(ImGui::GetIO().DeltaTime);  // advance animations
guiRenderer.renderAll();                   // reads tweened values
gui.endFrame();
```

### Convenience Methods

```cpp
// Fade in/out (animates alpha)
tweens.fadeIn(guiId, 0.3f);
tweens.fadeOut(guiId, 0.3f, Easing::EaseIn, [](int id) {
    // Called when fade completes
});

// Slide to position
tweens.slideTo(guiId, 300.0f, 400.0f, 0.4f);

// Color transition (targets a child widget by index path)
tweens.colorTo(guiId, {0, 1}, 1.0f, 0.0f, 0.0f, 1.0f, 0.3f);  // Flash red

// Screen shake
tweens.shake(guiId, 0.4f, 8.0f, 15.0f);

// Zoom in/out (animates scaleX and scaleY)
tweens.zoomIn(guiId, 0.3f);   // Scale 0 -> 1, window appears from center
tweens.zoomOut(guiId, 0.3f, Easing::EaseIn, [](int id) {
    // Called when zoom-out completes
});

// Y-axis flip (animates rotationY)
tweens.flipY(guiId, 0.5f);      // Rotate 0 -> PI, shows back side
tweens.flipYBack(guiId, 0.5f);  // Rotate PI -> 0, shows front side
```

**Note:** The Vulkan pipeline uses `cullNone`, so back-face culling is disabled. Flip animations render correctly at all rotation angles without faces being discarded.

### Generic Animation

```cpp
// Animate any property — reads current value as "from" automatically
tweens.animate(guiId, {}, TweenProperty::Alpha, 0.5f, 0.3f, Easing::EaseOut);

// Explicit from/to
tweens.animate(guiId, {0}, TweenProperty::FloatValue, 0.0f, 1.0f, 0.5f);
```

### Easing Functions

| Easing | Description |
|--------|-------------|
| `Linear` | Constant speed |
| `EaseIn` | Quadratic acceleration |
| `EaseOut` | Quadratic deceleration (default) |
| `EaseInOut` | Quadratic ease in then out |
| `CubicOut` | Cubic deceleration |
| `ElasticOut` | Elastic overshoot |
| `BounceOut` | Bouncing effect |

### Animatable Properties

| Property | WidgetNode field | Typical use |
|----------|-----------------|-------------|
| `Alpha` | `alpha` | Fade in/out |
| `PosX`, `PosY` | `windowPosX`, `windowPosY` | Slide windows |
| `FloatValue` | `floatValue` | Progress bars, sliders |
| `IntValue` | `intValue` | Counters |
| `ColorR/G/B/A` | `colorR/G/B/A` | Color transitions |
| `Width`, `Height` | `width`, `height` | Resize |
| `ScaleX`, `ScaleY` | `scaleX`, `scaleY` | Zoom in/out |
| `RotationY` | `rotationY` | Y-axis flip |

### Cancellation

```cpp
int tid = tweens.fadeIn(guiId, 1.0f);
tweens.cancel(tid);          // Cancel specific tween
tweens.cancelAll(guiId);     // Cancel all tweens on a widget tree
tweens.cancelAll();          // Cancel everything
tweens.isActive(tid);        // Check if still running
tweens.activeCount();        // Number of active tweens
```

### Target Resolution

Tweens identify their target widget by `guiId` (from `guiRenderer.show()`) plus an optional `childPath` (vector of child indices). If the target tree is removed, the tween silently stops.

```cpp
// Target root window
tweens.animate(guiId, {}, TweenProperty::Alpha, 0.0f, 0.3f);

// Target children[0] of the window
tweens.animate(guiId, {0}, TweenProperty::FloatValue, 1.0f, 0.5f);

// Target children[2].children[0] (deeply nested)
tweens.animate(guiId, {2, 0}, TweenProperty::ColorR, 1.0f, 0.3f);
```

---

## State Serialization

finegui supports saving and loading widget state, enabling features like persistent user preferences, session restore, and undo/redo. Only widgets with explicit IDs (the `id` field on `WidgetNode`, or `:id` on script maps) are included in serialization.

### State Types

Widget state is stored as a `WidgetStateMap`, which is an `std::unordered_map<std::string, WidgetStateValue>`. Each key is the widget's `id` string, and the value is a variant:

```cpp
using WidgetStateValue = std::variant<bool, int, double, std::string, std::vector<float>>;
```

The state field saved depends on the widget type:

| Widget Type | State Field | Variant Type |
|-------------|-------------|--------------|
| `checkbox` | `boolValue` | `bool` |
| `slider` | `floatValue` | `double` |
| `sliderInt` | `intValue` | `int` |
| `combo` | `selectedIndex` | `int` |
| `inputText` | `stringValue` | `std::string` |
| `inputInt` | `intValue` | `int` |
| `inputFloat` | `floatValue` | `double` |
| `colorEdit` | RGBA color | `std::vector<float>` (4 elements) |
| `colorPicker` | RGBA color | `std::vector<float>` (4 elements) |
| `dragFloat` | `floatValue` | `double` |
| `dragInt` | `intValue` | `int` |
| `radioButton` | `intValue` | `int` |
| `selectable` | `boolValue` | `bool` |

### Retained Mode (C++ API)

`GuiRenderer` provides save/load methods that operate on individual widget trees or across all trees:

```cpp
// Save state for a single tree (by gui ID from show())
WidgetStateMap state = guiRenderer.saveState(guiId);

// Save state across all trees
WidgetStateMap state = guiRenderer.saveState();

// Load state into a single tree
guiRenderer.loadState(guiId, state);

// Load state across all trees
guiRenderer.loadState(state);
```

Only widgets that have a non-empty `id` field are included. Widgets without an explicit ID are skipped during both save and load.

**Example: Persisting settings across sessions**

```cpp
// Build a settings window with explicit IDs
int settingsId = guiRenderer.show(WidgetNode::window("Settings", {
    WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f),
    WidgetNode::checkbox("Fullscreen", false),
    WidgetNode::combo("Quality", {"Low", "Medium", "High"}, 1),
}));

// Give each widget an ID for serialization
auto* tree = guiRenderer.get(settingsId);
tree->children[0].id = "volume";
tree->children[1].id = "fullscreen";
tree->children[2].id = "quality";

// Save state (e.g., when closing the application)
WidgetStateMap state = guiRenderer.saveState(settingsId);
// state contains: {"volume": 0.5, "fullscreen": false, "quality": 1}

// Load state (e.g., when reopening)
guiRenderer.loadState(settingsId, state);
```

### Script Mode (finescript API)

Scripts use `ui.save_state` and `ui.load_state` to save and restore widget state. The returned value is a finescript map keyed by widget `:id`.

```
# Save current widget state
set state (ui.save_state)

# ... later (e.g., after rebuilding the UI) ...

# Restore state
ui.load_state state
```

Widgets in scripts must have an `:id` field to be included:

```
ui.show {ui.window "Settings" [
    {ui.slider "Volume" 0.0 1.0 0.5 :id "volume"}
    {ui.checkbox "Fullscreen" false :id "fullscreen"}
    {ui.combo "Quality" ["Low" "Medium" "High"] 1 :id "quality"}
]}

# Save
set saved (ui.save_state)

# Restore
ui.load_state saved
```

### Serializing State to Disk

To persist state across application restarts, use `MapRenderer::serializeState()` to convert a `WidgetStateMap` into parseable finescript source text. The serialized format uses `{=key value}` map literal syntax that can be parsed back by the script engine.

**C++ round-trip example:**

```cpp
// 1. Save state
auto state = renderer.saveState(guiId);

// 2. Serialize to finescript source text
std::string text = MapRenderer::serializeState(state, engine.interner());

// 3. Write to file
std::ofstream out("settings.dat");
out << text;
out.close();

// 4. Later: read file, parse, and restore
std::ifstream in("settings.dat");
std::string source((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());

// Parse the serialized map back through the script engine
auto result = engine.executeCommand(source);

// Convert the parsed map back to WidgetStateMap and load
// (application-specific conversion from finescript::Value map)
```

**Script round-trip example:**

```
# Save and serialize
set state (ui.save_state)
set text {ui.serialize_state state}
# Write `text` to a file via your file I/O mechanism

# Later: read file contents back into `text`
# Parse and restore
set restored_state {eval text}
ui.load_state restored_state
```

---

## Keyboard Shortcuts (HotkeyManager)

`HotkeyManager` is a standalone class for binding keyboard shortcuts to callbacks. It wraps ImGui's `Shortcut()` API with focus routing, so hotkeys will not fire when a text input field is active. Call `update()` once per frame between `beginFrame()` and `endFrame()`.

### C++ API (Retained Mode)

```cpp
#include <finegui/hotkey_manager.hpp>

finegui::HotkeyManager hotkeys;

// Bind shortcuts — each returns a binding ID
int saveId = hotkeys.bind(ImGuiMod_Ctrl | ImGuiKey_S, []() { save(); });
hotkeys.bind(ImGuiKey_F5, []() { refresh(); });
hotkeys.bind(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, []() { redo(); });

// Each frame:
gui.beginFrame();
hotkeys.update();           // check all bindings and fire callbacks
guiRenderer.renderAll();
gui.endFrame();

// Manage bindings
hotkeys.unbind(saveId);                         // remove by ID
hotkeys.unbindChord(ImGuiMod_Ctrl | ImGuiKey_S); // remove all bindings for a chord
hotkeys.unbindAll();                            // remove everything

// Enable/disable individual bindings
hotkeys.setEnabled(saveId, false);
hotkeys.setEnabled(saveId, true);

// Enable/disable all bindings globally
hotkeys.setGlobalEnabled(false);   // suppress all hotkeys (e.g., during a modal)
hotkeys.setGlobalEnabled(true);

// Query
hotkeys.isEnabled(saveId);         // check a specific binding
hotkeys.isGlobalEnabled();
hotkeys.bindingCount();            // number of active bindings
```

### Script API

Scripts use `gui.bind_key` and `gui.unbind_key` to manage hotkeys:

```
# Bind a hotkey — returns a binding ID
set save_id (gui.bind_key "ctrl+s" fn [] do
    set state (ui.save_state)
end)

gui.bind_key "f5" fn [] do
    print "Refreshing..."
end

gui.bind_key "ctrl+shift+z" fn [] do
    print "Redo"
end

# Unbind by ID
gui.unbind_key save_id
```

### String Chord Format

`parseChord()` accepts a case-insensitive string of modifier and key names separated by `+`. This format is used by the script API and is also available from C++ via `HotkeyManager::parseChord()`.

**Modifiers:** `ctrl`, `shift`, `alt`, `super` / `cmd`

**Letters:** `a` -- `z`

**Digits:** `0` -- `9`

**Function keys:** `f1` -- `f24`

**Named keys:**

| String | Key |
|--------|-----|
| `escape` / `esc` | Escape |
| `enter` / `return` | Enter |
| `space` | Space |
| `tab` | Tab |
| `backspace` | Backspace |
| `delete` / `del` | Delete |
| `insert` / `ins` | Insert |
| `up`, `down`, `left`, `right` | Arrow keys |
| `home`, `end` | Home / End |
| `pageup`, `pagedown` | Page Up / Page Down |
| `minus` | Minus / Hyphen |
| `equals` / `equal` | Equals |

Combine modifiers and a key with `+`:

```
"ctrl+s"           → Ctrl+S
"ctrl+shift+a"     → Ctrl+Shift+A
"f5"               → F5
"alt+enter"        → Alt+Enter
```

### Static Utilities

Two static helper methods are available for working with chord values directly:

```cpp
// Parse a string into an ImGuiKeyChord (returns 0 on failure)
ImGuiKeyChord chord = HotkeyManager::parseChord("ctrl+s");

// Format a chord as a human-readable string
std::string label = HotkeyManager::formatChord(ImGuiMod_Ctrl | ImGuiKey_S);
// label == "Ctrl+S"
```

These are useful for displaying shortcut labels in menu items or tooltips.

### ScriptGui Integration

To enable `gui.bind_key` / `gui.unbind_key` in scripts, connect the `HotkeyManager` to the `ScriptGui` instance before running scripts that use hotkey bindings:

```cpp
finegui::HotkeyManager hotkeys;
finegui::ScriptGui scriptGui(engine, guiRenderer);
scriptGui.setHotkeyManager(&hotkeys);

scriptGui.loadAndRun(R"SCRIPT(
    gui.bind_key "ctrl+s" fn [] do
        print "Save!"
    end
)SCRIPT");

// Each frame:
gui.beginFrame();
hotkeys.update();
scriptGui.processPendingMessages();
guiRenderer.renderAll();
gui.endFrame();
```

The `HotkeyManager` must outlive the `ScriptGui` that references it.

---

## Build System

finegui is organized as layered libraries:

| Library | CMake Target | Depends On | Description |
|---------|-------------|------------|-------------|
| `finegui` | `finegui` / `finegui-shared` | finevk, imgui | Core (immediate mode) |
| `finegui-retained` | `finegui-retained` / `finegui-retained-shared` | finegui | Retained-mode widgets |
| `finegui-script` | `finegui-script` / `finegui-script-shared` | finegui-retained, finescript | Script engine integration |

CMake options:

| Option | Default | Description |
|--------|---------|-------------|
| `FINEGUI_BUILD_RETAINED` | `OFF` | Build the retained-mode layer |
| `FINEGUI_BUILD_SCRIPT` | `OFF` | Build the script layer (requires retained + finescript) |

```bash
# Build everything
cmake -DFINEGUI_BUILD_SCRIPT=ON ..
make -j$(nproc)
```

### Linking

```cmake
# Core only (immediate mode)
target_link_libraries(myapp PRIVATE finegui)

# Retained mode
target_link_libraries(myapp PRIVATE finegui-retained)

# Script integration
target_link_libraries(myapp PRIVATE finegui-script)
```
