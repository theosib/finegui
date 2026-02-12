# Textured Skin System — Design Plan

**Status:** Planned (Phase B)
**Depends on:** finegui core, retained-mode layer, script layer

## Overview

The textured skin system replaces ImGui's flat-colored rectangle rendering with artist-authored textures from a sprite atlas, using 9-slice (nine-patch) rendering to scale elements to any size without distorting borders. This is how shipped game UIs achieve a cohesive visual style — buttons with beveled edges, windows with ornate borders, scrollbars that look like carved wood, etc.

The system is designed to be **opt-in and non-breaking**. All existing finegui code continues to work unchanged with the default ImGui style. When a skin is loaded and activated, the retained-mode and map renderers automatically switch to textured drawing for elements that have skin definitions. Elements without skin definitions fall back to stock ImGui rendering, so skins can be partial.

## Architecture

```
  ┌──────────────────────┐
  │   Skin Definition    │  ← finescript map / file
  │  (element → region)  │
  └──────────┬───────────┘
             │ load
  ┌──────────▼───────────┐     ┌──────────────────┐
  │       Skin           │────▶│    SkinAtlas      │
  │ (element mappings,   │     │ (texture handle,  │
  │  padding overrides)  │     │  named regions)   │
  └──────────┬───────────┘     └──────────────────┘
             │
  ┌──────────▼───────────┐
  │    SkinManager       │  ← held by application, passed to renderers
  │ (active skin,        │
  │  skin stack for      │
  │  per-widget override)│
  └──────────┬───────────┘
             │ query during rendering
  ┌──────────▼───────────┐
  │  Widget Renderer /   │
  │  Map Renderer        │
  │ (draws 9-slice       │
  │  instead of flat)    │
  └──────────────────────┘
```

## Data Model

### SliceInsets

Defines the non-stretching border widths for 9-slice rendering.

```cpp
struct SliceInsets {
    float left = 0, right = 0, top = 0, bottom = 0;
};
```

### AtlasRegion

A rectangular region within the atlas texture, with 9-slice insets.

```cpp
struct AtlasRegion {
    float x, y, w, h;          // Pixel rect in atlas
    SliceInsets insets;         // 9-slice border widths
    uint32_t atlasWidth;       // Atlas dimensions (for UV calc)
    uint32_t atlasHeight;

    ImVec2 uvMin() const;      // Top-left UV
    ImVec2 uvMax() const;      // Bottom-right UV
};
```

### SkinElementState

Each skinned element can have different textures per interaction state.

```cpp
enum class SkinElementState {
    Normal,
    Hovered,
    Active,     // Pressed / currently interacting
    Disabled,
    Focused,    // Keyboard focus ring
    Checked,    // Checkbox / radio checked state
    Unchecked   // Checkbox / radio unchecked state
};
```

### SkinElementType

Enumerates every visual piece of every widget type that can be skinned.

```cpp
enum class SkinElementType {
    // --- Window chrome ---
    WindowBackground,
    WindowTitleBar,
    WindowTitleBarCollapsed,
    WindowCloseButton,
    WindowResizeGrip,
    WindowBorder,

    // --- Buttons ---
    Button,
    SmallButton,
    ImageButtonFrame,
    ColorButtonFrame,

    // --- Input frames ---
    // Shared background for InputText, InputInt, InputFloat, Combo closed,
    // InputMultiline, InputWithHint, ColorEdit, DragFloat, DragInt, etc.
    FrameBackground,

    // --- Checkboxes & Radio ---
    Checkbox,           // states: Normal/Hovered + Checked/Unchecked
    RadioButton,        // states: Normal/Hovered + Checked/Unchecked

    // --- Sliders ---
    SliderTrack,
    SliderThumb,

    // --- Scrollbars ---
    ScrollbarBackground,
    ScrollbarThumb,

    // --- Progress ---
    ProgressBarBackground,
    ProgressBarFill,

    // --- Tabs ---
    TabBarBackground,
    TabInactive,
    TabActive,
    TabHovered,

    // --- Trees & Headers ---
    CollapsingHeaderBackground,
    TreeNodeBackground,
    TreeExpandArrow,

    // --- Tables ---
    TableHeaderBackground,
    TableRowBackground,
    TableRowBackgroundAlt,

    // --- Menus ---
    MenuBarBackground,
    MenuBackground,
    MenuItemHovered,

    // --- Popups & Modals ---
    PopupBackground,
    ModalBackground,
    ModalDimOverlay,    // Semi-transparent overlay behind modal

    // --- Tooltips ---
    TooltipBackground,

    // --- Misc ---
    Separator,
    SelectableBackground,
    ChildBackground,    // Scrollable child region

    // --- Combo dropdown ---
    ComboDropdownBackground,
    ComboArrow,

    // --- Listbox ---
    ListBoxBackground,
};
```

