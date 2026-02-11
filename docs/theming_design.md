# Theming & Styling Design Notes

## Phase A: Script Style Push/Pop (Near-term)

Expose ImGui's built-in `PushStyleColor`/`PushStyleVar`/`PopStyleColor`/`PopStyleVar` to scripts and retained mode. This is the high-value, low-effort path that gets dark/themed UIs without custom rendering.

### Script API

```
# Push individual colors
ui.push_color :button [0.2 0.1 0.1 1.0]
ui.push_color :button_hovered [0.4 0.2 0.2 1.0]
# ... widgets ...
ui.pop_color 2

# Push style vars (floats or vec2)
ui.push_var :frame_rounding 8.0
ui.push_var :window_padding [12 12]
# ... widgets ...
ui.pop_var 2

# Named theme preset (applies a batch of push calls)
ui.push_theme "dark_fantasy"
# ... widgets ...
ui.pop_theme
```

### Implementation

**Widget approach:** Style pushes are pseudo-widgets in the tree (like SameLine, Spacing, Indent):

```cpp
// New WidgetNode types
WidgetNode::Type::PushStyleColor   // fields: intValue = ImGuiCol_*, colorR/G/B/A
WidgetNode::Type::PopStyleColor    // fields: intValue = count
WidgetNode::Type::PushStyleVar     // fields: intValue = ImGuiStyleVar_*, floatValue or width/height
WidgetNode::Type::PopStyleVar      // fields: intValue = count
```

In renderers, these map directly to `ImGui::PushStyleColor()`/`ImGui::PopStyleColor()` etc.

**Symbol mapping** needed in ConverterSymbols:

```
:button -> ImGuiCol_Button
:button_hovered -> ImGuiCol_ButtonHovered
:button_active -> ImGuiCol_ButtonActive
:window_bg -> ImGuiCol_WindowBg
:frame_bg -> ImGuiCol_FrameBg
:text -> ImGuiCol_Text
:border -> ImGuiCol_Border
# ... all ~55 ImGuiCol_ values

:frame_rounding -> ImGuiStyleVar_FrameRounding
:window_rounding -> ImGuiStyleVar_WindowRounding
:window_padding -> ImGuiStyleVar_WindowPadding
:frame_padding -> ImGuiStyleVar_FramePadding
:item_spacing -> ImGuiStyleVar_ItemSpacing
# ... all ~30 ImGuiStyleVar_ values
```

**Safety:** The renderers must ensure push/pop balance. If a window subtree has unbalanced pushes, force-pop at window end. Track push count per window.

### Named Themes

A theme is a collection of style color/var overrides stored as a finescript map:

```
set dark_fantasy {
    :button [0.15 0.08 0.08 1.0]
    :button_hovered [0.3 0.15 0.15 1.0]
    :window_bg [0.05 0.03 0.03 0.95]
    :frame_bg [0.1 0.06 0.06 1.0]
    :text [0.9 0.85 0.7 1.0]
    :border [0.4 0.25 0.1 1.0]
    :frame_rounding 4.0
    :window_rounding 6.0
}
```

`ui.push_theme` reads the map and emits the appropriate push calls. `ui.pop_theme` pops the correct count.

C++ side: `GuiRenderer` could accept a `Theme` struct or just use the same map approach.

---

## Phase B: Textured Skin System (Future)

Full custom-rendered widgets using texture atlases and `ImDrawList`. This replaces ImGui's default rectangle rendering with textured visuals (stone borders, parchment backgrounds, ornate buttons).

### Architecture

```
SkinAtlas          -- texture atlas of UI elements (borders, fills, icons)
SkinDefinition     -- maps widget states to atlas regions + 9-slice params
SkinRenderer       -- replaces default rendering for specific widget types
```

### SkinAtlas

A single GPU texture containing all UI elements:

```
+-------+-------+-------+
| btn   | btn   | btn   |
| normal| hover | press |
+-------+-------+-------+
| frame | frame | scroll|
| bg    | border| thumb |
+-------+-------+-------+
| window corners / edges|
+-----------------------+
```

