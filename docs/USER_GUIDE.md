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

### Input Capture

Check whether the GUI wants to consume mouse/keyboard input before forwarding to your game:

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