### SkinElement

Bundles the per-state atlas regions for one visual element, plus optional overrides.

```cpp
struct SkinElement {
    // State → atlas region mapping. Not all states are required.
    // Lookup falls back: Active → Hovered → Normal → (skip skinning)
    std::unordered_map<SkinElementState, AtlasRegion> states;

    // Content padding override (pixels). If non-zero, replaces ImGui's
    // default padding for this element. Order: left, top, right, bottom.
    float paddingLeft = 0, paddingTop = 0, paddingRight = 0, paddingBottom = 0;
    bool hasPadding = false;

    // Color tint multiplied with the texture (default white = no tint).
    // Allows reusing one texture for multiple color variants.
    ImU32 tint = IM_COL32(255, 255, 255, 255);

    // Minimum size — prevents the element from being smaller than the
    // 9-slice borders (which would cause visual artifacts).
    float minWidth = 0, minHeight = 0;

    /// Get the region for a given state, with fallback chain.
    const AtlasRegion* get(SkinElementState state) const;
};
```

**Fallback chain:** `Active → Hovered → Normal`. If `Normal` is not defined, the element is unskinned and falls back to default ImGui rendering.

## Core Classes

### SkinAtlas

Manages the atlas texture and named regions.

```cpp
class SkinAtlas {
public:
    SkinAtlas() = default;

    /// Load atlas image from file. Registers with GuiSystem.
    void loadTexture(GuiSystem& gui, const std::string& imagePath);

    /// Use a pre-registered texture handle.
    void setTexture(TextureHandle handle, uint32_t atlasWidth, uint32_t atlasHeight);

    /// Define a named region within the atlas.
    void defineRegion(const std::string& name,
                      float x, float y, float w, float h,
                      const SliceInsets& insets = {});

    /// Look up a region by name. Returns nullptr if not found.
    const AtlasRegion* getRegion(const std::string& name) const;

    /// Check if a region exists.
    bool hasRegion(const std::string& name) const;

    /// Atlas texture handle (for ImDrawList::AddImage).
    TextureHandle texture() const;
    uint32_t width() const;
    uint32_t height() const;

private:
    TextureHandle texture_;
    uint32_t atlasWidth_ = 0, atlasHeight_ = 0;
    std::unordered_map<std::string, AtlasRegion> regions_;
};
```

### Skin

A complete skin definition: maps element types to atlas regions.

```cpp
class Skin {
public:
    Skin() = default;
    explicit Skin(const std::string& name);

    const std::string& name() const;

    /// Set the atlas this skin uses.
    void setAtlas(SkinAtlas* atlas);
    SkinAtlas* atlas() const;

    /// Define a skinned element.
    void setElement(SkinElementType type, const SkinElement& element);

    /// Convenience: define a single-state element by region name.
    void setElement(SkinElementType type, SkinElementState state,
                    const std::string& regionName);

    /// Look up an element. Returns nullptr if this element is not skinned.
    const SkinElement* getElement(SkinElementType type) const;

    /// Named custom elements for per-widget overrides.
    /// Widgets reference these by name via the `:skin` field.
    void setCustomElement(const std::string& name, const SkinElement& element);
    const SkinElement* getCustomElement(const std::string& name) const;

    /// Font override (optional). If set, pushed during skinned rendering.
    void setFont(ImFont* font);
    ImFont* font() const;

    /// Load a complete skin from a finescript definition file.
    static std::unique_ptr<Skin> loadFromFile(
        const std::string& path, SkinAtlas& atlas,
        finescript::ScriptEngine& engine);

    /// Load from an already-parsed finescript map.
    static std::unique_ptr<Skin> loadFromMap(
        const finescript::Value& map, SkinAtlas& atlas);

private:
    std::string name_;
    SkinAtlas* atlas_ = nullptr;
    ImFont* font_ = nullptr;
    std::unordered_map<SkinElementType, SkinElement> elements_;
    std::unordered_map<std::string, SkinElement> customElements_;
};
```

