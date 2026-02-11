# finegui Feature Checklist

## Widget Primitives

### Layout
- [x] same_line
- [x] spacing
- [x] indent / unindent
- [x] dummy
- [x] new_line

### Display
- [x] text
- [x] text_colored
- [x] text_wrapped
- [x] text_disabled
- [x] bullet_text
- [x] separator
- [x] separator_text
- [x] progress_bar
- [x] plot_lines
- [x] plot_histogram

### Core Input
- [x] button
- [x] checkbox
- [x] slider (float)
- [x] slider_int
- [x] input_text
- [x] input_int
- [x] input_float
- [x] combo
- [x] listbox

### Advanced Input
- [x] radio_button
- [x] selectable
- [x] input_multiline
- [x] color_edit
- [x] color_picker
- [x] drag_float
- [x] drag_int
- [x] drag_float3
- [x] input_with_hint (placeholder text)
- [x] slider_angle
- [x] small_button
- [x] color_button (color swatch display)

### Containers
- [x] window
- [x] group
- [x] columns
- [x] collapsing_header
- [x] tree_node
- [x] tab_bar / tab
- [x] child (scroll region)

### Tables
- [x] table
- [x] table_row
- [x] table_next_column

### Menus & Popups
- [x] menu_bar
- [x] menu
- [x] menu_item
- [x] popup
- [x] modal
- [x] context_menu (right-click)
- [x] main_menu_bar (top-level app menu)
- [x] open_popup (script function)
- [x] close_popup (script function)

### Tooltips
- [x] tooltip
- [x] item_tooltip (hover tooltip on previous widget)

### Images
- [x] image (retained mode)
- [x] image (script/MapRenderer path)
- [x] image_button

### Custom Drawing
- [x] canvas (with draw_line, draw_rect, draw_circle, draw_text, draw_triangle)

## Window Control
- [x] window flags (no_resize, no_title_bar, no_move, no_scrollbar, no_collapse, always_auto_resize, no_background, menu_bar)
- [x] window position (via animation fields)
- [ ] window size (programmatic)
- [ ] window flags: no_nav, no_inputs

## Interaction

### Drag & Drop
- [x] DragDropManager (click-to-pick-up mode)
- [x] Traditional ImGui drag-and-drop
- [x] drag_type, drag_data, drop_accept fields
- [x] on_drop, on_drag callbacks
- [x] Three drag modes (both, traditional-only, click-to-pick-only)

### Focus Management
- [x] Tab navigation (focusable flag)
- [x] Auto-focus on window appearance
- [x] on_focus / on_blur callbacks
- [x] Programmatic focus (setFocus)
- [x] Widget search by ID (findById)

## Animation & Tweening
- [x] TweenManager with easing functions (Linear, EaseIn, EaseOut, EaseInOut, CubicOut, ElasticOut, BounceOut)
- [x] Property tweening (Alpha, PosX, PosY, FloatValue, IntValue, Color RGBA, Width, Height, ScaleX, ScaleY, RotationY)
- [x] Convenience: fadeIn, fadeOut, slideTo, colorTo, shake
- [x] Convenience: zoomIn, zoomOut, flipY, flipYBack
- [x] Vertex post-processing for scale/rotation transforms
- [x] Completion callbacks

## Style & Theming
- [x] PushStyleColor / PopStyleColor (retained mode)
- [x] PushStyleVar / PopStyleVar (retained mode)
- [ ] Named theme presets (push_theme / pop_theme)
- [ ] Script-accessible theme switching (Dark/Light/Classic)
- [ ] Textured skin system (9-slice, SkinAtlas, SkinnedRenderer) — Phase B, future

## Script Integration
- [x] finescript ui.* builder functions
- [x] MapRenderer for map-based UI
- [x] Widget converter (map -> WidgetNode)
- [x] Callback support (on_click, on_change, on_submit, on_close, on_select)
- [x] Canvas draw commands in script
- [x] Symbol ID support in findById / ui.find
- [ ] ui.open_popup / ui.close_popup functions
- [ ] String interpolation in widget text (requires finescript feature)

## Rendering & Backend
- [x] Vulkan (finevk) backend
- [x] ImGui 1.92+ texture lifecycle (WantCreate, WantUpdates, WantDestroy)
- [x] High-DPI / Retina support
- [x] TextureRegistry for dynamic textures
- [x] Threaded rendering (GuiDrawData capture)
- [x] RenderSurface abstraction

## 3D in GUI
- [ ] Offscreen render to texture (render 3D scene to finevk RenderTarget, display via Image widget)
- [ ] Custom render pass in Canvas

## General Polish
- [ ] Keyboard shortcuts / hotkeys (bind keys to actions)
- [ ] Serialization (save/load UI layout state)
- [ ] API reference documentation for all widget types and script bindings
