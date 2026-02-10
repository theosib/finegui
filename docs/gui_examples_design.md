# GUI Examples Design Specification

Comprehensive pseudo-scripts for all planned GUI use cases. Each script uses
real finescript syntax and identifies what new widget primitives, C++ support
functions, and language features it requires.

**Conventions used in these scripts:**
- `ui.*` functions that don't exist yet are marked with `# NEW`
- C++ support functions (game data, textures) prefixed with `game.*`
- Named parameters use `=name value` syntax (e.g., `=size [600 400]`)
- Map literals use `{=key value ...}` syntax
- Data symbols use `:name` (e.g., `:no_resize`, `:button`)
- `??` for null coalescing, `%` for string formatting
- The renderer skips nil children (for conditional rendering)

---

## Table of Contents

### Game UI
1. [Debug Overlay](#1-debug-overlay)
2. [HUD](#2-hud)
3. [Inventory Grid](#3-inventory-grid)
4. [Crafting / Furnace UI](#4-crafting--furnace-ui)
5. [Settings Menu](#5-settings-menu)
6. [Chat Window](#6-chat-window)
7. [Quest Log / Journal](#7-quest-log--journal)
8. [Character Stats](#8-character-stats)
9. [Dialog / NPC Interaction](#9-dialog--npc-interaction)
10. [Hotbar / Action Bar](#10-hotbar--action-bar)
11. [Trading / Shop](#11-trading--shop)
12. [Notification / Toast System](#12-notification--toast-system)
13. [Functional Block UI (Furnace)](#13-functional-block-ui)
14. [World Map](#14-world-map)
15. [Party / Social Panel](#15-party--social-panel)

### World Building Tools
16. [Texture Painter](#16-texture-painter)
17. [Block Shape Editor](#17-block-shape-editor)
18. [Terrain Brush Editor](#18-terrain-brush-editor)
19. [Entity / Component Editor](#19-entity--component-editor)
20. [Particle Effect Editor](#20-particle-effect-editor)

### Game Engine / Designer
21. [Asset Browser](#21-asset-browser)
22. [Script Editor](#22-script-editor)
23. [Property Inspector](#23-property-inspector)
24. [Scene Hierarchy](#24-scene-hierarchy)
25. [Timeline / Animation Editor](#25-timeline--animation-editor)
26. [Console / Log Window](#26-console--log-window)
27. [Node Graph Editor](#27-node-graph-editor)
28. [Menu Bar](#28-menu-bar)
29. [Toolbar](#29-toolbar)
30. [Status Bar](#30-status-bar)

---

## Game UI

### 1. Debug Overlay

A minimal always-visible overlay showing performance stats with collapsible
detail sections.

```finescript
# C++ provides: fps, frame_ms, draw_calls, tri_count, entity_count,
#               chunk_count, player_pos, fps_history (array of recent values)

set win {ui.window "Debug" [
    {ui.text "FPS: {fps}"}                                     # string interpolation
    {ui.same_line}                                             # NEW
    {ui.text "({frame_ms:.1f} ms)"}
    {ui.progress_bar (fps / 120.0) =overlay "{fps:.0f} fps"}  # NEW
    {ui.plot_lines "##fps" fps_history =height 50}             # NEW
    {ui.separator}
    {ui.collapsing_header "Renderer" [                         # NEW
        {ui.text "Draw calls: {draw_calls}"}
        {ui.text "Triangles: {tri_count}"}
    ]}
    {ui.collapsing_header "World" [
        {ui.text "Entities: {entity_count}"}
        {ui.text "Chunks loaded: {chunk_count}"}
        {ui.text "Player: ({player_pos.x:.1f}, {player_pos.y:.1f}, {player_pos.z:.1f})"}
    ]}
]}
set win.pos [10 10]
set win.flags [:no_resize :no_title_bar :always_auto_resize]
ui.show win
```

**New widgets:** same_line, progress_bar, plot_lines, collapsing_header
**New widget features:** window pos, window flags
**C++ support:** Game state variables updated each frame
**Language needs:** Format specifiers in interpolation (`:1f`, `:.0f`)

---

### 2. HUD

Health/mana bars, level display, and minimap thumbnail. Positioned overlay
with no window chrome.

```finescript
# C++ provides: player.health, player.max_health, player.mana, player.max_mana,
#               player.level, player.xp, player.xp_to_next, minimap_texture

set hud_win {ui.window "##hud" [
    # Health bar
    {ui.text_colored [1.0 0.3 0.3 1.0] "HP"}                 # NEW
    {ui.same_line}
    {ui.progress_bar (player.health / player.max_health)
        =overlay "{player.health}/{player.max_health}"
        =size [200 20]}

    # Mana bar
    {ui.text_colored [0.3 0.5 1.0 1.0] "MP"}
    {ui.same_line}
    {ui.progress_bar (player.mana / player.max_mana)
        =overlay "{player.mana}/{player.max_mana}"
        =size [200 20]}

    # XP bar
    {ui.text "Lv.{player.level}"}
    {ui.same_line}
    {ui.progress_bar (player.xp / player.xp_to_next)
        =size [200 12]}
]}
set hud_win.pos [10 10]
set hud_win.flags [:no_resize :no_move :no_title_bar :no_background
                   :always_auto_resize :no_nav]
ui.show hud_win

# Minimap (separate positioned window)
set minimap_win {ui.window "##minimap" [
    {ui.image minimap_texture [150 150]}                       # NEW
]}
set minimap_win.pos [screen_width - 160, 10]
set minimap_win.flags [:no_resize :no_move :no_title_bar :no_scrollbar]
ui.show minimap_win
```

**New widgets:** text_colored, image, progress_bar with :size
**New widget features:** window flags (no_background, no_nav), window pos
**C++ support:** Player stats, minimap texture handle, screen dimensions

---

### 3. Inventory Grid

Grid of item slots with drag-drop, tooltips, and context menus.

```finescript
# C++ provides: game.inventory (array of item maps or nil),
#               game.get_icon(item) -> texture, game.use_item(slot),
#               game.drop_item(slot), game.swap_items(a, b)

set SLOT_SIZE 48
set COLS 9
set SLOTS 36

fn render_slot [i] do
    let item {game.get_item i}
    let icon {if (item != nil) {game.get_icon item} empty_icon}

    {ui.group [
        {ui.image_button "slot_{i}" icon [SLOT_SIZE SLOT_SIZE]}    # NEW

        # Tooltip on hover
        {ui.item_tooltip [                                          # NEW
            if (item != nil) do
                {ui.text item.name}
                {ui.text_colored item.rarity_color item.rarity}
                {ui.separator}
                {ui.text_wrapped item.description}                  # NEW
                if (item.stats != nil) do
                    {ui.spacing}                                    # NEW
                    for stat in item.stats do
                        {ui.text "  {stat.name}: +{stat.value}"}
                    end
                end
            else
                {ui.text_disabled "Empty slot"}                     # NEW
            end
        ]}

        # Drag source (pick up item)
        if (item != nil) do
            {ui.drag_source "inventory_item" i [                    # NEW
                {ui.image icon [32 32]}
                {ui.same_line}
                {ui.text item.name}
                if (item.count > 1) do
                    {ui.text " x{item.count}"}
                end
            ]}
        end

        # Drop target (place item)
        {ui.drop_target "inventory_item" fn [payload] do            # NEW
            game.swap_items payload i
        end}

        # Right-click context menu
        if (item != nil) do
            {ui.context_menu [                                      # NEW
                {ui.menu_item "Use" fn [] do game.use_item i end}   # NEW
                {ui.menu_item "Drop" fn [] do game.drop_item i end}
                if item.stackable do
                    {ui.separator}
                    {ui.menu_item "Split Stack..." fn [] do
                        open_split_dialog i
                    end}
                end
            ]}
        end

        # Stack count overlay
        if (item != nil and item.count > 1) do
            # TODO: positioned text overlay via draw list
        end
    ]}
end

ui.show {ui.window "Inventory" [
    {ui.table "inv_grid" COLS =flags [:no_borders] [                # NEW
        for i in 0..SLOTS do
            {ui.table_next_column}                                  # NEW
            {render_slot i}
        end
    ]}
    {ui.separator}
    {ui.text "Gold: {game.gold}"}
]}
```

**New widgets:** image_button, item_tooltip, text_wrapped, text_disabled,
spacing, drag_source, drop_target, context_menu, menu_item, table,
table_next_column
**C++ support:** Item data, icon textures, inventory operations
**Language needs:** Conditional expressions in arrays (nil-skipping)

---

### 4. Crafting / Furnace UI

Functional block with input/output slots, fuel gauge, and progress indicator.

```finescript
# C++ provides: furnace state (input_item, fuel_item, output_item,
#               cook_progress 0-1, fuel_remaining 0-1, is_burning)

fn render_item_slot [id item] do
    let icon {if (item != nil) {game.get_icon item} empty_slot_icon}
    {ui.group [
        {ui.image_button id icon [48 48]}
        {ui.drop_target "inventory_item" fn [payload] do
            game.place_in_slot id payload
        end}
        if (item != nil) do
            {ui.drag_source "inventory_item" id [
                {ui.text item.name}
            ]}
            {ui.item_tooltip [{ui.text item.name}]}
        end
    ]}
end

ui.show {ui.window "Furnace" [
    # Input → Progress → Output
    {render_item_slot "input" furnace.input_item}
    {ui.same_line}
    {ui.text "  "}
    {ui.same_line}
    {ui.progress_bar furnace.cook_progress =size [80 48]
        =overlay {if furnace.is_burning "Smelting..." ""}}
    {ui.same_line}
    {ui.text "  "}
    {ui.same_line}
    {render_item_slot "output" furnace.output_item}

    {ui.separator}

    # Fuel slot + fuel gauge
    {render_item_slot "fuel" furnace.fuel_item}
    {ui.same_line}
    {ui.text "Fuel:"}
    {ui.same_line}
    {ui.progress_bar furnace.fuel_remaining =size [150 20]}
] =flags [:no_resize :always_auto_resize]}
```

**New widgets:** (same as inventory — same_line, image_button, progress_bar,
drag_source, drop_target, item_tooltip)
**C++ support:** Furnace state, slot operations

---

### 5. Settings Menu

Multi-tab settings with video, audio, and controls configuration.

```finescript
# C++ provides: settings map with current values, apply_settings(),
#               reset_defaults(), available_resolutions, keybindings

ui.show {ui.window "Settings" [
    {ui.tab_bar "settings_tabs" [                               # NEW
        {ui.tab "Video" [                                       # NEW
            {ui.combo "Resolution" available_resolutions settings.resolution_idx
                fn [v] do set settings.resolution_idx v end}
            {ui.checkbox "Fullscreen" settings.fullscreen
                fn [v] do set settings.fullscreen v end}
            {ui.checkbox "VSync" settings.vsync
                fn [v] do set settings.vsync v end}
            {ui.separator}
            {ui.slider_int "Render Distance" 4 32 settings.render_dist
                fn [v] do set settings.render_dist v end}
            {ui.slider "Field of View" 60.0 120.0 settings.fov
                fn [v] do set settings.fov v end}
            {ui.combo "Anti-aliasing" ["Off" "FXAA" "MSAA 2x" "MSAA 4x"]
                settings.aa_mode fn [v] do set settings.aa_mode v end}
        ]}

        {ui.tab "Audio" [
            {ui.slider "Master Volume" 0.0 1.0 settings.master_vol
                fn [v] do set settings.master_vol v end}
            {ui.slider "Music" 0.0 1.0 settings.music_vol
                fn [v] do set settings.music_vol v end}
            {ui.slider "Effects" 0.0 1.0 settings.sfx_vol
                fn [v] do set settings.sfx_vol v end}
            {ui.slider "Ambient" 0.0 1.0 settings.ambient_vol
                fn [v] do set settings.ambient_vol v end}
            {ui.separator}
            {ui.checkbox "Subtitles" settings.subtitles
                fn [v] do set settings.subtitles v end}
        ]}

        {ui.tab "Controls" [
            {ui.table "keybinds" 2 =flags [:row_bg :borders_h] [
                {ui.table_headers ["Action" "Key"]}             # NEW
                for b in keybindings do
                    {ui.table_next_row}                         # NEW
                    {ui.table_next_column}
                    {ui.text b.action}
                    {ui.table_next_column}
                    {ui.button b.key fn [] do
                        game.start_key_capture b
                    end}
                end
            ]}
            {ui.separator}
            {ui.slider "Mouse Sensitivity" 0.1 10.0 settings.mouse_sens
                fn [v] do set settings.mouse_sens v end}
            {ui.checkbox "Invert Y Axis" settings.invert_y
                fn [v] do set settings.invert_y v end}
        ]}
    ]}

    {ui.separator}
    {ui.button "Apply" fn [] do game.apply_settings settings end}
    {ui.same_line}
    {ui.button "Reset to Defaults" fn [] do
        set settings {game.default_settings}
    end}
] =size [600 450]}
```

**New widgets:** tab_bar, tab, table, table_headers, table_next_row,
table_next_column
**C++ support:** Settings persistence, key capture modal, display mode enumeration

---

### 6. Chat Window

Scrollable chat with channel tabs, colored messages, and text input.

```finescript
# C++ provides: chat.messages (array), chat.send(channel, text),
#               chat.channels (array of names)

set chat_input ""

fn render_messages [messages] do
    {ui.child "##scroll" =size [0 -30] =auto_scroll true [      # NEW
        for msg in messages do
            {ui.text_colored msg.color
                "[{msg.timestamp}] <{msg.sender}> {msg.text}"}
        end
    ]}
end

ui.show {ui.window "Chat" [
    {ui.tab_bar "chat_tabs" [
        {ui.tab "All" [
            {render_messages chat.all_messages}
        ]}
        {ui.tab "Party" [
            {render_messages chat.party_messages}
        ]}
        {ui.tab "System" [
            {render_messages chat.system_messages}
        ]}
    ]}

    # Input line
    {ui.input "##chat_input" chat_input
        =hint "Type a message..."                                # NEW: hint text
        =on_submit fn [v] do
            chat.send current_channel v
            set chat_input ""
        end}
] =size [400 300] =pos [10 screen_height - 310]}
```

**New widgets:** child (scroll region with auto_scroll), text_colored, input with :hint
**C++ support:** Chat message system, channel filtering

---

### 7. Quest Log / Journal

Collapsible quest sections with progress tracking.

```finescript
# C++ provides: quests (array), each has: name, description, objectives (array),
#               status (:active/:complete/:failed), objectives have: text, done

fn render_quest [quest] do
    let header_color {match quest.status
        :active [1.0 1.0 1.0 1.0]
        :complete [0.3 1.0 0.3 1.0]
        :failed [1.0 0.3 0.3 0.5]
        _ [0.7 0.7 0.7 1.0]
    end}

    {ui.collapsing_header quest.name [
        {ui.text_wrapped quest.description}
        {ui.separator}
        {ui.text "Objectives:"}
        for obj in quest.objectives do
            {ui.checkbox obj.text obj.done}
        end
        if (quest.status == :complete) do
            {ui.spacing}
            {ui.text_colored [0.3 1.0 0.3 1.0] "COMPLETED"}
        end
    ]}
end

ui.show {ui.window "Quest Log" [
    {ui.text "Active Quests"}
    {ui.separator}
    for q in {quests.filter fn [q] (q.status == :active)} do
        {render_quest q}
    end
    {ui.spacing}
    {ui.spacing}
    {ui.text "Completed Quests"}
    {ui.separator}
    for q in {quests.filter fn [q] (q.status == :complete)} do
        {render_quest q}
    end
] =size [400 500]}
```

**New widgets:** collapsing_header, text_wrapped, spacing
**Language used:** .filter, match, for-in, closures

---

### 8. Character Stats

Tables displaying character attributes, equipment, and skills.

```finescript
# C++ provides: player stats map, equipment slots, skills array

ui.show {ui.window "Character" [
    {ui.tab_bar "char_tabs" [
        {ui.tab "Stats" [
            {ui.text "Level {player.level} {player.class_name}"}
            {ui.separator}
            {ui.table "stats" 2 =flags [:row_bg :borders_h] [
                {ui.table_headers ["Attribute" "Value"]}
                for stat in player.stats do
                    {ui.table_next_row}
                    {ui.table_next_column}
                    {ui.text stat.name}
                    {ui.table_next_column}
                    {ui.text "{stat.value}"}
                    {ui.item_tooltip [
                        {ui.text "{stat.name}: {stat.value} (base: {stat.base})"}
                        if (stat.bonus != 0) do
                            {ui.text_colored [0.3 1.0 0.3 1.0]
                                "+{stat.bonus} from equipment"}
                        end
                    ]}
                end
            ]}
        ]}

        {ui.tab "Equipment" [
            for slot in equipment_slots do
                let item {game.get_equipped slot.id}
                {ui.text "{slot.name}:"}
                {ui.same_line}
                if (item != nil) do
                    {ui.text_colored item.rarity_color item.name}
                    {ui.item_tooltip [
                        {ui.text item.name}
                        {ui.text_colored item.rarity_color item.rarity}
                        {ui.separator}
                        for stat in item.stats do
                            {ui.text "  +{stat.value} {stat.name}"}
                        end
                    ]}
                else
                    {ui.text_disabled "(empty)"}
                end
            end
        ]}

        {ui.tab "Skills" [
            for skill in player.skills do
                {ui.collapsing_header skill.name [
                    {ui.text "Level: {skill.level}/{skill.max_level}"}
                    {ui.progress_bar (skill.xp / skill.xp_next)
                        =overlay "{skill.xp}/{skill.xp_next} XP"}
                    {ui.text_wrapped skill.description}
                ]}
            end
        ]}
    ]}
] =size [450 500]}
```

**New widgets:** table with headers, item_tooltip, text_disabled, same_line,
collapsing_header, progress_bar, text_wrapped, text_colored, tab_bar/tab

---

### 9. Dialog / NPC Interaction

NPC dialog with portrait, text flow, and branching choices.

```finescript
# C++ provides: dialog state (current_node with speaker, text, choices)

fn render_dialog [node] do
    {ui.window "##dialog" [
        # Speaker name
        {ui.text_colored [1.0 0.9 0.5 1.0] node.speaker}
        {ui.separator}

        # Dialog text (wrapped)
        {ui.text_wrapped node.text}

        {ui.spacing}
        {ui.spacing}
        {ui.separator}

        # Choice buttons
        if (node.choices.length > 0) do
            for choice in node.choices do
                {ui.button choice.text fn [] do
                    game.dialog_choose choice.id
                end}
            end
        else
            # Simple continue
            {ui.button "Continue..." fn [] do
                game.dialog_advance
            end}
        end
    ] =flags [:no_resize :no_move :no_collapse :no_title_bar]
      =pos [screen_width / 2 - 250, screen_height - 250]
      =size [500 200]}
end

render_dialog current_dialog
```

**New widgets:** text_wrapped, text_colored, spacing, window pos/size/flags
**C++ support:** Dialog tree, choice handling

---

### 10. Hotbar / Action Bar

Horizontal row of ability/item slots with cooldown display and key labels.

```finescript
# C++ provides: hotbar_slots (array of 10), each has: item, icon, cooldown (0-1),
#               key_label ("1"-"0"), is_active

set SLOT_SIZE 48

fn render_hotbar_slot [i slot] do
    let icon {if (slot.item != nil) slot.icon empty_slot_icon}
    let tint {if (slot.cooldown > 0) [0.5 0.5 0.5 1.0] [1.0 1.0 1.0 1.0]}

    {ui.group [
        # Slot button with tint for cooldown
        {ui.image_button "hb_{i}" icon [SLOT_SIZE SLOT_SIZE] =tint tint}

        # Drop target for dragging items from inventory
        {ui.drop_target "inventory_item" fn [payload] do
            game.set_hotbar i payload
        end}

        # Tooltip
        if (slot.item != nil) do
            {ui.item_tooltip [
                {ui.text slot.item.name}
                if (slot.cooldown > 0) do
                    {ui.text_colored [1.0 0.5 0.5 1.0]
                        "Cooldown: {slot.cooldown:.1f}s"}
                end
            ]}
        end

        # Key label below slot
        {ui.text "  {slot.key_label}"}
    ]}
end

set hotbar_win {ui.window "##hotbar" [
    for i in 0..10 do
        {render_hotbar_slot i hotbar_slots[i]}
        if (i < 9) do
            {ui.same_line}
        end
    end
]}
set hotbar_win.pos [(screen_width - 540) / 2, screen_height - 80]
set hotbar_win.flags [:no_resize :no_move :no_title_bar :no_scrollbar
                      :always_auto_resize :no_background]
ui.show hotbar_win
```

**New widgets:** image_button with :tint, drop_target, item_tooltip,
same_line, text_colored
**C++ support:** Hotbar state, cooldown updates, slot assignment

---

### 11. Trading / Shop

Two-panel interface: shop inventory and player inventory with currency.

```finescript
# C++ provides: shop.items, player.inventory, player.gold,
#               shop.buy(item_id), shop.sell(slot_id), shop.prices

fn render_shop_item [item] do
    {ui.selectable item.name false fn [] do                      # NEW
        set selected_item item
    end}
    {ui.item_tooltip [
        {ui.text item.name}
        {ui.text_colored [1.0 0.84 0.0 1.0] "Price: {item.price}g"}
        {ui.separator}
        {ui.text_wrapped item.description}
    ]}
end

ui.show {ui.window "Shop" [
    # Two columns: shop left, player right
    {ui.child "shop_panel" =size [250 -50] =border true [
        {ui.text "Shop Items"}
        {ui.separator}
        for item in shop.items do
            {render_shop_item item}
        end
    ]}
    {ui.same_line}
    {ui.child "player_panel" =size [250 -50] =border true [
        {ui.text "Your Items"}
        {ui.separator}
        for i in 0..player.inventory.length do
            let item player.inventory[i]
            if (item != nil) do
                {ui.selectable item.name false fn [] do
                    set selected_sell_item i
                end}
            end
        end
    ]}

    {ui.separator}

    # Transaction bar
    {ui.text_colored [1.0 0.84 0.0 1.0] "Gold: {player.gold}"}
    {ui.same_line}
    {ui.button "Buy" fn [] do
        if (selected_item != nil) do
            shop.buy selected_item.id
        end
    end}
    {ui.same_line}
    {ui.button "Sell" fn [] do
        if (selected_sell_item != nil) do
            shop.sell selected_sell_item
        end
    end}
] =size [550 400]}
```

**New widgets:** selectable, child with :border, text_colored

---

### 12. Notification / Toast System

Auto-positioned, auto-dismissing notification popups.

```finescript
# C++ provides: notifications (managed array), each has: text, color,
#               duration, elapsed. C++ removes expired ones.

# Called from C++ each frame to rebuild notification stack
fn render_notifications [] do
    let y_offset 10
    for i in 0..notifications.length do
        let notif notifications[i]
        let alpha {max 0.0 (1.0 - notif.elapsed / notif.duration)}

        set win {ui.window "##notif_{i}" [
            {ui.text_colored [notif.color[0] notif.color[1]
                              notif.color[2] alpha] notif.text}
        ]}
        set win.pos [screen_width - 310, y_offset]
        set win.flags [:no_resize :no_move :no_title_bar :no_scrollbar
                       :always_auto_resize :no_inputs :no_nav]
        set win.bg_alpha alpha
        ui.show win

        set y_offset (y_offset + 40)
    end
end
```

**New widgets:** window bg_alpha control
**C++ support:** Notification manager (add/expire/array management), timer

---

### 13. Functional Block UI

Generic functional block with configurable slots (chest, enchanting table, etc.).

```finescript
# This is a reusable component function.
# block_def describes the layout: rows of slots with labels.

fn render_block_ui [block_def block_state] do
    {ui.window block_def.title [
        for row in block_def.rows do
            for slot in row.slots do
                let item {block_state.get_slot slot.id}
                let icon {if (item != nil) {game.get_icon item} empty_slot_icon}

                {ui.image_button slot.id icon [48 48]}
                {ui.drop_target "inventory_item" fn [payload] do
                    block_state.place slot.id payload
                end}
                if (item != nil) do
                    {ui.drag_source "inventory_item" slot.id [
                        {ui.text item.name}
                    ]}
                    {ui.item_tooltip [{ui.text item.name}]}
                end
                {ui.same_line}
            end

            # Label at end of row
            if (row.label != nil) do
                {ui.text row.label}
            end
        end

        # Progress bars if any
        for prog in block_def.progress_bars do
            {ui.text prog.label}
            {ui.same_line}
            {ui.progress_bar {block_state.get_progress prog.id}
                =size [prog.width 20]}
        end
    ] =flags [:no_resize :always_auto_resize]}
end

# Example: Furnace definition
set furnace_def {=title "Furnace"
    =rows [
        {=slots [{=id "input"}] =label nil}
        {=slots [{=id "fuel"}] =label "Fuel"}
    ]
    =progress_bars [
        {=id "cook" =label "Progress:" =width 150}
        {=id "fuel" =label "Fuel:" =width 150}
    ]
}

render_block_ui furnace_def furnace_state
```

**New widgets:** (reuses previous ones)
**Key pattern:** Data-driven UI — block_def map describes layout, function renders it

---

### 14. World Map

Canvas-based map with markers, region labels, and zoom controls.

```finescript
# C++ provides: map_texture, markers (array of {x, y, icon, label}),
#               map_zoom, map_offset

set MAP_SIZE 500

ui.show {ui.window "World Map" [
    {ui.child "##map_canvas" =size [MAP_SIZE MAP_SIZE] =border true [
        # Base map image (scrollable/zoomable via C++ transforms)
        {ui.image map_texture [MAP_SIZE * map_zoom, MAP_SIZE * map_zoom]}

        # Overlay markers via draw list
        {ui.canvas fn [draw] do                                  # NEW
            for marker in markers do
                let sx (marker.x * map_zoom + map_offset.x)
                let sy (marker.y * map_zoom + map_offset.y)
                draw.circle [sx sy] 5 marker.color
                draw.text [sx + 8, sy - 6] marker.label
            end
        end}
    ]}

    # Controls
    {ui.slider "Zoom" 0.5 4.0 map_zoom fn [v] do
        set map_zoom v
    end}
    {ui.text "Click map to set waypoint"}
] =size [MAP_SIZE + 20, MAP_SIZE + 80]}
```

**New widgets:** canvas (DrawList wrapper), image, child
**C++ support:** Map texture, coordinate transforms, marker data

---

### 15. Party / Social Panel

Player list with health bars, roles, and action buttons.

```finescript
# C++ provides: party.members (array), each has: name, health, max_health,
#               class_name, role, is_leader, is_online

fn render_party_member [member] do
    {ui.group [
        # Online indicator + Name
        {ui.text_colored
            {if member.is_online [0.3 1.0 0.3 1.0] [0.5 0.5 0.5 1.0]}
            member.name}
        {ui.same_line}
        {ui.text_disabled "({member.class_name})"}

        # Health bar
        {ui.progress_bar (member.health / member.max_health)
            =size [200 12]
            =overlay "{member.health}/{member.max_health}"}

        # Action buttons (leader only)
        if is_leader do
            {ui.small_button "Kick" fn [] do                    # NEW
                game.kick_member member.id
            end}
            {ui.same_line}
            {ui.small_button "Promote" fn [] do
                game.promote_member member.id
            end}
        end
    ]}
    {ui.separator}
end

ui.show {ui.window "Party ({party.members.length}/4)" [
    for member in party.members do
        {render_party_member member}
    end
    {ui.spacing}
    {ui.button "Leave Party" fn [] do game.leave_party end}
    if is_leader do
        {ui.same_line}
        {ui.button "Disband" fn [] do game.disband_party end}
    end
] =size [280 0]}
```

**New widgets:** small_button, text_disabled, progress_bar with :size

---

## World Building Tools

### 16. Texture Painter

Color picker, brush settings, and texture preview canvas.

```finescript
# C++ provides: texture_canvas (texture handle), brush state, palette

set brush_color [1.0 0.0 0.0 1.0]
set brush_size 5
set brush_hardness 0.8

ui.show {ui.window "Texture Painter" [
    {ui.menu_bar [                                               # NEW
        {ui.menu "File" [                                        # NEW
            {ui.menu_item "New" fn [] do game.new_texture end}
            {ui.menu_item "Open..." fn [] do game.open_texture end}
            {ui.menu_item "Save" fn [] do game.save_texture end}
            {ui.separator}
            {ui.menu_item "Export PNG..." fn [] do game.export_png end}
        ]}
        {ui.menu "Edit" [
            {ui.menu_item "Undo" fn [] do game.undo end}
            {ui.menu_item "Redo" fn [] do game.redo end}
        ]}
    ]}

    # Canvas
    {ui.child "##canvas" =size [512 512] =border true [
        {ui.image texture_canvas [512 512]}
        {ui.canvas fn [draw] do
            # Draw brush cursor at mouse position
            if (ui.is_window_hovered) do
                let mp {ui.mouse_pos}
                draw.circle mp (brush_size * 2) [1.0 1.0 1.0 0.5] 1
            end
        end}
    ]}

    # Brush settings
    {ui.text "Brush"}
    {ui.color_edit "Color" brush_color fn [v] do                 # NEW
        set brush_color v
    end}
    {ui.slider_int "Size" 1 50 brush_size fn [v] do
        set brush_size v
    end}
    {ui.slider "Hardness" 0.0 1.0 brush_hardness fn [v] do
        set brush_hardness v
    end}

    # Palette
    {ui.separator_text "Palette"}                                # NEW
    for i in 0..palette.length do
        let color palette[i]
        {ui.color_button "pal_{i}" color fn [] do                # NEW
            set brush_color color
        end}
        if ((i + 1) % 8 != 0) do
            {ui.same_line}
        end
    end
] =size [550 700]}
```

**New widgets:** menu_bar, menu, menu_item, color_edit, color_button,
separator_text, canvas
**C++ support:** Texture painting operations, undo/redo, file I/O

---

### 17. Block Shape Editor

Vertex editing with face list and UV preview.

```finescript
# C++ provides: block model (vertices, faces, uvs), 3D preview texture

ui.show {ui.window "Block Shape Editor" [
    {ui.tab_bar "editor_tabs" [
        {ui.tab "Geometry" [
            # Vertex table
            {ui.table "vertices" 4 =flags [:row_bg :borders :resizable] [
                {ui.table_headers ["#" "X" "Y" "Z"]}
                for i in 0..model.vertices.length do
                    let v model.vertices[i]
                    {ui.table_next_row}
                    {ui.table_next_column}
                    {ui.text "{i}"}
                    {ui.table_next_column}
                    {ui.drag_float "##vx_{i}" v[0] 0.01 -1.0 1.0  # NEW
                        fn [val] do set v[0] val end}
                    {ui.table_next_column}
                    {ui.drag_float "##vy_{i}" v[1] 0.01 -1.0 1.0
                        fn [val] do set v[1] val end}
                    {ui.table_next_column}
                    {ui.drag_float "##vz_{i}" v[2] 0.01 -1.0 1.0
                        fn [val] do set v[2] val end}
                end
            ]}
            {ui.button "Add Vertex" fn [] do model.add_vertex end}
            {ui.same_line}
            {ui.button "Delete Selected" fn [] do
                model.delete_vertex selected_vertex
            end}
        ]}

        {ui.tab "Faces" [
            for i in 0..model.faces.length do
                let face model.faces[i]
                {ui.collapsing_header "Face {i}" [
                    {ui.text "Vertices: {face.indices}"}
                    {ui.combo "Texture" texture_names face.texture_idx
                        fn [v] do set face.texture_idx v end}
                ]}
            end
        ]}

        {ui.tab "UV Editor" [
            {ui.child "##uv_canvas" =size [400 400] =border true [
                {ui.image uv_preview_texture [400 400]}
                {ui.canvas fn [draw] do
                    # Draw UV wireframe
                    for face in model.faces do
                        for i in 0..face.uvs.length do
                            let uv1 face.uvs[i]
                            let uv2 face.uvs[(i + 1) % face.uvs.length]
                            draw.line
                                [(uv1[0] * 400) (uv1[1] * 400)]
                                [(uv2[0] * 400) (uv2[1] * 400)]
                                [1.0 1.0 0.0 1.0] 1
                        end
                    end
                end}
            ]}
        ]}
    ]}
] =size [500 600]}
```

**New widgets:** drag_float, table with :resizable, canvas
**C++ support:** 3D model data, UV preview texture

---

### 18. Terrain Brush Editor

Brush tools for terrain painting with biome and height controls.

```finescript
# C++ provides: terrain state, brush modes, biome list

set brush_mode :raise
set brush_radius 5
set brush_strength 0.5
set brush_falloff 0.7
set selected_biome 0

ui.show {ui.window "Terrain Tools" [
    {ui.text "Brush Mode"}
    {ui.radio_button "Raise" (brush_mode == :raise)              # NEW
        fn [] do set brush_mode :raise end}
    {ui.same_line}
    {ui.radio_button "Lower" (brush_mode == :lower)
        fn [] do set brush_mode :lower end}
    {ui.same_line}
    {ui.radio_button "Flatten" (brush_mode == :flatten)
        fn [] do set brush_mode :flatten end}
    {ui.same_line}
    {ui.radio_button "Smooth" (brush_mode == :smooth)
        fn [] do set brush_mode :smooth end}
    {ui.radio_button "Paint" (brush_mode == :paint)
        fn [] do set brush_mode :paint end}

    {ui.separator}

    {ui.text "Brush Settings"}
    {ui.slider_int "Radius" 1 50 brush_radius
        fn [v] do set brush_radius v end}
    {ui.slider "Strength" 0.0 1.0 brush_strength
        fn [v] do set brush_strength v end}
    {ui.slider "Falloff" 0.0 1.0 brush_falloff
        fn [v] do set brush_falloff v end}

    if (brush_mode == :paint) do
        {ui.separator}
        {ui.text "Biome"}
        {ui.combo "##biome" biome_names selected_biome
            fn [v] do set selected_biome v end}
    end

    {ui.separator}
    {ui.text "Terrain Info"}
    {ui.text "Height at cursor: {terrain.cursor_height:.2f}"}
    {ui.text "Biome: {terrain.cursor_biome}"}
] =size [300 0]}
```

**New widgets:** radio_button
**C++ support:** Terrain system, brush preview

---

### 19. Entity / Component Editor

Dynamic property inspector for ECS entities.

```finescript
# C++ provides: selected_entity, component_types, add_component, remove_component

fn render_property [prop ctx] do
    match prop.type
        :int do
            {ui.drag_int prop.name prop.value prop.speed prop.min prop.max
                fn [v] do ctx.set_property prop.id v end}
        end
        :float do
            {ui.drag_float prop.name prop.value prop.speed prop.min prop.max
                fn [v] do ctx.set_property prop.id v end}
        end
        :vec3 do
            {ui.drag_float3 prop.name prop.value 0.1                 # NEW
                fn [v] do ctx.set_property prop.id v end}
        end
        :color do
            {ui.color_edit prop.name prop.value
                fn [v] do ctx.set_property prop.id v end}
        end
        :bool do
            {ui.checkbox prop.name prop.value
                fn [v] do ctx.set_property prop.id v end}
        end
        :string do
            {ui.input prop.name prop.value
                fn [v] do ctx.set_property prop.id v end}
        end
        :enum do
            {ui.combo prop.name prop.options prop.selected
                fn [v] do ctx.set_property prop.id v end}
        end
        :asset do
            {ui.input prop.name prop.value}
            {ui.same_line}
            {ui.small_button "..." fn [] do
                open_asset_picker prop.id prop.asset_type
            end}
            {ui.drop_target "asset" fn [payload] do
                ctx.set_property prop.id payload
            end}
        end
        _ do
            {ui.text "{prop.name}: {prop.value} (unknown type)"}
        end
    end
end

fn render_component [comp entity] do
    {ui.collapsing_header comp.name [
        for prop in comp.properties do
            {render_property prop entity}
        end
        {ui.spacing}
        {ui.small_button "Remove Component" fn [] do
            entity.remove_component comp.type
        end}
    ] =default_open true}
end

ui.show {ui.window "Inspector" [
    if (selected_entity != nil) do
        # Entity header
        {ui.input "Name" selected_entity.name
            fn [v] do set selected_entity.name v end}
        {ui.text_disabled "ID: {selected_entity.id}"}
        {ui.separator}

        # Components
        for comp in selected_entity.components do
            {render_component comp selected_entity}
        end

        # Add component
        {ui.spacing}
        {ui.button "Add Component..." fn [] do
            ui.open_popup "add_comp"                             # NEW
        end}
        {ui.popup "add_comp" [                                   # NEW
            for t in available_component_types do
                {ui.selectable t.name false fn [] do
                    selected_entity.add_component t
                    ui.close_popup                               # NEW
                end}
            end
        ]}
    else
        {ui.text_disabled "No entity selected"}
    end
] =size [350 0]}
```

**New widgets:** drag_float, drag_float3, drag_int, color_edit, small_button,
selectable, popup, open_popup, close_popup, drop_target
**Key pattern:** Type-dispatch rendering via match

---

### 20. Particle Effect Editor

Parameter editor with preview canvas and curve editing.

```finescript
# C++ provides: particle_system state, preview texture, curve data

ui.show {ui.window "Particle Editor" [
    # Preview
    {ui.child "##preview" =size [0 300] =border true [
        {ui.image particle_preview [400 300]}
    ]}

    {ui.button "Play" fn [] do particle.play end}
    {ui.same_line}
    {ui.button "Stop" fn [] do particle.stop end}
    {ui.same_line}
    {ui.button "Restart" fn [] do particle.restart end}

    {ui.separator}

    {ui.tab_bar "ptabs" [
        {ui.tab "Emission" [
            {ui.drag_float "Rate" particle.rate 1.0 0.0 10000.0}
            {ui.drag_float "Duration" particle.duration 0.1 0.0 60.0}
            {ui.checkbox "Looping" particle.looping}
            {ui.drag_int "Max Particles" particle.max_count 1 0 100000}
        ]}

        {ui.tab "Shape" [
            {ui.combo "Emitter" ["Point" "Sphere" "Box" "Cone" "Ring"]
                particle.emitter_type}
            {ui.drag_float3 "Position" particle.position 0.1}
            {ui.drag_float3 "Size" particle.size 0.1}
            {ui.slider_angle "Cone Angle" particle.cone_angle}   # NEW
        ]}

        {ui.tab "Velocity" [
            {ui.drag_float3 "Initial" particle.velocity 0.1}
            {ui.drag_float3 "Gravity" particle.gravity 0.01}
            {ui.drag_float "Dampening" particle.dampening 0.01 0.0 1.0}
        ]}

        {ui.tab "Appearance" [
            {ui.color_edit "Start Color" particle.start_color}
            {ui.color_edit "End Color" particle.end_color}
            {ui.drag_float "Start Size" particle.start_size 0.01 0.0 10.0}
            {ui.drag_float "End Size" particle.end_size 0.01 0.0 10.0}
            {ui.drag_float "Start Alpha" particle.start_alpha 0.01 0.0 1.0}
            {ui.drag_float "End Alpha" particle.end_alpha 0.01 0.0 1.0}
        ]}

        {ui.tab "Curves" [
            # Size over lifetime curve
            {ui.text "Size over Lifetime"}
            {ui.canvas fn [draw] do                              # Curve editor
                let w 400
                let h 100
                # Draw grid
                draw.rect [0 0] [w h] [0.2 0.2 0.2 1.0]
                # Draw curve points
                for i in 0..(particle.size_curve.length - 1) do
                    let p1 particle.size_curve[i]
                    let p2 particle.size_curve[i + 1]
                    draw.line
                        [(p1.t * w) (h - p1.v * h)]
                        [(p2.t * w) (h - p2.v * h)]
                        [1.0 1.0 0.0 1.0] 2
                end
            end =size [400 100]}
        ]}
    ]}
] =size [450 700]}
```

**New widgets:** drag_float3, color_edit, slider_angle, canvas with :size
**C++ support:** Particle system, curve data, preview rendering

---

## Game Engine / Designer

### 21. Asset Browser

Split panel: directory tree + thumbnail grid with search.

```finescript
# C++ provides: file_tree (recursive), assets with thumbnails, search

set search_text ""
set selected_dir root_dir

fn render_dir_tree [dir] do
    let flags {if (dir == selected_dir) [:selected :open_on_arrow]
                                        [:open_on_arrow]}
    {ui.tree_node dir.name flags fn [] do                        # NEW
        set selected_dir dir
    end [
        for child in dir.children do
            if child.is_dir do
                {render_dir_tree child}
            end
        end
    ]}
end

fn render_asset_thumbnail [asset] do
    {ui.group [
        {ui.image asset.thumbnail [64 64]}
        {ui.text asset.name}
        {ui.drag_source "asset" asset.path [
            {ui.image asset.thumbnail [32 32]}
            {ui.same_line}
            {ui.text asset.name}
        ]}
        {ui.context_menu [
            {ui.menu_item "Open" fn [] do game.open_asset asset end}
            {ui.menu_item "Rename..." fn [] do open_rename asset end}
            {ui.menu_item "Delete" fn [] do game.delete_asset asset end}
        ]}
    ]}
end

ui.show {ui.window "Assets" [
    # Search bar
    {ui.input "##search" search_text =hint "Search assets..."
        fn [v] do set search_text v end}
    {ui.separator}

    # Left panel: directory tree
    {ui.child "##dirs" =size [200 0] =border true [
        {render_dir_tree root_dir}
    ]}
    {ui.same_line}

    # Right panel: thumbnail grid
    {ui.child "##files" =size [0 0] [
        let assets {get_dir_assets selected_dir}
        let filtered {if (search_text.length > 0)
            {assets.filter fn [a] {a.name.contains search_text}}
            assets}

        {ui.table "##grid" 4 =flags [:no_borders] [
            for asset in filtered do
                {ui.table_next_column}
                {render_asset_thumbnail asset}
            end
        ]}
    ]}
] =size [700 500]}
```

**New widgets:** tree_node (with flags, on_select, children), child with :border
**C++ support:** File system, thumbnails, asset operations

---

### 22. Script Editor

Multiline text editor with line numbers (basic — syntax highlighting is future).

```finescript
# C++ provides: file operations, script runner

set current_file nil
set source_text ""
set output_text ""

ui.show {ui.window "Script Editor" [
    {ui.menu_bar [
        {ui.menu "File" [
            {ui.menu_item "New" fn [] do
                set source_text ""
                set current_file nil
            end}
            {ui.menu_item "Open..." fn [] do
                set current_file {game.file_dialog "Open Script" "*.script"}
                if (current_file != nil) do
                    set source_text {game.read_file current_file}
                end
            end}
            {ui.menu_item "Save" fn [] do
                if (current_file != nil) do
                    game.write_file current_file source_text
                end
            end}
        ]}
        {ui.menu "Run" [
            {ui.menu_item "Execute" fn [] do
                set output_text {game.run_script source_text}
            end}
        ]}
    ]}

    # Editor area
    {ui.text {if (current_file != nil) current_file "Untitled"}}
    {ui.input_multiline "##editor" source_text                   # NEW
        =size [-1 -200]
        fn [v] do set source_text v end}

    # Output panel
    {ui.separator_text "Output"}
    {ui.child "##output" =size [0 0] [
        {ui.text output_text}
    ]}
] =size [800 600]}
```

**New widgets:** input_multiline, menu_bar, menu, menu_item, separator_text
**C++ support:** File dialog, file I/O, script execution

---

### 23. Property Inspector

(See [Entity / Component Editor](#19-entity--component-editor) — same pattern,
reusable for any inspectable object.)

---

### 24. Scene Hierarchy

Tree view of scene entities with drag-drop reparenting and context menus.

```finescript
# C++ provides: scene_root (recursive tree of entities)

fn render_entity_node [entity] do
    let has_children (entity.children.length > 0)
    let flags []
    if (entity == selected_entity) do flags.push :selected end
    if (not has_children) do flags.push :leaf end

    {ui.tree_node entity.name flags fn [] do
        set selected_entity entity
    end [
        for child in entity.children do
            {render_entity_node child}
        end
    ]}

    # Drag to reparent
    {ui.drag_source "entity" entity.id [
        {ui.text entity.name}
    ]}
    {ui.drop_target "entity" fn [payload] do
        scene.reparent payload entity.id
    end}

    # Context menu
    {ui.context_menu [
        {ui.menu_item "Add Child" fn [] do scene.add_child entity end}
        {ui.menu_item "Duplicate" fn [] do scene.duplicate entity end}
        {ui.separator}
        {ui.menu_item "Delete" fn [] do scene.delete entity end}
    ]}
end

ui.show {ui.window "Scene Hierarchy" [
    {ui.input "##filter" filter_text =hint "Filter..."
        fn [v] do set filter_text v end}
    {ui.separator}

    {ui.child "##tree" =size [0 -30] [
        {render_entity_node scene_root}
    ]}

    {ui.button "Add Entity" fn [] do scene.add_entity end}
    {ui.same_line}
    {ui.button "Add Group" fn [] do scene.add_group end}
] =size [300 500]}
```

**New widgets:** tree_node (with flags, drag-drop), child, context_menu
**C++ support:** Scene graph operations

---

### 25. Timeline / Animation Editor

Horizontal scrollable canvas with keyframe markers and playback controls.

```finescript
# C++ provides: animation state (tracks, keyframes, current_time, duration)

set current_time 0.0
set playing false

ui.show {ui.window "Timeline" [
    # Playback controls
    {ui.button {if playing "||" ">"} fn [] do
        set playing (not playing)
    end}
    {ui.same_line}
    {ui.button "|<" fn [] do set current_time 0.0 end}
    {ui.same_line}
    {ui.drag_float "Time" current_time 0.01 0.0 animation.duration
        fn [v] do set current_time v end}
    {ui.same_line}
    {ui.text "/ {animation.duration:.2f}s"}

    {ui.separator}

    # Track list + timeline canvas
    {ui.child "##timeline" =size [0 0] [
        # Left: track names (fixed width)
        for track in animation.tracks do
            {ui.text track.name}
            {ui.spacing}
        end
    ]}
    # TODO: proper split layout
    # Right side would be a canvas showing keyframes on a time axis
    # This requires horizontal scrolling child + canvas drawing
    # Full implementation would use DrawList extensively
] =size [800 300]}
```

**New widgets:** drag_float, canvas for keyframes
**C++ support:** Animation system
**Note:** Full timeline editor would need significant custom drawing. Shown here
as a starting point — real implementation would lean heavily on canvas/DrawList.

---

### 26. Console / Log Window

Filterable log with colored output and command input.

```finescript
# C++ provides: log_entries (array of {level, text, timestamp}),
#               execute_command(text)

set command_text ""
set filter_text ""
set show_info true
set show_warn true
set show_error true

fn level_color [level] do
    match level
        :info [0.8 0.8 0.8 1.0]
        :warn [1.0 0.9 0.3 1.0]
        :error [1.0 0.3 0.3 1.0]
        :debug [0.5 0.5 0.5 1.0]
        _ [1.0 1.0 1.0 1.0]
    end
end

fn should_show [entry] do
    if (filter_text.length > 0 and not {entry.text.contains filter_text}) do
        return false
    end
    match entry.level
        :info show_info
        :warn show_warn
        :error show_error
        _ true
    end
end

ui.show {ui.window "Console" [
    # Filter controls
    {ui.checkbox "Info" show_info fn [v] do set show_info v end}
    {ui.same_line}
    {ui.checkbox "Warn" show_warn fn [v] do set show_warn v end}
    {ui.same_line}
    {ui.checkbox "Error" show_error fn [v] do set show_error v end}
    {ui.same_line}
    {ui.input "##filter" filter_text =hint "Filter..."
        fn [v] do set filter_text v end}
    {ui.same_line}
    {ui.button "Clear" fn [] do log_entries.clear end}

    {ui.separator}

    # Log output (scrollable)
    {ui.child "##log" =size [0 -30] =auto_scroll true [
        for entry in log_entries do
            if {should_show entry} do
                {ui.text_colored {level_color entry.level}
                    "[{entry.timestamp}] [{entry.level}] {entry.text}"}
            end
        end
    ]}

    # Command input
    {ui.input "##cmd" command_text =hint "Enter command..."
        =on_submit fn [v] do
            game.execute_command v
            set command_text ""
        end}
] =size [700 400]}
```

**New widgets:** child with :auto_scroll, text_colored, checkbox, input with :hint
**Language used:** match, for-in, closures, string interpolation, filter logic

---

### 27. Node Graph Editor

Canvas-based editor for visual scripting / shader graphs. Heavily DrawList-based.

```finescript
# C++ provides: graph state (nodes, connections), layout positions
# This is the most complex example — mostly custom drawing

set selected_node nil

fn render_node_graph [] do
    {ui.window "Node Editor" [
        {ui.canvas fn [draw] do
            let canvas_pos {ui.get_cursor_screen_pos}

            # Draw connections
            for conn in graph.connections do
                let from_pos {get_pin_pos conn.from_node conn.from_pin}
                let to_pos {get_pin_pos conn.to_node conn.to_pin}
                draw.bezier from_pos to_pos [0.7 0.7 0.3 1.0] 2
            end

            # Draw nodes
            for node in graph.nodes do
                let pos [(node.pos[0] + canvas_pos[0])
                         (node.pos[1] + canvas_pos[1])]
                let size node.size

                # Node background
                let bg {if (node == selected_node) [0.3 0.3 0.5 1.0]
                                                    [0.2 0.2 0.2 1.0]}
                draw.rect_filled pos [(pos[0] + size[0]) (pos[1] + size[1])]
                    bg 4

                # Title bar
                draw.rect_filled pos [(pos[0] + size[0]) (pos[1] + 25)]
                    node.color 4
                draw.text [(pos[0] + 5) (pos[1] + 5)] node.name

                # Input pins
                for i in 0..node.inputs.length do
                    let pin node.inputs[i]
                    let pin_y (pos[1] + 35 + i * 20)
                    draw.circle [(pos[0]) pin_y] 5 pin.color
                    draw.text [(pos[0] + 10) (pin_y - 8)] pin.name
                end

                # Output pins
                for i in 0..node.outputs.length do
                    let pin node.outputs[i]
                    let pin_y (pos[1] + 35 + i * 20)
                    draw.circle [(pos[0] + size[0]) pin_y] 5 pin.color
                    draw.text [(pos[0] + size[0] - 60) (pin_y - 8)] pin.name
                end
            end
        end =size [0 0]}  # Fill available space

        # Node palette (right-click to add)
        {ui.context_menu [
            {ui.menu "Add Node" [
                for cat in node_categories do
                    {ui.menu cat.name [
                        for type in cat.types do
                            {ui.menu_item type.name fn [] do
                                graph.add_node type {ui.mouse_pos}
                            end}
                        end
                    ]}
                end
            ]}
        ]}
    ] =size [800 600]}
end

render_node_graph
```

**New widgets:** canvas (DrawList), context_menu with nested menus
**C++ support:** Graph data model, layout, connection routing, bezier drawing
**Note:** A real node editor would need mouse hit-testing, drag handling,
connection creation via click-drag between pins. This is the most C++-heavy
example — likely a custom widget with a script-accessible API rather than
pure script drawing.

---

### 28. Menu Bar

Full application menu bar with menus, submenus, separators, and shortcuts.

```finescript
# This would be part of a larger editor window

{ui.main_menu_bar [                                              # NEW
    {ui.menu "File" [
        {ui.menu_item "New Project" =shortcut "Ctrl+N"
            fn [] do editor.new_project end}
        {ui.menu_item "Open Project..." =shortcut "Ctrl+O"
            fn [] do editor.open_project end}
        {ui.menu "Open Recent" [
            for path in recent_projects do
                {ui.menu_item path fn [] do editor.open path end}
            end
        ]}
        {ui.separator}
        {ui.menu_item "Save" =shortcut "Ctrl+S"
            fn [] do editor.save end}
        {ui.menu_item "Save As..." =shortcut "Ctrl+Shift+S"
            fn [] do editor.save_as end}
        {ui.separator}
        {ui.menu_item "Exit" fn [] do editor.exit end}
    ]}
    {ui.menu "Edit" [
        {ui.menu_item "Undo" =shortcut "Ctrl+Z"
            fn [] do editor.undo end}
        {ui.menu_item "Redo" =shortcut "Ctrl+Shift+Z"
            fn [] do editor.redo end}
        {ui.separator}
        {ui.menu_item "Cut" =shortcut "Ctrl+X"
            fn [] do editor.cut end}
        {ui.menu_item "Copy" =shortcut "Ctrl+C"
            fn [] do editor.copy end}
        {ui.menu_item "Paste" =shortcut "Ctrl+V"
            fn [] do editor.paste end}
        {ui.separator}
        {ui.menu_item "Preferences..." fn [] do
            editor.show_preferences
        end}
    ]}
    {ui.menu "View" [
        {ui.menu_item "Inspector" =checked show_inspector
            fn [] do set show_inspector (not show_inspector) end}
        {ui.menu_item "Console" =checked show_console
            fn [] do set show_console (not show_console) end}
        {ui.menu_item "Scene" =checked show_scene
            fn [] do set show_scene (not show_scene) end}
        {ui.menu_item "Assets" =checked show_assets
            fn [] do set show_assets (not show_assets) end}
    ]}
    {ui.menu "Tools" [
        {ui.menu_item "Texture Painter" fn [] do open_texture_painter end}
        {ui.menu_item "Block Editor" fn [] do open_block_editor end}
        {ui.menu_item "Terrain Editor" fn [] do open_terrain_editor end}
        {ui.menu_item "Particle Editor" fn [] do open_particle_editor end}
        {ui.separator}
        {ui.menu_item "Script Console" fn [] do open_console end}
    ]}
    {ui.menu "Help" [
        {ui.menu_item "Documentation" fn [] do game.open_docs end}
        {ui.menu_item "About" fn [] do show_about_dialog end}
    ]}
]}
```

**New widgets:** main_menu_bar, menu (nested), menu_item with =shortcut and =checked
**Key pattern:** Deeply nested menu structure from script

---

### 29. Toolbar

Horizontal row of icon buttons with tooltips and toggle states.

```finescript
# C++ provides: tool icons (textures), current_tool

set tools [
    {=id :select =icon select_icon =tip "Select (Q)"}
    {=id :move =icon move_icon =tip "Move (W)"}
    {=id :rotate =icon rotate_icon =tip "Rotate (E)"}
    {=id :scale =icon scale_icon =tip "Scale (R)"}
    nil  # separator
    {=id :paint =icon paint_icon =tip "Paint (B)"}
    {=id :sculpt =icon sculpt_icon =tip "Sculpt (S)"}
]

set toolbar_win {ui.window "##toolbar" [
    for tool in tools do
        if (tool == nil) do
            {ui.separator}                  # Vertical separator in horizontal layout
        else
            let tint {if (current_tool == tool.id)
                [1.0 1.0 1.0 1.0]
                [0.6 0.6 0.6 1.0]}
            {ui.image_button tool.id tool.icon [24 24] =tint tint
                fn [] do set current_tool tool.id end}
            {ui.item_tooltip [{ui.text tool.tip}]}
            {ui.same_line}
        end
    end
]}
set toolbar_win.flags [:no_resize :no_move :no_title_bar :no_scrollbar
                       :always_auto_resize]
set toolbar_win.pos [10 30]
ui.show toolbar_win
```

**New widgets:** image_button with :tint, item_tooltip
**C++ support:** Tool icons, tool switching

---

### 30. Status Bar

Bottom-of-screen information bar.

```finescript
# C++ provides: status info

set status_win {ui.window "##statusbar" [
    {ui.text editor.status_text}
    {ui.same_line =offset (screen_width - 400)}
    {ui.text "Entities: {scene.entity_count}"}
    {ui.same_line}
    {ui.text "|"}
    {ui.same_line}
    {ui.text "{editor.fps:.0f} FPS"}
    {ui.same_line}
    {ui.text "|"}
    {ui.same_line}
    {ui.text editor.mode}
]}
set status_win.pos [0 screen_height - 25]
set status_win.size [screen_width 25]
set status_win.flags [:no_resize :no_move :no_title_bar :no_scrollbar
                      :no_collapse]
ui.show status_win
```

**New widgets:** same_line with :offset
**C++ support:** Status data

---

## Summary: Complete Widget Primitive List

### Layout (Phase 3)
- `ui.same_line` — horizontal layout (:offset for alignment)
- `ui.spacing` — vertical space
- `ui.indent` / `ui.unindent` — indentation
- `ui.dummy` — invisible spacer with specific size
- `ui.new_line` — force new line

### Display (Phase 3)
- `ui.progress_bar` — with =overlay text, =size
- `ui.text_colored` — colored text (takes [r g b a] + text)
- `ui.text_wrapped` — auto-wrapping text
- `ui.text_disabled` — grayed out text
- `ui.bullet_text` — bulleted text
- `ui.separator_text` — separator with centered text label
- `ui.plot_lines` — line graph from array (=height)
- `ui.plot_histogram` — bar chart from array

### Containers (Phase 4)
- `ui.collapsing_header` — expandable section (=default_open)
- `ui.tree_node` — tree hierarchy (=flags, on_select, children)
- `ui.tab_bar` / `ui.tab` — tabbed sections
- `ui.child` — scroll region (=size, =border, =auto_scroll)

### Tables (Phase 5)
- `ui.table` — table container (id, columns, =flags)
- `ui.table_headers` — header row from array
- `ui.table_next_row` — advance to next row
- `ui.table_next_column` — advance to next column

### Menus & Popups (Phase 6)
- `ui.menu_bar` / `ui.main_menu_bar` — menu bars
- `ui.menu` — dropdown menu
- `ui.menu_item` — menu action (=shortcut, =checked)
- `ui.popup` — popup window (by id)
- `ui.open_popup` / `ui.close_popup` — popup control
- `ui.context_menu` — right-click popup
- `ui.tooltip` / `ui.item_tooltip` — hover tooltips

### Advanced Input (Phase 7)
- `ui.drag_float` / `ui.drag_int` — drag-to-change numbers
- `ui.drag_float3` — 3-component vector input
- `ui.color_edit` — color editor with picker
- `ui.color_button` — clickable color swatch
- `ui.radio_button` — radio button
- `ui.input_multiline` — multi-line text editor (=size)
- `ui.input` with `=hint` — placeholder text
- `ui.slider_angle` — angle in degrees
- `ui.selectable` — selectable list item
- `ui.small_button` — compact button

### Interaction (Phase 8)
- `ui.drag_source` — start drag operation (type, payload, preview)
- `ui.drop_target` — accept drop (type, callback)
- `ui.image` — display texture (=size)
- `ui.image_button` — clickable image (=tint)

### Canvas (Phase 9)
- `ui.canvas` — custom drawing via DrawList callback (=size)
  - `draw.line`, `draw.rect`, `draw.rect_filled`, `draw.circle`,
    `draw.text`, `draw.bezier`, `draw.image`

### Window Control
- `.pos` — [x, y] position
- `.size` — [w, h] size
- `.flags` — array of flag symbols
- `.bg_alpha` — background alpha
- Window flags: `:no_resize`, `:no_move`, `:no_title_bar`, `:no_scrollbar`,
  `:no_collapse`, `:always_auto_resize`, `:no_background`, `:no_nav`,
  `:no_inputs`

---

## Build Priority

Based on how many examples use each widget:

| Widget | Used By Examples | Priority |
|--------|-----------------|----------|
| same_line | 1,2,4,5,6,8,10,11,15,17,29,30 | Critical |
| progress_bar | 1,2,4,10,15,20 | Critical |
| text_colored | 2,3,7,8,11,12,15,26 | Critical |
| collapsing_header | 1,7,8,17,19 | Critical |
| tab_bar / tab | 5,6,8,17,20 | Critical |
| child (scroll) | 3,6,11,14,21,22,24,26 | Critical |
| table | 3,5,8,17,21 | Critical |
| context_menu | 3,21,24,27 | High |
| menu_bar / menu | 16,22,27,28 | High |
| image / image_button | 2,3,4,10,14,21,29 | High |
| drag_source / drop_target | 3,4,10,19,21,24 | High |
| item_tooltip | 3,4,8,10,29 | High |
| text_wrapped | 3,7,9,11 | High |
| drag_float / drag_int | 17,19,20,25 | High |
| color_edit | 16,19,20 | Medium |
| selectable | 11,19,21 | Medium |
| popup / open_popup | 19 | Medium |
| tree_node | 21,24 | Medium |
| radio_button | 18 | Medium |
| input_multiline | 22 | Medium |
| small_button | 15,19 | Medium |
| canvas (DrawList) | 14,17,20,25,27 | Medium |
| text_disabled | 3,8,19 | Medium |
| separator_text | 16,22 | Medium |
| plot_lines | 1 | Low |
| slider_angle | 20 | Low |
| color_button | 16 | Low |
| main_menu_bar | 28 | Low |