### SkinManager

Runtime management: active skin, skin stack for per-widget overrides.

```cpp
class SkinManager {
public:
    SkinManager() = default;

    // --- Skin registration ---

    void registerSkin(const std::string& name, std::unique_ptr<Skin> skin);
    void unregisterSkin(const std::string& name);
    Skin* getSkin(const std::string& name) const;

    // --- Active skin ---

    /// Set the globally active skin. Pass "" to disable skinning.
    void setActiveSkin(const std::string& name);
    Skin* activeSkin() const;
    const std::string& activeSkinName() const;

    /// Is any skin active?
    bool hasSkin() const;

    // --- Per-widget override stack ---

    /// Push a named skin onto the stack (for push_skin/pop_skin widgets).
    void pushSkin(const std::string& name);
    void popSkin();

    // --- Query during rendering ---

    /// Get the effective skin (top of stack, or active skin).
    Skin* effectiveSkin() const;

    /// Convenience: look up an element from the effective skin.
    /// Returns nullptr if no skin is active or element is not defined.
    const SkinElement* getElement(SkinElementType type) const;

    /// Convenience: look up a custom element by name.
    const SkinElement* getCustomElement(const std::string& name) const;

    /// Atlas texture ID for the effective skin (for ImDrawList calls).
    ImTextureID atlasTextureId() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Skin>> skins_;
    std::string activeSkinName_;
    Skin* activeSkin_ = nullptr;
    std::vector<Skin*> skinStack_;  // push_skin/pop_skin
};
```

### NineSlice (Static Utility)

The 9-slice rendering primitive. Emits 9 textured quads to an `ImDrawList`.

```cpp
namespace NineSlice {

/// Draw a 9-slice textured rectangle.
///
/// Divides the source region into a 3x3 grid using `insets`:
///   - 4 corners: fixed size, no stretching
///   - 4 edges: stretch in one axis
///   - 1 center: stretches in both axes
///
/// If the target rect is smaller than the insets, corners are clamped
/// to prevent overlap (graceful degradation).
void draw(ImDrawList* drawList,
          ImTextureID tex,
          const ImVec2& posMin, const ImVec2& posMax,
          const AtlasRegion& region,
          ImU32 tint = IM_COL32_WHITE);

/// Draw with explicit UV bounds and insets (lower-level).
void drawUV(ImDrawList* drawList,
            ImTextureID tex,
            const ImVec2& posMin, const ImVec2& posMax,
            const ImVec2& uvMin, const ImVec2& uvMax,
            const SliceInsets& insets,   // in pixels
            float regionWidth,           // for pixel→UV conversion
            float regionHeight,
            ImU32 tint = IM_COL32_WHITE);

} // namespace NineSlice
```

## Rendering Integration

### Approach: Widget-Level Replacement

When a skin is active, the retained-mode widget renderer and map renderer replace their standard ImGui calls with skinned equivalents. The pattern for each widget:

1. **Query** the SkinManager for the relevant `SkinElement`
2. **Determine state** (hovered, active, disabled) from ImGui state queries
3. **Calculate layout** — use ImGui's layout system for positioning, but apply skin padding
4. **Draw background** — 9-slice textured quad via `NineSlice::draw()`
5. **Handle interaction** — `ImGui::InvisibleButton()` or equivalent for hit testing
6. **Draw foreground** — text, icons via `ImDrawList::AddText()` / `AddImage()`
7. **Fallback** — if no skin element is defined, call the standard ImGui function

### Example: Skinned Button