Loaded from a texture atlas descriptor (JSON/map) that maps region names to UV rects.

### 9-Slice Rendering

Most UI elements use 9-slice (9-patch) rendering: corners stay fixed size, edges stretch in one direction, center fills. `ImDrawList::AddImage` with UV manipulation.

```cpp
struct NineSlice {
    ImTextureID texture;
    ImVec2 uvMin, uvMax;       // full region in atlas
    float borderLeft, borderRight, borderTop, borderBottom;  // in pixels

    void draw(ImDrawList* dl, ImVec2 min, ImVec2 max, ImU32 tint = IM_COL32_WHITE);
};
```

### SkinDefinition

Maps widget types + states to visual elements:

```cpp
struct WidgetSkin {
    NineSlice background;       // normal state
    NineSlice backgroundHover;  // hovered
    NineSlice backgroundActive; // pressed/active
    NineSlice backgroundDisabled;
    ImVec2 contentPadding;      // padding inside the skin border
    ImFont* font = nullptr;     // optional font override
    ImU32 textColor = IM_COL32_WHITE;
    ImU32 textColorDisabled = IM_COL32(128, 128, 128, 255);
};

struct SkinDefinition {
    WidgetSkin button;
    WidgetSkin windowFrame;
    WidgetSkin windowTitleBar;
    WidgetSkin checkbox;
    WidgetSkin sliderTrack;
    WidgetSkin sliderGrab;
    WidgetSkin inputFrame;
    WidgetSkin scrollbar;
    WidgetSkin scrollbarGrab;
    WidgetSkin tab;
    WidgetSkin tabActive;
    // ... etc
};
```

### Rendering Approach

The key pattern: use `ImGui::InvisibleButton` (or equivalent) for hit testing and interaction, then draw custom visuals via `ImDrawList`:

```cpp
// Instead of ImGui::Button("Attack"):
ImVec2 size(120, 40);
ImGui::InvisibleButton("##attack", size);
bool hovered = ImGui::IsItemHovered();
bool active = ImGui::IsItemActive();

ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 p = ImGui::GetItemRectMin();

auto& skin = active ? skinDef.button.backgroundActive
           : hovered ? skinDef.button.backgroundHover
           : skinDef.button.background;
skin.draw(dl, p, {p.x + size.x, p.y + size.y});

// Centered text
ImVec2 textSize = ImGui::CalcTextSize("Attack");
dl->AddText({p.x + (size.x - textSize.x) * 0.5f, p.y + (size.y - textSize.y) * 0.5f},
            skinDef.button.textColor, "Attack");
```

### Integration with finegui

Two possible approaches:

**A) Skin-aware renderer:** A new `SkinnedRenderer` that replaces `GuiRenderer::renderNode()` dispatch. When a skin is set, buttons/checkboxes/etc use InvisibleButton + draw list instead of the normal ImGui calls. Falls back to default rendering for unskinned widget types.

**B) Widget-level skin override:** Each WidgetNode can optionally carry a skin reference. The existing renderer checks for it and switches rendering path per-widget. More flexible but more complex.

Approach A is simpler and sufficient for most games (one global skin).

### Script Integration

```
# Load skin atlas
set skin {ui.load_skin "dark_fantasy.atlas"}
ui.set_skin skin

# Or per-window
ui.show {ui.window "Inventory" [
    # all widgets in this window use the skin
    ...
] :skin dark_fantasy_skin}
```

### Considerations

- 9-slice rendering adds draw calls (9 quads per element vs 1 for ImGui default)
- Atlas must be a registered GPU texture (use TextureRegistry)
- Font rendering still uses ImGui's font atlas (no custom text rendering needed)
- Animated elements (glow, pulse) possible via tint color modulation per frame
- Cursor skinning: set `io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange` and render custom cursor via draw list

### Priority

Phase A (push/pop) should come first — it's a few hundred lines of code and immediately useful. Phase B (textured skins) is a larger project that should wait until there's an actual game UI that needs it, to avoid designing in a vacuum.
