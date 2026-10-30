# Textured Skins

**Status:** Implemented

A skin replaces ImGui's flat rectangles with art: a window becomes an iron-bound oak panel, a
progress bar a strip of embers. It's opt-in, and with no skin active ImGui draws as it always
has. A skin can be partial: any part it leaves out stays ImGui's own.

## How it works

ImGui paints every widget's background from a small number of places. A patch adds a call at
each of them:

```cpp
if (!ImGuiSkinDraw(window->DrawList, ImGuiSkinPart_Button, ImGuiSkinStateOf(hovered, held), bb.Min, bb.Max))
    RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
```

So layout, input, text, popups, scrollbars, and everything else stay ImGui's, and only the
paint changes. This replaced an earlier plan to reimplement each widget in finegui, which would
have meant rebuilding window chrome and scrollbars by hand and would have missed whatever
ImGui draws for itself. It costs a patch to keep up to date, which is 31 lines over 25 hunks.

| File | What |
|---|---|
| `third_party/imgui-skin-hooks.patch` | The hooks |
| `include/finegui/imgui_skin_hooks.h` | The parts and states the hooks use. Separate from `imgui.h`, which stays stock |
| `include/finegui/skin.hpp` | `Skin`, 9-slice layout, `setActiveSkin` |
| `include/finegui/skin_loader.hpp` | Loads a `.fgs` definition, its atlas, and its font |

## Using a skin

From C++:

```cpp
std::string error;
auto skin = finegui::loadSkin(gui, "ui/skin/hearth.fgs", engine, context, error);
if (skin) finegui::setActiveSkin(skin.get());   // nullptr goes back to plain ImGui
```

A game on FineStructureVoxel calls `ui_skin "ui/skin/hearth.fgs"` from its UI script.

## Skin definitions

A `.fgs` file is a finescript map. finescript ends a statement at a newline, so it has to be
one line, and it's best generated along with the atlas: Keystone's `tools/gen_ui_skin.py` is a
worked example that writes both, and can't let them drift apart.

```
{=atlas "hearth.png" =units_per_pixel 0.5 =font "Font.ttf" =font_size 18 =regions {=btn {=x 6 =y 6 =w 200 =h 68 =inset [16 16 16 16]} =btn_hot {=x 212 =y 6 =w 200 =h 68 =inset [20 20 20 20]}} =parts {=button {=normal "btn" =hovered "btn_hot"}} =style {=frame_padding [11 6]} =colors {=Text [0.95 0.88 0.74 1]}}
```

| Key | Meaning |
|---|---|
| `atlas` | Image, found beside the `.fgs`. Keep a few pixels clear between regions, or filtering picks up the neighbour |
| `units_per_pixel` | 0.5 for art drawn at 2x, which stays sharp on high-DPI screens |
| `font`, `font_size` | Optional. Found beside the `.fgs` |
| `regions` | Rectangles of the atlas. `inset` is the left, top, right, bottom border that doesn't stretch |
| `parts` | For each part, which region to use in each state |
| `style` | `window_padding`, `frame_padding`, `item_spacing`, `item_inner_spacing`, `cell_padding`, `window_title_align`, `button_text_align`, `scrollbar_size`, `grab_min_size`, `indent_spacing` |
| `colors` | Any of ImGui's colours by its own name, such as `Text` or `CheckMark` |

Skins start from square corners and no outlines, since borders belong to the art.

**Parts:** `window`, `popup`, `tooltip`, `title_bar`, `menu_bar`, `button`, `close_button`,
`frame` (inputs, drags, combos), `checkbox`, `radio`, `slider_track`, `slider_grab`,
`progress_track`, `progress_fill`, `scrollbar_track`, `scrollbar_grab`, `header` (selectables,
tree nodes, collapsing headers, menu items), `tab`.

**States:** `normal`, `hovered`, `active`, `selected`. `active` falls back to `hovered`, and
anything else to `normal`. A part with no `normal` isn't skinned. A check mark or radio dot is
still ImGui's, in the `CheckMark` colour, over whatever the part draws.

## Updating ImGui

The submodule stays exactly as upstream has it. CMake copies the two files the patch touches
into the build directory and patches those, so an update is:

```sh
git -C third_party/imgui fetch --tags
git -C third_party/imgui checkout <version>
cmake --build build
```

If a hunk no longer fits, configure stops with `patch`'s report and nothing is built wrong.
Hunks that still fit apply on their own, so usually only one or two need attention:

```sh
mkdir -p /tmp/skin/a /tmp/skin/b
cp third_party/imgui/imgui.cpp third_party/imgui/imgui_widgets.cpp /tmp/skin/a/
cp /tmp/skin/a/*.cpp /tmp/skin/b/
(cd /tmp/skin/b && patch -p1 --batch -i "$OLDPWD/third_party/imgui-skin-hooks.patch")
# Each hunk that failed is in a .rej file beside its source. Make those edits by hand
# in /tmp/skin/b, then delete the .rej and .orig files and write the new patch:
(cd /tmp/skin && { diff -u a/imgui.cpp b/imgui.cpp; diff -u a/imgui_widgets.cpp b/imgui_widgets.cpp; }) \
    > third_party/imgui-skin-hooks.patch
```

After an update, look for widgets ImGui has added or redrawn: a new one simply shows up in
plain ImGui until it gets a hook. A window holding every widget is a quick way
to see them all at once.

## Limits

- 9-slice centres and edges stretch rather than tile, so art wants its grain along the long axis.
- Skin art and font glyphs live in different textures, so widgets cost a draw call or two each.
  That's nothing for a HUD, and would want a shared atlas for a dense data table.
- `ImGui::SetNextWindowBgAlpha` doesn't reach skinned windows.