```cpp
void renderButton(WidgetNode& node, SkinManager* skinMgr) {
    const SkinElement* elem = skinMgr ? skinMgr->getElement(SkinElementType::Button) : nullptr;

    // Check for per-widget skin override
    if (skinMgr && !node.skinOverride.empty()) {
        elem = skinMgr->getCustomElement(node.skinOverride);
    }

    if (elem && elem->get(SkinElementState::Normal)) {
        // --- Skinned path ---

        // Determine state
        auto state = SkinElementState::Normal;
        if (!node.enabled) state = SkinElementState::Disabled;

        // Calculate size with skin padding
        ImVec2 textSize = ImGui::CalcTextSize(node.label.c_str());
        float padL = elem->hasPadding ? elem->paddingLeft : ImGui::GetStyle().FramePadding.x;
        float padR = elem->hasPadding ? elem->paddingRight : ImGui::GetStyle().FramePadding.x;
        float padT = elem->hasPadding ? elem->paddingTop : ImGui::GetStyle().FramePadding.y;
        float padB = elem->hasPadding ? elem->paddingBottom : ImGui::GetStyle().FramePadding.y;
        ImVec2 size(textSize.x + padL + padR, textSize.y + padT + padB);

        // Enforce minimum size
        size.x = std::max(size.x, elem->minWidth);
        size.y = std::max(size.y, elem->minHeight);

        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Interaction (invisible — skin provides visuals)
        bool clicked = ImGui::InvisibleButton(node.label.c_str(), size);
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        if (hovered && !active) state = SkinElementState::Hovered;
        if (active) state = SkinElementState::Active;

        // Draw 9-slice background
        const AtlasRegion* region = elem->get(state);
        if (region) {
            NineSlice::draw(ImGui::GetWindowDrawList(),
                           skinMgr->atlasTextureId(),
                           pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           *region, elem->tint);
        }

        // Draw label text centered
        ImVec2 textPos(pos.x + (size.x - textSize.x) * 0.5f,
                       pos.y + (size.y - textSize.y) * 0.5f);
        ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text),
                                            node.label.c_str());

        if (clicked && node.onClick) node.onClick(node);
    } else {
        // --- Fallback: standard ImGui button ---
        if (ImGui::Button(node.label.c_str()) && node.onClick) {
            node.onClick(node);
        }
    }
}
```

### Widgets Requiring Skinned Rendering

Each widget type needs a skinned code path. Complexity varies:

| Widget | Elements Used | Complexity |
|--------|--------------|------------|
| **Window** | WindowBackground, WindowTitleBar, WindowBorder | High — need to replicate ImGui window chrome |
| **Button** | Button (3 states) | Medium |
| **SmallButton** | SmallButton (3 states) | Medium |
| **Checkbox** | Checkbox (Checked/Unchecked × Normal/Hovered) | Medium |
| **RadioButton** | RadioButton (Checked/Unchecked × Normal/Hovered) | Medium |
| **Slider/SliderInt** | SliderTrack + SliderThumb | High — composite element |
| **InputText/Int/Float** | FrameBackground | Low — background only |
| **Combo** | FrameBackground + ComboDropdownBackground + ComboArrow | Medium |
| **ProgressBar** | ProgressBarBackground + ProgressBarFill | Medium — two layered elements |
| **TabBar/Tab** | TabBarBackground + TabActive + TabInactive | Medium |
| **CollapsingHeader** | CollapsingHeaderBackground | Low |
| **TreeNode** | TreeNodeBackground + TreeExpandArrow | Medium |
| **MenuBar/Menu** | MenuBarBackground + MenuBackground + MenuItemHovered | Medium |
| **MenuItem** | MenuItemHovered | Low |
| **Popup/Modal** | PopupBackground / ModalBackground + ModalDimOverlay | Low |
| **Tooltip** | TooltipBackground | Low |
| **Table** | TableHeaderBackground + TableRowBackground | Low |
| **Selectable** | SelectableBackground | Low |
| **ListBox** | ListBoxBackground | Low |
| **Child** | ChildBackground | Low |
| **Scrollbar** | ScrollbarBackground + ScrollbarThumb | High — need custom scrollbar |
| **Separator** | Separator | Low |
| **ImageButton** | ImageButtonFrame | Medium |
| **ColorEdit/Picker** | FrameBackground | Low — background only |
| **DragFloat/Int** | FrameBackground | Low |

## Skin Definition Format

Skins are defined as finescript maps, loadable from `.fgs` (finegui skin) files:

