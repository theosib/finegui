# finegui

A GUI toolkit for games, built on [Dear ImGui](https://github.com/ocornut/imgui) and drawn with
Vulkan through [FineStructureVK](https://github.com/theosib/FineStructureVulkan). You can use it as
plain ImGui from C++, as a retained widget tree, or, most interestingly, from
[finescript](https://github.com/theosib/FineStructureScript), where **a GUI is just data**.

It draws all the UI in [Keystone](https://github.com/theosib/Keystone-RTS), an RTS on the
[FineStructureVoxel](https://github.com/theosib/FineStructureVoxel) engine.

## A GUI is a live data structure

In most toolkits a script builds widgets through an API, and the widgets then live somewhere else.
In finegui there's nothing else. A window on screen and a finescript map are **the same thing**: a
map is a widget, its fields are the widget's properties, and its `children` array is what's inside
it. There's no widget object behind the map, and no step that converts one to the other. Every
frame, the renderer walks the script's own maps and draws what it finds.

The mapping runs both ways, so there's nothing to keep in sync:

- **Change the data, and the screen follows.** Set a field, and it's on screen next frame.
- **Change the screen, and the data follows.** When a player drags a slider, the new value is
  written into that slider's map, where the script can read it.
- **Change the shape, and the tree follows.** Push onto `children` and a widget appears in the
  open window. Remove one and it's gone.

```
set label {ui.text "Hello"}
set window {ui.window "Greeter" [label]}
ui.show window

set label.text "Goodbye"                # On screen by the next frame
window.children.push {ui.button "OK"}   # A new button in the open window
```

The `ui.*` functions are only a convenience, since all they do is make maps. This shows the same
window, with no function involved until `ui.show`:

```
ui.show {=type :window =title "Greeter" =children [{=type :text =text "Hello"}]}
```

Since a GUI is an ordinary value, everything you can do with a value, you can do with a GUI. Keep a
reference to the one label you'll want to change. Write a function that returns a row, and call it
in a loop. Build a panel ahead of time with `ui.stage`, and show it with `ui.go_live` the moment a
key is pressed. There's no framework to learn beyond the language's own maps and arrays.

This works because finescript maps are shared by reference, so the script and the renderer hold the
same object. See [Dynamic Updates](docs/USER_GUIDE.md#dynamic-updates-from-scripts) and
[Map-Based Rendering](docs/USER_GUIDE.md#map-based-rendering-maprenderer) in the guide.

## What else it does

- **Three ways to work, side by side.** Immediate-mode ImGui, retained C++ `WidgetNode` trees, and
  script maps can all be on screen at once.
- **A wide set of widgets.** Windows, tables, trees, tabs, menus, modals, tooltips, plots, drawing
  canvases, inputs with history, and drag-and-drop between slots. See
  [Available Widget Types](docs/USER_GUIDE.md#available-widget-types).
- **Textured skins.** A skin replaces ImGui's flat rectangles with art, so a window can be an
  iron-bound oak panel and a progress bar a strip of embers. It's opt-in, and it can be partial. See
  [docs/SKIN_SYSTEM.md](docs/SKIN_SYSTEM.md).
- **3D inside the GUI.** Render to a texture and show it in a widget, for item previews and
  minimaps. See [3D in GUI](docs/USER_GUIDE.md#3d-in-gui).
- **Animation.** Tweens for fades, slides, pulses and colour changes.
- **Hotkeys and input routing.** The GUI sits in a prioritized input chain, and takes only what it
  should: `Auto` follows ImGui, `Passive` watches, and `Exclusive` keeps everything for a menu.
- **Threaded rendering.** Draw data can be captured on one thread and submitted on another.
- **High-DPI.** Layout is in logical pixels, and fonts are rasterized at the screen's real density.
- **Messages from C++.** Scripts register handlers, and C++ can deliver to them from any thread.

## Quick start

```cpp
#include <finegui/finegui.hpp>

finegui::GuiConfig config;
config.msaaSamples = renderer->msaaSamples();
config.dpiScale = window->contentScale().x;

finegui::GuiSystem gui(device.get(), config);
gui.initialize(renderer.get());

// Each frame
gui.beginFrame();
ImGui::Begin("My Window");
ImGui::Text("Hello from finegui!");
ImGui::End();
gui.endFrame();

frame.beginRenderPass({0.1f, 0.1f, 0.15f, 1.0f});
gui.render(frame);
frame.endRenderPass();
```

## Building

finegui looks for [FineStructureVK](https://github.com/theosib/FineStructureVulkan) and, for the
script layer, [finescript](https://github.com/theosib/FineStructureScript) as sibling directories,
each built first. ImGui is a submodule, carrying a small patch for skins.

```sh
git clone --recursive https://github.com/theosib/finegui.git
cmake -S finegui -B finegui/build -DCMAKE_BUILD_TYPE=Release
cmake --build finegui/build -j
```

## Documentation

| Document | Covers |
|---|---|
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Everything, with examples: start here |
| [docs/SKIN_SYSTEM.md](docs/SKIN_SYSTEM.md) | Textured skins, and keeping ImGui's patch current |
| [docs/theming_design.md](docs/theming_design.md) | Styles and themes |
| [docs/input_event_design.md](docs/input_event_design.md) | How input reaches the GUI |
| [docs/gui_examples_design.md](docs/gui_examples_design.md) | Worked designs for game UIs |
| [docs/USER_GUIDE_LLM.md](docs/USER_GUIDE_LLM.md) | The API, condensed for coding assistants |

## License

Apache License 2.0: see [LICENSE](LICENSE) and [NOTICE](NOTICE). Dear ImGui has its own licence, in
`third_party/imgui/LICENSE.txt`.