```
{
  =name "medieval"
  =atlas "medieval_atlas.png"

  =regions {
    =btn_normal     {=x 0   =y 0   =w 48  =h 48  =inset [8 8 8 8]}
    =btn_hovered    {=x 48  =y 0   =w 48  =h 48  =inset [8 8 8 8]}
    =btn_active     {=x 96  =y 0   =w 48  =h 48  =inset [8 8 8 8]}
    =btn_disabled   {=x 144 =y 0   =w 48  =h 48  =inset [8 8 8 8]}

    =win_bg         {=x 0   =y 48  =w 64  =h 64  =inset [16 16 16 16]}
    =win_title      {=x 0   =y 112 =w 64  =h 32  =inset [16 16 4 4]}

    =frame_bg       {=x 64  =y 48  =w 32  =h 32  =inset [4 4 4 4]}
    =frame_focused  {=x 96  =y 48  =w 32  =h 32  =inset [4 4 4 4]}

    =chk_unchecked  {=x 0   =y 144 =w 24  =h 24  =inset [0 0 0 0]}
    =chk_checked    {=x 24  =y 144 =w 24  =h 24  =inset [0 0 0 0]}

    =slider_track   {=x 0   =y 168 =w 48  =h 12  =inset [6 6 2 2]}
    =slider_thumb   {=x 48  =y 168 =w 16  =h 24  =inset [4 4 4 4]}

    =progress_bg    {=x 0   =y 180 =w 48  =h 16  =inset [4 4 4 4]}
    =progress_fill  {=x 48  =y 180 =w 48  =h 16  =inset [4 4 4 4]}

    =tab_inactive   {=x 0   =y 196 =w 48  =h 28  =inset [8 8 4 0]}
    =tab_active     {=x 48  =y 196 =w 48  =h 28  =inset [8 8 4 0]}

    =popup_bg       {=x 128 =y 48  =w 48  =h 48  =inset [12 12 12 12]}
    =tooltip_bg     {=x 176 =y 48  =w 32  =h 32  =inset [6 6 6 6]}
    =separator      {=x 0   =y 224 =w 48  =h 4   =inset [4 4 0 0]}

    =scroll_bg      {=x 128 =y 96  =w 16  =h 48  =inset [2 2 4 4]}
    =scroll_thumb   {=x 144 =y 96  =w 16  =h 24  =inset [4 4 4 4]}

    =menu_bg        {=x 128 =y 144 =w 48  =h 48  =inset [8 8 8 8]}
    =menu_highlight {=x 176 =y 144 =w 48  =h 24  =inset [4 4 4 4]}
  }

  =elements {
    =button {
      =normal "btn_normal"
      =hovered "btn_hovered"
      =active "btn_active"
      =disabled "btn_disabled"
      =padding [12 8 12 8]
    }

    =window_background {
      =normal "win_bg"
    }

    =window_title_bar {
      =normal "win_title"
      =padding [16 6 16 6]
    }

    =frame_background {
      =normal "frame_bg"
      =focused "frame_focused"
      =padding [6 4 6 4]
    }

    =checkbox {
      =unchecked "chk_unchecked"
      =checked "chk_checked"
    }

    =slider_track {
      =normal "slider_track"
    }

    =slider_thumb {
      =normal "slider_thumb"
      =hovered "slider_thumb"
    }

    =progress_bar_background {
      =normal "progress_bg"
    }

    =progress_bar_fill {
      =normal "progress_fill"
      =tint [0.2 0.8 0.3 1.0]
    }

    =tab_inactive {
      =normal "tab_inactive"
    }

    =tab_active {
      =normal "tab_active"
    }

    =popup_background {
      =normal "popup_bg"
    }

    =tooltip_background {
      =normal "tooltip_bg"
    }

    =separator {
      =normal "separator"
    }

    =scrollbar_background {
      =normal "scroll_bg"
    }

    =scrollbar_thumb {
      =normal "scroll_thumb"
      =hovered "scroll_thumb"
    }

    =menu_background {
      =normal "menu_bg"
    }

    =menu_item_hovered {
      =normal "menu_highlight"
    }
  }

  # Optional: custom elements for per-widget overrides
  =custom {
    =stone_button {
      =normal "stone_btn_normal"
      =hovered "stone_btn_hovered"
      =active "stone_btn_active"
      =padding [16 10 16 10]
    }
  }
}
```

## C++ API Usage

### Loading and Activating a Skin

```cpp
#include <finegui/skin_manager.hpp>
#include <finegui/skin_atlas.hpp>
#include <finegui/skin.hpp>

// Create atlas — load image and register as texture
SkinAtlas atlas;
atlas.loadTexture(gui, "assets/ui/medieval_atlas.png");

// Load skin definition
auto skin = Skin::loadFromFile("assets/ui/medieval.fgs", atlas, engine);

// Register with manager
SkinManager skinMgr;
skinMgr.registerSkin("medieval", std::move(skin));
skinMgr.setActiveSkin("medieval");

// Connect to renderers
guiRenderer.setSkinManager(&skinMgr);
// and/or
mapRenderer.setSkinManager(&skinMgr);
```

### Disabling Skinning

```cpp
// Disable — all widgets revert to standard ImGui rendering
skinMgr.setActiveSkin("");
```

### Per-Widget Skin Override (Retained Mode)

```cpp
auto btn = WidgetNode::button("Craft", onClick);
btn.skinOverride = "stone_button";  // Use custom element from skin
```

### Per-Widget Skin Override (Tree Scope)

```cpp
auto win = WidgetNode::window("Special", {
    WidgetNode::pushSkin("alternate_skin"),
    WidgetNode::button("A"),
    WidgetNode::button("B"),
    WidgetNode::popSkin(),
    WidgetNode::button("C"),  // Uses default skin
});
```

### Runtime Skin Switching

```cpp
skinMgr.setActiveSkin("sci_fi");  // Immediate switch, affects next frame
```

### Programmatic Skin Definition (No File)

```cpp
SkinAtlas atlas;
atlas.setTexture(myTextureHandle, 512, 512);
atlas.defineRegion("btn", 0, 0, 48, 48, {8, 8, 8, 8});
atlas.defineRegion("btn_hover", 48, 0, 48, 48, {8, 8, 8, 8});

auto skin = std::make_unique<Skin>("custom");
skin->setAtlas(&atlas);
skin->setElement(SkinElementType::Button, SkinElementState::Normal, "btn");
skin->setElement(SkinElementType::Button, SkinElementState::Hovered, "btn_hover");

skinMgr.registerSkin("custom", std::move(skin));
skinMgr.setActiveSkin("custom");
```

## Script API

### Loading and Switching Skins

```
# Load a skin from a definition file (registers it by name)
ui.load_skin "medieval" "assets/ui/medieval.fgs"

# Activate a skin (affects all widgets)
ui.set_skin "medieval"

# Disable skinning
ui.set_skin nil

# Switch skins dynamically
ui.button "Medieval" fn [] do
    ui.set_skin "medieval"
end
ui.button "Sci-Fi" fn [] do
    ui.set_skin "sci_fi"
end
ui.button "Default" fn [] do
    ui.set_skin nil
end
```

### Per-Widget Skin Override

```
# Use a custom skin element for one widget
{ui.button "Craft" :skin "stone_button" fn [] do
    print "Crafting..."
end}
```

### Scoped Skin Override (push/pop)

```
ui.show {ui.window "Mixed" [
    {ui.push_skin "alternate"}
    {ui.button "Alternate-styled A"}
    {ui.button "Alternate-styled B"}
    {ui.pop_skin}
    {ui.button "Default-styled C"}
]}
```

## New WidgetNode Types

```cpp
// New widget types for skin push/pop
enum class Type {
    // ... existing types ...
    PushSkin,   // Push a named skin onto the stack
    PopSkin,    // Pop the skin stack
};

// New WidgetNode field
std::string skinOverride;  // Custom skin element name (empty = use default)
```

Builders:

```cpp
static WidgetNode pushSkin(std::string skinName);
static WidgetNode popSkin();
```

## New Files

| File | Description |
|------|-------------|
| `include/finegui/nine_slice.hpp` | NineSlice static draw utility |
| `include/finegui/skin_atlas.hpp` | SkinAtlas class |
| `include/finegui/skin_element.hpp` | SliceInsets, AtlasRegion, SkinElementState, SkinElementType, SkinElement |
| `include/finegui/skin.hpp` | Skin class |
| `include/finegui/skin_manager.hpp` | SkinManager class |
| `src/retained/nine_slice.cpp` | 9-slice rendering implementation |
| `src/retained/skin_atlas.cpp` | Atlas loading + region management |
| `src/retained/skin.cpp` | Skin definition + file loading |
| `src/retained/skin_manager.cpp` | Runtime skin management |

## Modified Files

| File | Changes |
|------|---------|
| `src/retained/widget_renderer.cpp` | Add skinned rendering path for each widget type |
| `src/retained/map_renderer.cpp` | Add skinned rendering path for map-based widgets |
| `include/finegui/widget_node.hpp` | Add `PushSkin`/`PopSkin` types, `skinOverride` field |
| `include/finegui/gui_renderer.hpp` | Add `setSkinManager()` |
| `include/finegui/map_renderer.hpp` | Add `setSkinManager()` |
| `src/script/script_bindings.cpp` | Add `ui.load_skin`, `ui.set_skin`, `ui.push_skin`, `ui.pop_skin` bindings |
| `src/script/widget_converter.cpp` | Handle `:skin` field on widget maps |
| `CMakeLists.txt` | Add new source files |

## Implementation Phases

### Phase 1: Foundation (Data Model + 9-Slice)

- `SliceInsets`, `AtlasRegion`, `SkinElementState`, `SkinElementType`, `SkinElement`
- `NineSlice::draw()` and `NineSlice::drawUV()`
- `SkinAtlas` (texture + named regions)
- `Skin` (element mappings, custom elements)
- `SkinManager` (registration, active skin, skin stack)
- Unit tests for 9-slice UV calculation, atlas region lookup, element fallback chain
- No rendering integration yet — just data model and utilities

### Phase 2: Core Widget Skinning

Skinned rendering for the most impactful widgets:

- **Button** (normal/hovered/active/disabled)
- **Window** (background, title bar)
- **FrameBackground** (InputText, InputInt, InputFloat, Combo, DragFloat/Int)
- **Checkbox** (checked/unchecked × normal/hovered)
- **Slider** (track + thumb)
- **ProgressBar** (background + fill)
- Connect `SkinManager` to `GuiRenderer` and `MapRenderer`

### Phase 3: Complete Widget Coverage

- RadioButton, Selectable, TabBar/Tab, TreeNode, CollapsingHeader
- MenuBar, Menu, MenuItem
- Popup, Modal, Tooltip
- Table header/row backgrounds
- Scrollbar (track + thumb)
- Separator, Child background
- ListBox, ComboDropdown
- SmallButton, ImageButton frame, ColorButton frame

### Phase 4: Script Integration + Definition Loading

- `Skin::loadFromFile()` / `Skin::loadFromMap()` — parse finescript skin definitions
- `ui.load_skin`, `ui.set_skin` script bindings
- `ui.push_skin`, `ui.pop_skin` widget builders in script
- `:skin` field on widget maps
- `PushSkin`/`PopSkin` widget types + builders

### Phase 5: Polish

- Graceful degradation when target rect is smaller than 9-slice insets
- Font override per skin
- Tint support (color multiply on texture)
- Documentation and examples
- Atlas packing guidelines / recommended workflow

## Design Decisions

### Why widget-level replacement instead of draw list post-processing?

Post-processing ImGui's draw list is fragile: you'd need to identify which rectangles correspond to which widgets, which ImGui doesn't track. It also breaks when ImGui changes internal draw order between versions. Widget-level replacement is explicit, predictable, and gives full control over sizing/padding.

### Why a single atlas texture?

Multiple textures would require texture switches mid-frame, which breaks ImGui's batching and adds draw calls. A single atlas keeps everything in one draw call per window, matching ImGui's performance model.

### Why finescript for skin definitions?

- Consistent with finegui's script layer
- No additional parser dependency (JSON would require a JSON library)
- Supports comments (unlike JSON)
- Can be evaluated by the script engine, enabling dynamic skin generation
- Maps directly to the `Skin::loadFromMap()` API

### Why fallback to standard ImGui rendering?

Skins should be incremental. A game might only want to skin buttons and windows but leave checkboxes as default. Partial skins also make development easier — you can skin one widget type at a time and see results immediately.

## Migration Guide for Existing Projects

**No migration required.** The skin system is entirely opt-in:

1. If you don't create a `SkinManager`, nothing changes
2. If you don't call `setSkinManager()` on renderers, nothing changes
3. If a skin is active but doesn't define an element for a widget type, that widget renders normally

To adopt skinning:

1. Create your atlas image (any image editor or texture packer)
2. Write a `.fgs` skin definition file
3. Load the atlas and skin at startup
4. Call `setSkinManager(&skinMgr)` on your renderers
5. Set the active skin

Widgets that have skin definitions will render textured; others keep the default ImGui look. You can incrementally add more element definitions to your skin file as you create the artwork.
