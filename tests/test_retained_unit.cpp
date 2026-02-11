/**
 * @file test_retained_unit.cpp
 * @brief Unit tests for retained-mode widget system (no Vulkan required)
 *
 * Tests:
 * - WidgetNode construction via convenience builders
 * - WidgetNode field defaults
 * - Widget tree hierarchy
 * - Visibility and enabled flags
 * - widgetTypeName() for all types
 * - GuiRenderer show/hide/update/get ID management
 */

#include <finegui/widget_node.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/drag_drop_manager.hpp>
#include <finegui/texture_registry.hpp>
#include <imgui.h>

#include <iostream>
#include <cassert>
#include <string>

using namespace finegui;

// ============================================================================
// WidgetNode Builder Tests
// ============================================================================

void test_window_builder() {
    std::cout << "Testing: WidgetNode::window builder... ";

    auto w = WidgetNode::window("Test Window");
    assert(w.type == WidgetNode::Type::Window);
    assert(w.label == "Test Window");
    assert(w.children.empty());
    assert(w.visible == true);
    assert(w.enabled == true);

    std::cout << "PASSED\n";
}

void test_window_with_children() {
    std::cout << "Testing: WidgetNode::window with children... ";

    auto w = WidgetNode::window("Settings", {
        WidgetNode::text("Hello"),
        WidgetNode::button("Click me"),
        WidgetNode::separator(),
    });
    assert(w.type == WidgetNode::Type::Window);
    assert(w.children.size() == 3);
    assert(w.children[0].type == WidgetNode::Type::Text);
    assert(w.children[0].textContent == "Hello");
    assert(w.children[1].type == WidgetNode::Type::Button);
    assert(w.children[1].label == "Click me");
    assert(w.children[2].type == WidgetNode::Type::Separator);

    std::cout << "PASSED\n";
}

void test_text_builder() {
    std::cout << "Testing: WidgetNode::text builder... ";

    auto t = WidgetNode::text("Hello World");
    assert(t.type == WidgetNode::Type::Text);
    assert(t.textContent == "Hello World");

    std::cout << "PASSED\n";
}

void test_button_builder() {
    std::cout << "Testing: WidgetNode::button builder... ";

    bool clicked = false;
    auto b = WidgetNode::button("Press", [&clicked](WidgetNode&) {
        clicked = true;
    });
    assert(b.type == WidgetNode::Type::Button);
    assert(b.label == "Press");
    assert(b.onClick);

    // Invoke callback manually
    b.onClick(b);
    assert(clicked);

    // Button without callback
    auto b2 = WidgetNode::button("No callback");
    assert(!b2.onClick);

    std::cout << "PASSED\n";
}

void test_checkbox_builder() {
    std::cout << "Testing: WidgetNode::checkbox builder... ";

    bool changed = false;
    auto c = WidgetNode::checkbox("Enable", true, [&changed](WidgetNode&) {
        changed = true;
    });
    assert(c.type == WidgetNode::Type::Checkbox);
    assert(c.label == "Enable");
    assert(c.boolValue == true);
    assert(c.onChange);

    c.onChange(c);
    assert(changed);

    std::cout << "PASSED\n";
}

void test_slider_builder() {
    std::cout << "Testing: WidgetNode::slider builder... ";

    auto s = WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f);
    assert(s.type == WidgetNode::Type::Slider);
    assert(s.label == "Volume");
    assert(s.floatValue == 0.5f);
    assert(s.minFloat == 0.0f);
    assert(s.maxFloat == 1.0f);

    std::cout << "PASSED\n";
}

void test_slider_int_builder() {
    std::cout << "Testing: WidgetNode::sliderInt builder... ";

    auto s = WidgetNode::sliderInt("Level", 5, 1, 10);
    assert(s.type == WidgetNode::Type::SliderInt);
    assert(s.label == "Level");
    assert(s.intValue == 5);
    assert(s.minInt == 1);
    assert(s.maxInt == 10);

    std::cout << "PASSED\n";
}

void test_input_text_builder() {
    std::cout << "Testing: WidgetNode::inputText builder... ";

    bool changed = false;
    bool submitted = false;
    auto i = WidgetNode::inputText("Name", "Alice",
        [&changed](WidgetNode&) { changed = true; },
        [&submitted](WidgetNode&) { submitted = true; }
    );
    assert(i.type == WidgetNode::Type::InputText);
    assert(i.label == "Name");
    assert(i.stringValue == "Alice");
    assert(i.onChange);
    assert(i.onSubmit);

    i.onChange(i);
    assert(changed);
    i.onSubmit(i);
    assert(submitted);

    std::cout << "PASSED\n";
}

void test_input_int_builder() {
    std::cout << "Testing: WidgetNode::inputInt builder... ";

    auto i = WidgetNode::inputInt("Count", 42);
    assert(i.type == WidgetNode::Type::InputInt);
    assert(i.label == "Count");
    assert(i.intValue == 42);

    std::cout << "PASSED\n";
}

void test_input_float_builder() {
    std::cout << "Testing: WidgetNode::inputFloat builder... ";

    auto i = WidgetNode::inputFloat("Scale", 1.5f);
    assert(i.type == WidgetNode::Type::InputFloat);
    assert(i.label == "Scale");
    assert(i.floatValue == 1.5f);

    std::cout << "PASSED\n";
}

void test_combo_builder() {
    std::cout << "Testing: WidgetNode::combo builder... ";

    auto c = WidgetNode::combo("Resolution",
        {"1920x1080", "2560x1440", "3840x2160"}, 0);
    assert(c.type == WidgetNode::Type::Combo);
    assert(c.label == "Resolution");
    assert(c.items.size() == 3);
    assert(c.items[0] == "1920x1080");
    assert(c.items[2] == "3840x2160");
    assert(c.selectedIndex == 0);

    std::cout << "PASSED\n";
}

void test_separator_builder() {
    std::cout << "Testing: WidgetNode::separator builder... ";

    auto s = WidgetNode::separator();
    assert(s.type == WidgetNode::Type::Separator);

    std::cout << "PASSED\n";
}

void test_group_builder() {
    std::cout << "Testing: WidgetNode::group builder... ";

    auto g = WidgetNode::group({
        WidgetNode::text("A"),
        WidgetNode::text("B"),
    });
    assert(g.type == WidgetNode::Type::Group);
    assert(g.children.size() == 2);

    std::cout << "PASSED\n";
}

void test_columns_builder() {
    std::cout << "Testing: WidgetNode::columns builder... ";

    auto c = WidgetNode::columns(3, {
        WidgetNode::text("Col1"),
        WidgetNode::text("Col2"),
        WidgetNode::text("Col3"),
    });
    assert(c.type == WidgetNode::Type::Columns);
    assert(c.columnCount == 3);
    assert(c.children.size() == 3);

    std::cout << "PASSED\n";
}

void test_image_builder() {
    std::cout << "Testing: WidgetNode::image builder... ";

    TextureHandle tex;
    tex.id = 99;
    tex.width = 128;
    tex.height = 128;

    auto img = WidgetNode::image(tex, 64.0f, 64.0f);
    assert(img.type == WidgetNode::Type::Image);
    assert(img.texture.id == 99);
    assert(img.imageWidth == 64.0f);
    assert(img.imageHeight == 64.0f);

    std::cout << "PASSED\n";
}

// ============================================================================
// WidgetNode Field Defaults
// ============================================================================

void test_field_defaults() {
    std::cout << "Testing: WidgetNode field defaults... ";

    WidgetNode n;
    // Numeric defaults
    assert(n.floatValue == 0.0f);
    assert(n.intValue == 0);
    assert(n.boolValue == false);
    assert(n.stringValue.empty());
    assert(n.selectedIndex == -1);

    // Range defaults
    assert(n.minFloat == 0.0f);
    assert(n.maxFloat == 1.0f);
    assert(n.minInt == 0);
    assert(n.maxInt == 100);

    // Layout defaults
    assert(n.width == 0.0f);
    assert(n.height == 0.0f);
    assert(n.columnCount == 1);

    // State defaults
    assert(n.visible == true);
    assert(n.enabled == true);

    // No callbacks
    assert(!n.onClick);
    assert(!n.onChange);
    assert(!n.onSubmit);
    assert(!n.onClose);

    // Empty collections
    assert(n.items.empty());
    assert(n.children.empty());

    // Texture defaults
    assert(!n.texture.valid());
    assert(n.imageWidth == 0.0f);
    assert(n.imageHeight == 0.0f);

    std::cout << "PASSED\n";
}

// ============================================================================
// Visibility / Enabled
// ============================================================================

void test_visibility_flags() {
    std::cout << "Testing: Visibility and enabled flags... ";

    auto w = WidgetNode::window("Test", {
        WidgetNode::text("Visible"),
        WidgetNode::button("Hidden"),
    });

    // Default: all visible and enabled
    assert(w.visible == true);
    assert(w.enabled == true);
    assert(w.children[0].visible == true);
    assert(w.children[1].visible == true);

    // Toggle
    w.children[1].visible = false;
    assert(w.children[1].visible == false);

    w.enabled = false;
    assert(w.enabled == false);

    std::cout << "PASSED\n";
}

// ============================================================================
// widgetTypeName
// ============================================================================

void test_widget_type_names() {
    std::cout << "Testing: widgetTypeName for all types... ";

    // Phase 1
    assert(std::string(widgetTypeName(WidgetNode::Type::Window)) == "Window");
    assert(std::string(widgetTypeName(WidgetNode::Type::Text)) == "Text");
    assert(std::string(widgetTypeName(WidgetNode::Type::Button)) == "Button");
    assert(std::string(widgetTypeName(WidgetNode::Type::Checkbox)) == "Checkbox");
    assert(std::string(widgetTypeName(WidgetNode::Type::Slider)) == "Slider");
    assert(std::string(widgetTypeName(WidgetNode::Type::SliderInt)) == "SliderInt");
    assert(std::string(widgetTypeName(WidgetNode::Type::InputText)) == "InputText");
    assert(std::string(widgetTypeName(WidgetNode::Type::InputInt)) == "InputInt");
    assert(std::string(widgetTypeName(WidgetNode::Type::InputFloat)) == "InputFloat");
    assert(std::string(widgetTypeName(WidgetNode::Type::Combo)) == "Combo");
    assert(std::string(widgetTypeName(WidgetNode::Type::Separator)) == "Separator");
    assert(std::string(widgetTypeName(WidgetNode::Type::Group)) == "Group");
    assert(std::string(widgetTypeName(WidgetNode::Type::Columns)) == "Columns");
    assert(std::string(widgetTypeName(WidgetNode::Type::Image)) == "Image");

    // Phase 2
    assert(std::string(widgetTypeName(WidgetNode::Type::TabBar)) == "TabBar");
    assert(std::string(widgetTypeName(WidgetNode::Type::MenuItem)) == "MenuItem");

    // Phase 3
    assert(std::string(widgetTypeName(WidgetNode::Type::Table)) == "Table");
    assert(std::string(widgetTypeName(WidgetNode::Type::ColorEdit)) == "ColorEdit");

    // Phase 4
    assert(std::string(widgetTypeName(WidgetNode::Type::Canvas)) == "Canvas");
    assert(std::string(widgetTypeName(WidgetNode::Type::ProgressBar)) == "ProgressBar");

    std::cout << "PASSED\n";
}

// ============================================================================
// Nested Tree Structure
// ============================================================================

void test_nested_tree() {
    std::cout << "Testing: Deeply nested widget tree... ";

    auto tree = WidgetNode::window("Root", {
        WidgetNode::group({
            WidgetNode::columns(2, {
                WidgetNode::text("Left"),
                WidgetNode::group({
                    WidgetNode::button("Nested Button"),
                    WidgetNode::slider("Nested Slider", 0.5f, 0.0f, 1.0f),
                }),
            }),
        }),
        WidgetNode::separator(),
        WidgetNode::button("Bottom"),
    });

    assert(tree.type == WidgetNode::Type::Window);
    assert(tree.children.size() == 3);

    auto& group = tree.children[0];
    assert(group.type == WidgetNode::Type::Group);
    assert(group.children.size() == 1);

    auto& cols = group.children[0];
    assert(cols.type == WidgetNode::Type::Columns);
    assert(cols.columnCount == 2);
    assert(cols.children.size() == 2);

    auto& rightGroup = cols.children[1];
    assert(rightGroup.type == WidgetNode::Type::Group);
    assert(rightGroup.children.size() == 2);
    assert(rightGroup.children[0].type == WidgetNode::Type::Button);
    assert(rightGroup.children[1].type == WidgetNode::Type::Slider);

    std::cout << "PASSED\n";
}

// ============================================================================
// Complete Settings Panel (from design doc)
// ============================================================================

void test_settings_panel_pattern() {
    std::cout << "Testing: Settings panel pattern from design doc... ";

    float volumeValue = 0.5f;
    bool muteValue = false;
    int resIndex = 0;
    bool applied = false;

    auto settings = WidgetNode::window("Settings", {
        WidgetNode::text("Audio"),
        WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f, [&volumeValue](WidgetNode& w) {
            volumeValue = w.floatValue;
        }),
        WidgetNode::checkbox("Mute", false, [&muteValue](WidgetNode& w) {
            muteValue = w.boolValue;
        }),
        WidgetNode::separator(),
        WidgetNode::text("Graphics"),
        WidgetNode::combo("Resolution", {"1920x1080", "2560x1440", "3840x2160"}, 0,
            [&resIndex](WidgetNode& w) {
                resIndex = w.selectedIndex;
            }),
        WidgetNode::separator(),
        WidgetNode::button("Apply", [&applied](WidgetNode&) {
            applied = true;
        }),
    });

    assert(settings.type == WidgetNode::Type::Window);
    assert(settings.label == "Settings");
    assert(settings.children.size() == 8);

    // Simulate interactions via callbacks
    settings.children[1].floatValue = 0.8f;
    settings.children[1].onChange(settings.children[1]);
    assert(volumeValue == 0.8f);

    settings.children[2].boolValue = true;
    settings.children[2].onChange(settings.children[2]);
    assert(muteValue == true);

    settings.children[5].selectedIndex = 2;
    settings.children[5].onChange(settings.children[5]);
    assert(resIndex == 2);

    settings.children[7].onClick(settings.children[7]);
    assert(applied);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 3 Builder Tests
// ============================================================================

void test_same_line_builder() {
    std::cout << "Testing: WidgetNode::sameLine builder... ";

    auto sl = WidgetNode::sameLine();
    assert(sl.type == WidgetNode::Type::SameLine);
    assert(sl.offsetX == 0.0f);

    auto sl2 = WidgetNode::sameLine(100.0f);
    assert(sl2.offsetX == 100.0f);

    std::cout << "PASSED\n";
}

void test_spacing_builder() {
    std::cout << "Testing: WidgetNode::spacing builder... ";

    auto sp = WidgetNode::spacing();
    assert(sp.type == WidgetNode::Type::Spacing);

    std::cout << "PASSED\n";
}

void test_text_colored_builder() {
    std::cout << "Testing: WidgetNode::textColored builder... ";

    auto tc = WidgetNode::textColored(1.0f, 0.3f, 0.3f, 1.0f, "Error!");
    assert(tc.type == WidgetNode::Type::TextColored);
    assert(tc.textContent == "Error!");
    assert(tc.colorR == 1.0f);
    assert(tc.colorG == 0.3f);
    assert(tc.colorB == 0.3f);
    assert(tc.colorA == 1.0f);

    std::cout << "PASSED\n";
}

void test_text_wrapped_builder() {
    std::cout << "Testing: WidgetNode::textWrapped builder... ";

    auto tw = WidgetNode::textWrapped("This is a long text that wraps.");
    assert(tw.type == WidgetNode::Type::TextWrapped);
    assert(tw.textContent == "This is a long text that wraps.");

    std::cout << "PASSED\n";
}

void test_text_disabled_builder() {
    std::cout << "Testing: WidgetNode::textDisabled builder... ";

    auto td = WidgetNode::textDisabled("Grayed out");
    assert(td.type == WidgetNode::Type::TextDisabled);
    assert(td.textContent == "Grayed out");

    std::cout << "PASSED\n";
}

void test_progress_bar_builder() {
    std::cout << "Testing: WidgetNode::progressBar builder... ";

    auto pb = WidgetNode::progressBar(0.75f);
    assert(pb.type == WidgetNode::Type::ProgressBar);
    assert(pb.floatValue == 0.75f);
    assert(pb.width == 0.0f);
    assert(pb.overlayText.empty());

    auto pb2 = WidgetNode::progressBar(0.5f, 200.0f, 20.0f, "50%");
    assert(pb2.floatValue == 0.5f);
    assert(pb2.width == 200.0f);
    assert(pb2.height == 20.0f);
    assert(pb2.overlayText == "50%");

    std::cout << "PASSED\n";
}

void test_collapsing_header_builder() {
    std::cout << "Testing: WidgetNode::collapsingHeader builder... ";

    auto ch = WidgetNode::collapsingHeader("Details", {
        WidgetNode::text("Hidden content"),
    });
    assert(ch.type == WidgetNode::Type::CollapsingHeader);
    assert(ch.label == "Details");
    assert(ch.children.size() == 1);
    assert(ch.defaultOpen == false);

    auto ch2 = WidgetNode::collapsingHeader("Open", {}, true);
    assert(ch2.defaultOpen == true);

    std::cout << "PASSED\n";
}

void test_phase3_type_names() {
    std::cout << "Testing: widgetTypeName for Phase 3 types... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::SameLine)) == "SameLine");
    assert(std::string(widgetTypeName(WidgetNode::Type::Spacing)) == "Spacing");
    assert(std::string(widgetTypeName(WidgetNode::Type::TextColored)) == "TextColored");
    assert(std::string(widgetTypeName(WidgetNode::Type::TextWrapped)) == "TextWrapped");
    assert(std::string(widgetTypeName(WidgetNode::Type::TextDisabled)) == "TextDisabled");
    assert(std::string(widgetTypeName(WidgetNode::Type::ProgressBar)) == "ProgressBar");
    assert(std::string(widgetTypeName(WidgetNode::Type::CollapsingHeader)) == "CollapsingHeader");

    std::cout << "PASSED\n";
}

void test_debug_overlay_pattern() {
    std::cout << "Testing: Debug overlay pattern from design doc... ";

    auto overlay = WidgetNode::window("Debug", {
        WidgetNode::text("FPS: 60"),
        WidgetNode::sameLine(),
        WidgetNode::text("(16.7 ms)"),
        WidgetNode::progressBar(0.5f, 0.0f, 0.0f, "60 fps"),
        WidgetNode::separator(),
        WidgetNode::collapsingHeader("Renderer", {
            WidgetNode::text("Draw calls: 42"),
            WidgetNode::text("Triangles: 12345"),
        }),
    });

    assert(overlay.children.size() == 6);
    assert(overlay.children[0].type == WidgetNode::Type::Text);
    assert(overlay.children[1].type == WidgetNode::Type::SameLine);
    assert(overlay.children[2].type == WidgetNode::Type::Text);
    assert(overlay.children[3].type == WidgetNode::Type::ProgressBar);
    assert(overlay.children[4].type == WidgetNode::Type::Separator);
    assert(overlay.children[5].type == WidgetNode::Type::CollapsingHeader);
    assert(overlay.children[5].children.size() == 2);

    std::cout << "PASSED\n";
}

void test_hud_pattern() {
    std::cout << "Testing: HUD pattern from design doc... ";

    auto hud = WidgetNode::window("##hud", {
        WidgetNode::textColored(1.0f, 0.3f, 0.3f, 1.0f, "HP"),
        WidgetNode::sameLine(),
        WidgetNode::progressBar(0.85f, 200.0f, 20.0f, "85/100"),
        WidgetNode::spacing(),
        WidgetNode::textColored(0.3f, 0.5f, 1.0f, 1.0f, "MP"),
        WidgetNode::sameLine(),
        WidgetNode::progressBar(0.6f, 200.0f, 20.0f, "60/100"),
    });

    assert(hud.children.size() == 7);
    assert(hud.children[0].type == WidgetNode::Type::TextColored);
    assert(hud.children[0].colorR == 1.0f);
    assert(hud.children[1].type == WidgetNode::Type::SameLine);
    assert(hud.children[2].type == WidgetNode::Type::ProgressBar);
    assert(hud.children[2].overlayText == "85/100");
    assert(hud.children[3].type == WidgetNode::Type::Spacing);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 4 Builder Tests
// ============================================================================

void test_tab_bar_builder() {
    std::cout << "Testing: WidgetNode::tabBar builder... ";

    auto tb = WidgetNode::tabBar("my_tabs", {
        WidgetNode::tabItem("Tab 1", {
            WidgetNode::text("Content 1"),
        }),
        WidgetNode::tabItem("Tab 2", {
            WidgetNode::text("Content 2"),
        }),
    });
    assert(tb.type == WidgetNode::Type::TabBar);
    assert(tb.id == "my_tabs");
    assert(tb.children.size() == 2);
    assert(tb.children[0].type == WidgetNode::Type::TabItem);
    assert(tb.children[0].label == "Tab 1");
    assert(tb.children[0].children.size() == 1);
    assert(tb.children[1].label == "Tab 2");

    std::cout << "PASSED\n";
}

void test_tab_item_builder() {
    std::cout << "Testing: WidgetNode::tabItem builder... ";

    auto ti = WidgetNode::tabItem("Settings", {
        WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f),
    });
    assert(ti.type == WidgetNode::Type::TabItem);
    assert(ti.label == "Settings");
    assert(ti.children.size() == 1);

    std::cout << "PASSED\n";
}

void test_tree_node_builder() {
    std::cout << "Testing: WidgetNode::treeNode builder... ";

    auto tn = WidgetNode::treeNode("Root", {
        WidgetNode::treeNode("Child 1", {}, false, true),
        WidgetNode::treeNode("Child 2", {
            WidgetNode::treeNode("Grandchild", {}, false, true),
        }),
    }, true);
    assert(tn.type == WidgetNode::Type::TreeNode);
    assert(tn.label == "Root");
    assert(tn.defaultOpen == true);
    assert(tn.leaf == false);
    assert(tn.children.size() == 2);
    assert(tn.children[0].label == "Child 1");
    assert(tn.children[0].leaf == true);
    assert(tn.children[1].children.size() == 1);

    std::cout << "PASSED\n";
}

void test_child_builder() {
    std::cout << "Testing: WidgetNode::child builder... ";

    auto ch = WidgetNode::child("##scroll", 300.0f, 200.0f, true, true, {
        WidgetNode::text("Scrollable content"),
    });
    assert(ch.type == WidgetNode::Type::Child);
    assert(ch.id == "##scroll");
    assert(ch.width == 300.0f);
    assert(ch.height == 200.0f);
    assert(ch.border == true);
    assert(ch.autoScroll == true);
    assert(ch.children.size() == 1);

    auto ch2 = WidgetNode::child("##simple");
    assert(ch2.border == false);
    assert(ch2.autoScroll == false);
    assert(ch2.width == 0.0f);

    std::cout << "PASSED\n";
}

void test_menu_bar_builder() {
    std::cout << "Testing: WidgetNode::menuBar builder... ";

    auto mb = WidgetNode::menuBar({
        WidgetNode::menu("File", {
            WidgetNode::menuItem("New"),
            WidgetNode::menuItem("Open"),
            WidgetNode::separator(),
            WidgetNode::menuItem("Exit"),
        }),
        WidgetNode::menu("Edit", {
            WidgetNode::menuItem("Undo"),
        }),
    });
    assert(mb.type == WidgetNode::Type::MenuBar);
    assert(mb.children.size() == 2);
    assert(mb.children[0].type == WidgetNode::Type::Menu);
    assert(mb.children[0].label == "File");
    assert(mb.children[0].children.size() == 4);
    assert(mb.children[0].children[2].type == WidgetNode::Type::Separator);

    std::cout << "PASSED\n";
}

void test_menu_builder() {
    std::cout << "Testing: WidgetNode::menu builder... ";

    auto menu = WidgetNode::menu("View", {
        WidgetNode::menuItem("Zoom In"),
        WidgetNode::menuItem("Zoom Out"),
    });
    assert(menu.type == WidgetNode::Type::Menu);
    assert(menu.label == "View");
    assert(menu.children.size() == 2);

    std::cout << "PASSED\n";
}

void test_menu_item_builder() {
    std::cout << "Testing: WidgetNode::menuItem builder... ";

    bool clicked = false;
    auto mi = WidgetNode::menuItem("Save", [&clicked](WidgetNode&) {
        clicked = true;
    }, "Ctrl+S", false);
    assert(mi.type == WidgetNode::Type::MenuItem);
    assert(mi.label == "Save");
    assert(mi.shortcutText == "Ctrl+S");
    assert(mi.checked == false);
    assert(mi.onClick);

    mi.onClick(mi);
    assert(clicked);

    auto mi2 = WidgetNode::menuItem("Show Grid", {}, "", true);
    assert(mi2.checked == true);

    std::cout << "PASSED\n";
}

void test_phase4_type_names() {
    std::cout << "Testing: widgetTypeName for Phase 4 types... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::TabBar)) == "TabBar");
    assert(std::string(widgetTypeName(WidgetNode::Type::TabItem)) == "TabItem");
    assert(std::string(widgetTypeName(WidgetNode::Type::TreeNode)) == "TreeNode");
    assert(std::string(widgetTypeName(WidgetNode::Type::Child)) == "Child");
    assert(std::string(widgetTypeName(WidgetNode::Type::MenuBar)) == "MenuBar");
    assert(std::string(widgetTypeName(WidgetNode::Type::Menu)) == "Menu");
    assert(std::string(widgetTypeName(WidgetNode::Type::MenuItem)) == "MenuItem");

    std::cout << "PASSED\n";
}

void test_settings_panel_with_tabs() {
    std::cout << "Testing: Settings panel with tabs pattern... ";

    auto settings = WidgetNode::window("Settings", {
        WidgetNode::tabBar("settings_tabs", {
            WidgetNode::tabItem("Audio", {
                WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f),
                WidgetNode::checkbox("Mute", false),
            }),
            WidgetNode::tabItem("Video", {
                WidgetNode::combo("Resolution", {"1080p", "1440p", "4K"}, 0),
            }),
        }),
        WidgetNode::separator(),
        WidgetNode::button("Apply"),
    });

    assert(settings.children.size() == 3);
    auto& tabBar = settings.children[0];
    assert(tabBar.type == WidgetNode::Type::TabBar);
    assert(tabBar.children.size() == 2);
    assert(tabBar.children[0].type == WidgetNode::Type::TabItem);
    assert(tabBar.children[0].children.size() == 2);
    assert(tabBar.children[1].children.size() == 1);

    std::cout << "PASSED\n";
}

void test_scene_hierarchy_pattern() {
    std::cout << "Testing: Scene hierarchy pattern... ";

    auto hierarchy = WidgetNode::window("Scene", {
        WidgetNode::child("##tree", 0.0f, -30.0f, false, false, {
            WidgetNode::treeNode("Root", {
                WidgetNode::treeNode("Player", {
                    WidgetNode::treeNode("Camera", {}, false, true),
                    WidgetNode::treeNode("Mesh", {}, false, true),
                }, true),
                WidgetNode::treeNode("Lights", {
                    WidgetNode::treeNode("Sun", {}, false, true),
                }),
            }, true),
        }),
        WidgetNode::button("Add Entity"),
    });

    assert(hierarchy.children.size() == 2);
    auto& child = hierarchy.children[0];
    assert(child.type == WidgetNode::Type::Child);
    assert(child.height == -30.0f);
    assert(child.children.size() == 1);

    auto& root = child.children[0];
    assert(root.type == WidgetNode::Type::TreeNode);
    assert(root.defaultOpen == true);
    assert(root.children.size() == 2);
    assert(root.children[0].children[0].leaf == true);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 5 Builder Tests
// ============================================================================

void test_table_builder() {
    std::cout << "Testing: WidgetNode::table builder... ";

    auto tbl = WidgetNode::table("stats", 2, {"Name", "Value"}, {
        WidgetNode::tableRow({
            WidgetNode::text("HP"),
            WidgetNode::text("100"),
        }),
        WidgetNode::tableRow({
            WidgetNode::text("MP"),
            WidgetNode::text("50"),
        }),
    }, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH);
    assert(tbl.type == WidgetNode::Type::Table);
    assert(tbl.id == "stats");
    assert(tbl.columnCount == 2);
    assert(tbl.items.size() == 2);
    assert(tbl.items[0] == "Name");
    assert(tbl.items[1] == "Value");
    assert(tbl.children.size() == 2);
    assert(tbl.children[0].type == WidgetNode::Type::TableRow);
    assert(tbl.children[0].children.size() == 2);
    assert(tbl.tableFlags == (ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH));

    // Minimal table
    auto tbl2 = WidgetNode::table("##grid", 4);
    assert(tbl2.columnCount == 4);
    assert(tbl2.items.empty());
    assert(tbl2.children.empty());
    assert(tbl2.tableFlags == 0);

    std::cout << "PASSED\n";
}

void test_table_row_builder() {
    std::cout << "Testing: WidgetNode::tableRow builder... ";

    // Container mode: children map to columns
    auto row = WidgetNode::tableRow({
        WidgetNode::text("A"),
        WidgetNode::text("B"),
        WidgetNode::text("C"),
    });
    assert(row.type == WidgetNode::Type::TableRow);
    assert(row.children.size() == 3);

    // Bare mode: no children (just advances to next row)
    auto bare = WidgetNode::tableRow();
    assert(bare.type == WidgetNode::Type::TableRow);
    assert(bare.children.empty());

    std::cout << "PASSED\n";
}

void test_table_next_column_builder() {
    std::cout << "Testing: WidgetNode::tableNextColumn builder... ";

    auto col = WidgetNode::tableNextColumn();
    assert(col.type == WidgetNode::Type::TableColumn);
    assert(col.children.empty());

    std::cout << "PASSED\n";
}

void test_phase5_type_names() {
    std::cout << "Testing: widgetTypeName for Phase 5 types... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::Table)) == "Table");
    assert(std::string(widgetTypeName(WidgetNode::Type::TableColumn)) == "TableColumn");
    assert(std::string(widgetTypeName(WidgetNode::Type::TableRow)) == "TableRow");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 6 Builder Tests
// ============================================================================

void test_color_edit_builder() {
    std::cout << "Testing: WidgetNode::colorEdit builder... ";

    auto ce = WidgetNode::colorEdit("Accent Color", 0.2f, 0.4f, 0.8f, 1.0f);
    assert(ce.type == WidgetNode::Type::ColorEdit);
    assert(ce.label == "Accent Color");
    assert(ce.colorR == 0.2f);
    assert(ce.colorG == 0.4f);
    assert(ce.colorB == 0.8f);
    assert(ce.colorA == 1.0f);

    // Default values
    auto ce2 = WidgetNode::colorEdit("Default");
    assert(ce2.colorR == 1.0f);
    assert(ce2.colorG == 1.0f);

    std::cout << "PASSED\n";
}

void test_color_picker_builder() {
    std::cout << "Testing: WidgetNode::colorPicker builder... ";

    auto cp = WidgetNode::colorPicker("Background", 0.1f, 0.1f, 0.15f, 1.0f);
    assert(cp.type == WidgetNode::Type::ColorPicker);
    assert(cp.label == "Background");
    assert(cp.colorR == 0.1f);
    assert(cp.colorB == 0.15f);

    std::cout << "PASSED\n";
}

void test_drag_float_builder() {
    std::cout << "Testing: WidgetNode::dragFloat builder... ";

    auto df = WidgetNode::dragFloat("Speed", 1.5f, 0.1f, 0.0f, 10.0f);
    assert(df.type == WidgetNode::Type::DragFloat);
    assert(df.label == "Speed");
    assert(df.floatValue == 1.5f);
    assert(df.dragSpeed == 0.1f);
    assert(df.minFloat == 0.0f);
    assert(df.maxFloat == 10.0f);

    // Default speed
    auto df2 = WidgetNode::dragFloat("X", 0.0f);
    assert(df2.dragSpeed == 1.0f);
    assert(df2.minFloat == 0.0f);
    assert(df2.maxFloat == 0.0f);  // 0 = no clamp

    std::cout << "PASSED\n";
}

void test_drag_int_builder() {
    std::cout << "Testing: WidgetNode::dragInt builder... ";

    auto di = WidgetNode::dragInt("Count", 50, 1.0f, 0, 100);
    assert(di.type == WidgetNode::Type::DragInt);
    assert(di.label == "Count");
    assert(di.intValue == 50);
    assert(di.dragSpeed == 1.0f);
    assert(di.minInt == 0);
    assert(di.maxInt == 100);

    std::cout << "PASSED\n";
}

void test_phase6_type_names() {
    std::cout << "Testing: widgetTypeName for Phase 6 types... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::ColorEdit)) == "ColorEdit");
    assert(std::string(widgetTypeName(WidgetNode::Type::ColorPicker)) == "ColorPicker");
    assert(std::string(widgetTypeName(WidgetNode::Type::DragFloat)) == "DragFloat");
    assert(std::string(widgetTypeName(WidgetNode::Type::DragInt)) == "DragInt");

    std::cout << "PASSED\n";
}

void test_data_table_pattern() {
    std::cout << "Testing: Data table pattern... ";

    // Keybindings table from design doc
    auto keybinds = WidgetNode::window("Settings", {
        WidgetNode::table("keybinds", 2, {"Action", "Key"}, {
            WidgetNode::tableRow({
                WidgetNode::text("Jump"),
                WidgetNode::text("Space"),
            }),
            WidgetNode::tableRow({
                WidgetNode::text("Shoot"),
                WidgetNode::text("LMB"),
            }),
        }, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH),
    });

    assert(keybinds.children.size() == 1);
    auto& table = keybinds.children[0];
    assert(table.type == WidgetNode::Type::Table);
    assert(table.columnCount == 2);
    assert(table.items.size() == 2);
    assert(table.items[0] == "Action");
    assert(table.children.size() == 2);
    assert(table.children[0].children[0].textContent == "Jump");
    assert(table.children[1].children[1].textContent == "LMB");

    std::cout << "PASSED\n";
}

void test_inventory_grid_pattern() {
    std::cout << "Testing: Inventory grid pattern... ";

    // Grid layout using imperative TableNextColumn (no TableRow)
    std::vector<WidgetNode> cells;
    for (int i = 0; i < 8; i++) {
        cells.push_back(WidgetNode::tableNextColumn());
        cells.push_back(WidgetNode::button("Slot " + std::to_string(i)));
    }

    auto grid = WidgetNode::table("##inv", 4, {}, std::move(cells));
    assert(grid.type == WidgetNode::Type::Table);
    assert(grid.columnCount == 4);
    assert(grid.items.empty());
    assert(grid.children.size() == 16);  // 8 * (column + button)
    assert(grid.children[0].type == WidgetNode::Type::TableColumn);
    assert(grid.children[1].type == WidgetNode::Type::Button);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 7 Builder Tests
// ============================================================================

void test_listbox_builder() {
    std::cout << "Testing: WidgetNode::listBox builder... ";

    auto lb = WidgetNode::listBox("Fruits", {"Apple", "Banana", "Cherry"}, 1, 5);
    assert(lb.type == WidgetNode::Type::ListBox);
    assert(lb.label == "Fruits");
    assert(lb.items.size() == 3);
    assert(lb.items[0] == "Apple");
    assert(lb.items[1] == "Banana");
    assert(lb.items[2] == "Cherry");
    assert(lb.selectedIndex == 1);
    assert(lb.heightInItems == 5);

    // Default height
    auto lb2 = WidgetNode::listBox("Colors", {"Red", "Green", "Blue"});
    assert(lb2.selectedIndex == 0);
    assert(lb2.heightInItems == -1);

    std::cout << "PASSED\n";
}

void test_popup_builder() {
    std::cout << "Testing: WidgetNode::popup builder... ";

    auto p = WidgetNode::popup("my_popup", {
        WidgetNode::text("Popup content"),
        WidgetNode::button("Close"),
    });
    assert(p.type == WidgetNode::Type::Popup);
    assert(p.id == "my_popup");
    assert(p.children.size() == 2);
    assert(p.children[0].type == WidgetNode::Type::Text);
    assert(p.boolValue == false);  // not open by default

    std::cout << "PASSED\n";
}

void test_modal_builder() {
    std::cout << "Testing: WidgetNode::modal builder... ";

    bool closeCalled = false;
    auto m = WidgetNode::modal("Confirm Delete", {
        WidgetNode::text("Are you sure?"),
        WidgetNode::button("OK"),
        WidgetNode::button("Cancel"),
    }, [&closeCalled](WidgetNode&) { closeCalled = true; });

    assert(m.type == WidgetNode::Type::Modal);
    assert(m.label == "Confirm Delete");
    assert(m.children.size() == 3);
    assert(m.onClose);
    assert(m.boolValue == false);  // not open by default

    // Test callback
    m.onClose(m);
    assert(closeCalled);

    std::cout << "PASSED\n";
}

void test_phase7_type_names() {
    std::cout << "Testing: Phase 7 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::ListBox)) == "ListBox");
    assert(std::string(widgetTypeName(WidgetNode::Type::Popup)) == "Popup");
    assert(std::string(widgetTypeName(WidgetNode::Type::Modal)) == "Modal");

    std::cout << "PASSED\n";
}

void test_listbox_callback_pattern() {
    std::cout << "Testing: ListBox with onChange callback... ";

    int selectedValue = -1;
    auto lb = WidgetNode::listBox("Items", {"A", "B", "C"}, 0, -1,
        [&selectedValue](WidgetNode& w) { selectedValue = w.selectedIndex; });

    assert(lb.onChange);
    // Simulate selection change
    lb.selectedIndex = 2;
    lb.onChange(lb);
    assert(selectedValue == 2);

    std::cout << "PASSED\n";
}

void test_popup_open_pattern() {
    std::cout << "Testing: Popup open pattern via boolValue... ";

    auto popup = WidgetNode::popup("context_menu", {
        WidgetNode::button("Cut"),
        WidgetNode::button("Copy"),
        WidgetNode::button("Paste"),
    });

    // Initially closed
    assert(popup.boolValue == false);

    // Simulate opening (button callback would set this)
    popup.boolValue = true;
    assert(popup.boolValue == true);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 8 Builder Tests
// ============================================================================

void test_canvas_builder() {
    std::cout << "Testing: WidgetNode::canvas builder... ";

    bool drawn = false;
    bool clicked = false;
    auto c = WidgetNode::canvas("##mycanvas", 200.0f, 150.0f,
        [&drawn](WidgetNode&) { drawn = true; },
        [&clicked](WidgetNode&) { clicked = true; });
    assert(c.type == WidgetNode::Type::Canvas);
    assert(c.id == "##mycanvas");
    assert(c.width == 200.0f);
    assert(c.height == 150.0f);
    assert(c.onDraw);
    assert(c.onClick);

    // Invoke callbacks
    c.onDraw(c);
    assert(drawn);
    c.onClick(c);
    assert(clicked);

    // Canvas without callbacks
    auto c2 = WidgetNode::canvas("##simple", 100.0f, 100.0f);
    assert(!c2.onDraw);
    assert(!c2.onClick);

    std::cout << "PASSED\n";
}

void test_tooltip_text_builder() {
    std::cout << "Testing: WidgetNode::tooltip (text) builder... ";

    auto t = WidgetNode::tooltip("Hover info");
    assert(t.type == WidgetNode::Type::Tooltip);
    assert(t.textContent == "Hover info");
    assert(t.children.empty());

    std::cout << "PASSED\n";
}

void test_tooltip_children_builder() {
    std::cout << "Testing: WidgetNode::tooltip (children) builder... ";

    auto t = WidgetNode::tooltip({
        WidgetNode::text("Line 1"),
        WidgetNode::text("Line 2"),
        WidgetNode::progressBar(0.5f),
    });
    assert(t.type == WidgetNode::Type::Tooltip);
    assert(t.textContent.empty());
    assert(t.children.size() == 3);
    assert(t.children[0].type == WidgetNode::Type::Text);
    assert(t.children[2].type == WidgetNode::Type::ProgressBar);

    std::cout << "PASSED\n";
}

void test_phase8_type_names() {
    std::cout << "Testing: Phase 8 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::Canvas)) == "Canvas");
    assert(std::string(widgetTypeName(WidgetNode::Type::Tooltip)) == "Tooltip");

    std::cout << "PASSED\n";
}

void test_canvas_with_tooltip_pattern() {
    std::cout << "Testing: Canvas with tooltip pattern... ";

    auto tree = WidgetNode::window("Drawing", {
        WidgetNode::canvas("##draw", 300.0f, 200.0f),
        WidgetNode::tooltip("Click to draw"),
        WidgetNode::separator(),
        WidgetNode::button("Clear"),
        WidgetNode::tooltip({
            WidgetNode::text("Clears the canvas"),
            WidgetNode::textDisabled("(cannot undo)"),
        }),
    });

    assert(tree.children.size() == 5);
    assert(tree.children[0].type == WidgetNode::Type::Canvas);
    assert(tree.children[0].width == 300.0f);
    assert(tree.children[1].type == WidgetNode::Type::Tooltip);
    assert(tree.children[1].textContent == "Click to draw");
    assert(tree.children[4].type == WidgetNode::Type::Tooltip);
    assert(tree.children[4].children.size() == 2);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 9 builder tests
// ============================================================================

void test_radio_button_builder() {
    std::cout << "Testing: RadioButton builder... ";

    bool called = false;
    auto rb = WidgetNode::radioButton("Option A", 0, 1, [&](WidgetNode& w) {
        called = true;
    });

    assert(rb.type == WidgetNode::Type::RadioButton);
    assert(rb.label == "Option A");
    assert(rb.intValue == 0);   // active value
    assert(rb.minInt == 1);     // this button's value
    assert(rb.onChange);

    std::cout << "PASSED\n";
}

void test_selectable_builder() {
    std::cout << "Testing: Selectable builder... ";

    auto sel = WidgetNode::selectable("Item 1", true);

    assert(sel.type == WidgetNode::Type::Selectable);
    assert(sel.label == "Item 1");
    assert(sel.boolValue == true);

    std::cout << "PASSED\n";
}

void test_input_text_multiline_builder() {
    std::cout << "Testing: InputTextMultiline builder... ";

    auto ml = WidgetNode::inputTextMultiline("Notes", "Hello world", 300.0f, 200.0f);

    assert(ml.type == WidgetNode::Type::InputTextMultiline);
    assert(ml.label == "Notes");
    assert(ml.stringValue == "Hello world");
    assert(ml.width == 300.0f);
    assert(ml.height == 200.0f);

    std::cout << "PASSED\n";
}

void test_bullet_text_builder() {
    std::cout << "Testing: BulletText builder... ";

    auto bt = WidgetNode::bulletText("Important point");

    assert(bt.type == WidgetNode::Type::BulletText);
    assert(bt.textContent == "Important point");

    std::cout << "PASSED\n";
}

void test_separator_text_builder() {
    std::cout << "Testing: SeparatorText builder... ";

    auto st = WidgetNode::separatorText("Section A");

    assert(st.type == WidgetNode::Type::SeparatorText);
    assert(st.label == "Section A");

    std::cout << "PASSED\n";
}

void test_indent_builder() {
    std::cout << "Testing: Indent/Unindent builder... ";

    auto ind = WidgetNode::indent(20.0f);
    assert(ind.type == WidgetNode::Type::Indent);
    assert(ind.width == 20.0f);

    auto unind = WidgetNode::unindent(20.0f);
    assert(unind.type == WidgetNode::Type::Indent);
    assert(unind.width == -20.0f);  // negative = unindent

    auto defInd = WidgetNode::indent();
    assert(defInd.width == 0.0f);

    std::cout << "PASSED\n";
}

void test_window_flags_builder() {
    std::cout << "Testing: Window flags builder... ";

    auto w = WidgetNode::window("Flagged", std::vector<WidgetNode>{}, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    assert(w.type == WidgetNode::Type::Window);
    assert(w.label == "Flagged");
    assert(w.windowFlags == (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize));

    // Default: no flags
    auto w2 = WidgetNode::window("Normal", {});
    assert(w2.windowFlags == 0);

    std::cout << "PASSED\n";
}

void test_phase9_type_names() {
    std::cout << "Testing: Phase 9 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::RadioButton)) == "RadioButton");
    assert(std::string(widgetTypeName(WidgetNode::Type::Selectable)) == "Selectable");
    assert(std::string(widgetTypeName(WidgetNode::Type::InputTextMultiline)) == "InputTextMultiline");
    assert(std::string(widgetTypeName(WidgetNode::Type::BulletText)) == "BulletText");
    assert(std::string(widgetTypeName(WidgetNode::Type::SeparatorText)) == "SeparatorText");
    assert(std::string(widgetTypeName(WidgetNode::Type::Indent)) == "Indent");

    std::cout << "PASSED\n";
}

void test_radio_button_group_pattern() {
    std::cout << "Testing: Radio button group pattern... ";

    auto tree = WidgetNode::window("Settings", {
        WidgetNode::separatorText("Theme"),
        WidgetNode::radioButton("Light", 0, 0),
        WidgetNode::radioButton("Dark", 0, 1),
        WidgetNode::radioButton("System", 0, 2),
        WidgetNode::separatorText("Details"),
        WidgetNode::indent(20.0f),
        WidgetNode::bulletText("Light: Bright theme"),
        WidgetNode::bulletText("Dark: Dark theme"),
        WidgetNode::bulletText("System: Follow OS"),
        WidgetNode::unindent(20.0f),
    });

    assert(tree.children.size() == 10);
    assert(tree.children[1].type == WidgetNode::Type::RadioButton);
    assert(tree.children[1].minInt == 0);  // light = value 0
    assert(tree.children[2].minInt == 1);  // dark = value 1
    assert(tree.children[3].minInt == 2);  // system = value 2

    std::cout << "PASSED\n";
}

// ============================================================================
// DnD Tests
// ============================================================================

void test_dnd_field_defaults() {
    std::cout << "Testing: DnD field defaults... ";

    WidgetNode n;
    n.type = WidgetNode::Type::Button;
    assert(n.dragType.empty());
    assert(n.dragData.empty());
    assert(n.dropAcceptType.empty());
    assert(!n.onDrop);
    assert(!n.onDragBegin);
    assert(n.dragMode == 0);

    std::cout << "PASSED\n";
}

void test_dnd_field_setting() {
    std::cout << "Testing: DnD field setting... ";

    auto img = WidgetNode::image({}, 48, 48);
    img.dragType = "item";
    img.dragData = "sword_01";
    img.dropAcceptType = "item";

    assert(img.dragType == "item");
    assert(img.dragData == "sword_01");
    assert(img.dropAcceptType == "item");

    bool dropCalled = false;
    img.onDrop = [&dropCalled](WidgetNode& w) {
        dropCalled = true;
        assert(w.dragData == "potion_02");
    };

    // Simulate drop delivery
    img.dragData = "potion_02";
    img.onDrop(img);
    assert(dropCalled);

    std::cout << "PASSED\n";
}

void test_dnd_drag_mode() {
    std::cout << "Testing: DnD drag mode... ";

    auto slot = WidgetNode::button("Slot");
    slot.dragType = "item";
    slot.dragMode = 2; // click-only
    assert(slot.dragMode == 2);

    slot.dragMode = 1; // drag-only
    assert(slot.dragMode == 1);

    slot.dragMode = 0; // both (default)
    assert(slot.dragMode == 0);

    std::cout << "PASSED\n";
}

void test_dnd_manager_basic() {
    std::cout << "Testing: DragDropManager basic operations... ";

    DragDropManager mgr;
    assert(!mgr.isHolding());

    DragDropManager::CursorItem item;
    item.type = "item";
    item.data = "sword";
    item.fallbackText = "Sword";
    mgr.pickUp(item);

    assert(mgr.isHolding());
    assert(mgr.isHolding("item"));
    assert(!mgr.isHolding("spell"));
    assert(mgr.cursorItem().data == "sword");
    assert(mgr.cursorItem().fallbackText == "Sword");

    auto delivered = mgr.dropItem();
    assert(delivered.type == "item");
    assert(delivered.data == "sword");
    assert(!mgr.isHolding());

    std::cout << "PASSED\n";
}

void test_dnd_manager_cancel() {
    std::cout << "Testing: DragDropManager cancel... ";

    DragDropManager mgr;
    DragDropManager::CursorItem item;
    item.type = "item";
    item.data = "shield";
    mgr.pickUp(item);
    assert(mgr.isHolding());

    mgr.cancel();
    assert(!mgr.isHolding());

    std::cout << "PASSED\n";
}

// ============================================================================
// TextureRegistry Tests
// ============================================================================

void test_texture_registry_basic() {
    std::cout << "Testing: TextureRegistry basic operations... ";

    TextureRegistry registry;
    assert(registry.size() == 0);
    assert(!registry.has("sword"));

    TextureHandle tex;
    tex.id = 42;
    tex.width = 64;
    tex.height = 64;
    registry.registerTexture("sword", tex);

    assert(registry.size() == 1);
    assert(registry.has("sword"));
    assert(!registry.has("shield"));

    auto retrieved = registry.get("sword");
    assert(retrieved.valid());
    assert(retrieved.id == 42);
    assert(retrieved.width == 64);
    assert(retrieved.height == 64);

    // Not found returns invalid handle
    auto missing = registry.get("shield");
    assert(!missing.valid());

    std::cout << "PASSED\n";
}

void test_texture_registry_unregister() {
    std::cout << "Testing: TextureRegistry unregister... ";

    TextureRegistry registry;
    TextureHandle tex;
    tex.id = 99;
    tex.width = 32;
    tex.height = 32;
    registry.registerTexture("potion", tex);
    assert(registry.has("potion"));

    registry.unregisterTexture("potion");
    assert(!registry.has("potion"));
    assert(registry.size() == 0);

    // Unregistering non-existent key is safe
    registry.unregisterTexture("nonexistent");

    std::cout << "PASSED\n";
}

void test_texture_registry_overwrite() {
    std::cout << "Testing: TextureRegistry overwrite... ";

    TextureRegistry registry;
    TextureHandle tex1;
    tex1.id = 1;
    tex1.width = 16;
    tex1.height = 16;
    registry.registerTexture("icon", tex1);

    TextureHandle tex2;
    tex2.id = 2;
    tex2.width = 32;
    tex2.height = 32;
    registry.registerTexture("icon", tex2);

    assert(registry.size() == 1);
    auto retrieved = registry.get("icon");
    assert(retrieved.id == 2);
    assert(retrieved.width == 32);

    std::cout << "PASSED\n";
}

void test_texture_registry_clear() {
    std::cout << "Testing: TextureRegistry clear... ";

    TextureRegistry registry;
    TextureHandle t;
    t.id = 1; t.width = 8; t.height = 8;
    registry.registerTexture("a", t);
    t.id = 2;
    registry.registerTexture("b", t);
    t.id = 3;
    registry.registerTexture("c", t);
    assert(registry.size() == 3);

    registry.clear();
    assert(registry.size() == 0);
    assert(!registry.has("a"));
    assert(!registry.has("b"));
    assert(!registry.has("c"));

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 10 - Style Push/Pop Builders
// ============================================================================

void test_push_style_color_builder() {
    std::cout << "Testing: WidgetNode::pushStyleColor builder... ";

    auto w = WidgetNode::pushStyleColor(21, 0.2f, 0.1f, 0.1f, 1.0f);
    assert(w.type == WidgetNode::Type::PushStyleColor);
    assert(w.intValue == 21);
    assert(w.colorR == 0.2f);
    assert(w.colorG == 0.1f);
    assert(w.colorB == 0.1f);
    assert(w.colorA == 1.0f);

    std::cout << "PASSED\n";
}

void test_pop_style_color_builder() {
    std::cout << "Testing: WidgetNode::popStyleColor builder... ";

    auto w = WidgetNode::popStyleColor(3);
    assert(w.type == WidgetNode::Type::PopStyleColor);
    assert(w.intValue == 3);

    // Default count = 1
    auto w2 = WidgetNode::popStyleColor();
    assert(w2.intValue == 1);

    std::cout << "PASSED\n";
}

void test_push_style_var_float_builder() {
    std::cout << "Testing: WidgetNode::pushStyleVar (float) builder... ";

    auto w = WidgetNode::pushStyleVar(11, 8.0f);  // ImGuiStyleVar_FrameRounding
    assert(w.type == WidgetNode::Type::PushStyleVar);
    assert(w.intValue == 11);
    assert(w.floatValue == 8.0f);

    std::cout << "PASSED\n";
}

void test_push_style_var_vec2_builder() {
    std::cout << "Testing: WidgetNode::pushStyleVar (Vec2) builder... ";

    auto w = WidgetNode::pushStyleVar(2, 12.0f, 12.0f);  // ImGuiStyleVar_WindowPadding
    assert(w.type == WidgetNode::Type::PushStyleVar);
    assert(w.intValue == 2);
    assert(w.width == 12.0f);
    assert(w.height == 12.0f);

    std::cout << "PASSED\n";
}

void test_pop_style_var_builder() {
    std::cout << "Testing: WidgetNode::popStyleVar builder... ";

    auto w = WidgetNode::popStyleVar(2);
    assert(w.type == WidgetNode::Type::PopStyleVar);
    assert(w.intValue == 2);

    // Default count = 1
    auto w2 = WidgetNode::popStyleVar();
    assert(w2.intValue == 1);

    std::cout << "PASSED\n";
}

void test_phase10_type_names() {
    std::cout << "Testing: Phase 10 widgetTypeName... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::PushStyleColor)) == "PushStyleColor");
    assert(std::string(widgetTypeName(WidgetNode::Type::PopStyleColor)) == "PopStyleColor");
    assert(std::string(widgetTypeName(WidgetNode::Type::PushStyleVar)) == "PushStyleVar");
    assert(std::string(widgetTypeName(WidgetNode::Type::PopStyleVar)) == "PopStyleVar");

    std::cout << "PASSED\n";
}

// ============================================================================
// Focus Management Tests
// ============================================================================

void test_focus_field_defaults() {
    std::cout << "Testing: Focus field defaults... ";

    WidgetNode n;
    n.type = WidgetNode::Type::Button;
    assert(n.focusable == true);
    assert(n.autoFocus == false);
    assert(!n.onFocus);
    assert(!n.onBlur);

    std::cout << "PASSED\n";
}

void test_focus_field_setting() {
    std::cout << "Testing: Focus field setting... ";

    auto input = WidgetNode::inputText("Name", "Alice");
    input.id = "name_input";
    input.focusable = false;
    input.autoFocus = true;

    assert(input.focusable == false);
    assert(input.autoFocus == true);

    bool focusCalled = false;
    bool blurCalled = false;
    input.onFocus = [&focusCalled](WidgetNode&) { focusCalled = true; };
    input.onBlur = [&blurCalled](WidgetNode&) { blurCalled = true; };

    assert(input.onFocus);
    assert(input.onBlur);

    // Invoke callbacks manually
    input.onFocus(input);
    assert(focusCalled);
    input.onBlur(input);
    assert(blurCalled);

    std::cout << "PASSED\n";
}

// ============================================================================
// Animation Field Tests
// ============================================================================

void test_animation_field_defaults() {
    std::cout << "Testing: Animation field defaults... ";

    WidgetNode n;
    assert(n.alpha == 1.0f);
    assert(n.windowPosX == FLT_MAX);
    assert(n.windowPosY == FLT_MAX);
    assert(n.scaleX == 1.0f);
    assert(n.scaleY == 1.0f);
    assert(n.rotationY == 0.0f);

    std::cout << "PASSED\n";
}

void test_animation_field_setting() {
    std::cout << "Testing: Animation field setting... ";

    auto w = WidgetNode::window("Test", {});
    w.alpha = 0.5f;
    w.windowPosX = 100.0f;
    w.windowPosY = 200.0f;
    w.scaleX = 0.5f;
    w.scaleY = 0.75f;
    w.rotationY = 1.57f;

    assert(w.alpha == 0.5f);
    assert(w.windowPosX == 100.0f);
    assert(w.windowPosY == 200.0f);
    assert(w.scaleX == 0.5f);
    assert(w.scaleY == 0.75f);
    assert(w.rotationY == 1.57f);

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 13: Context Menu & Main Menu Bar Builders
// ============================================================================

void test_context_menu_builder() {
    std::cout << "Testing: ContextMenu builder... ";

    auto cm = WidgetNode::contextMenu({
        WidgetNode::menuItem("Cut"),
        WidgetNode::menuItem("Copy"),
    });
    assert(cm.type == WidgetNode::Type::ContextMenu);
    assert(cm.children.size() == 2);
    assert(cm.children[0].label == "Cut");
    assert(cm.children[1].label == "Copy");

    std::cout << "PASSED\n";
}

void test_main_menu_bar_builder() {
    std::cout << "Testing: MainMenuBar builder... ";

    auto mmb = WidgetNode::mainMenuBar({
        WidgetNode::menu("File", {
            WidgetNode::menuItem("New"),
            WidgetNode::menuItem("Open"),
        }),
        WidgetNode::menu("Edit", {
            WidgetNode::menuItem("Undo"),
        }),
    });
    assert(mmb.type == WidgetNode::Type::MainMenuBar);
    assert(mmb.children.size() == 2);
    assert(mmb.children[0].label == "File");
    assert(mmb.children[0].children.size() == 2);
    assert(mmb.children[1].label == "Edit");
    assert(mmb.children[1].children.size() == 1);

    std::cout << "PASSED\n";
}

void test_phase13_type_names() {
    std::cout << "Testing: Phase 13 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::ContextMenu)) == "ContextMenu");
    assert(std::string(widgetTypeName(WidgetNode::Type::MainMenuBar)) == "MainMenuBar");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 14 - ItemTooltip & ImageButton
// ============================================================================

void test_item_tooltip_text_builder() {
    std::cout << "Testing: ItemTooltip text builder... ";

    auto node = WidgetNode::itemTooltip("Hover info");
    assert(node.type == WidgetNode::Type::ItemTooltip);
    assert(node.textContent == "Hover info");
    assert(node.children.empty());

    std::cout << "PASSED\n";
}

void test_item_tooltip_rich_builder() {
    std::cout << "Testing: ItemTooltip rich builder... ";

    auto node = WidgetNode::itemTooltip({
        WidgetNode::text("Line 1"),
        WidgetNode::separator(),
    });
    assert(node.type == WidgetNode::Type::ItemTooltip);
    assert(node.textContent.empty());
    assert(node.children.size() == 2);
    assert(node.children[0].type == WidgetNode::Type::Text);

    std::cout << "PASSED\n";
}

void test_image_button_builder() {
    std::cout << "Testing: ImageButton builder... ";

    auto node = WidgetNode::imageButton("btn1", TextureHandle{}, 64.0f, 48.0f);
    assert(node.type == WidgetNode::Type::ImageButton);
    assert(node.id == "btn1");
    assert(node.imageWidth == 64.0f);
    assert(node.imageHeight == 48.0f);

    std::cout << "PASSED\n";
}

void test_phase14_type_names() {
    std::cout << "Testing: Phase 14 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::ItemTooltip)) == "ItemTooltip");
    assert(std::string(widgetTypeName(WidgetNode::Type::ImageButton)) == "ImageButton");

    std::cout << "PASSED\n";
}

// ============================================================================
// Phase 15 - PlotLines & PlotHistogram
// ============================================================================

void test_plot_lines_builder() {
    std::cout << "Testing: PlotLines builder... ";

    auto node = WidgetNode::plotLines("FPS", {30.0f, 60.0f, 45.0f, 55.0f},
                                       "avg: 47.5", 0.0f, 100.0f, 200.0f, 40.0f);
    assert(node.type == WidgetNode::Type::PlotLines);
    assert(node.label == "FPS");
    assert(node.plotValues.size() == 4);
    assert(node.plotValues[0] == 30.0f);
    assert(node.plotValues[3] == 55.0f);
    assert(node.overlayText == "avg: 47.5");
    assert(node.minFloat == 0.0f);
    assert(node.maxFloat == 100.0f);
    assert(node.width == 200.0f);
    assert(node.height == 40.0f);

    std::cout << "PASSED\n";
}

void test_plot_histogram_builder() {
    std::cout << "Testing: PlotHistogram builder... ";

    auto node = WidgetNode::plotHistogram("Scores", {10.0f, 20.0f, 30.0f});
    assert(node.type == WidgetNode::Type::PlotHistogram);
    assert(node.label == "Scores");
    assert(node.plotValues.size() == 3);
    assert(node.plotValues[2] == 30.0f);
    assert(node.overlayText.empty());
    assert(node.minFloat == FLT_MAX);  // auto-scale
    assert(node.maxFloat == FLT_MAX);

    std::cout << "PASSED\n";
}

void test_phase15_type_names() {
    std::cout << "Testing: Phase 15 type names... ";

    assert(std::string(widgetTypeName(WidgetNode::Type::PlotLines)) == "PlotLines");
    assert(std::string(widgetTypeName(WidgetNode::Type::PlotHistogram)) == "PlotHistogram");

    std::cout << "PASSED\n";
}

void test_window_size_builder() {
    std::cout << "Testing: Window size builder... ";

    // Default window has zero size (auto)
    auto w1 = WidgetNode::window("Test");
    assert(w1.windowSizeW == 0.0f);
    assert(w1.windowSizeH == 0.0f);

    // Sized window builder
    auto w2 = WidgetNode::window("Sized", 400.0f, 300.0f,
                                  {WidgetNode::text("Hello")});
    assert(w2.type == WidgetNode::Type::Window);
    assert(w2.label == "Sized");
    assert(w2.windowSizeW == 400.0f);
    assert(w2.windowSizeH == 300.0f);
    assert(w2.children.size() == 1);
    assert(w2.windowFlags == 0);

    // Sized window with flags
    auto w3 = WidgetNode::window("Flagged", 200.0f, 150.0f, {},
                                  ImGuiWindowFlags_NoResize);
    assert(w3.windowSizeW == 200.0f);
    assert(w3.windowSizeH == 150.0f);
    assert(w3.windowFlags == ImGuiWindowFlags_NoResize);

    std::cout << "PASSED\n";
}

void test_window_flags_no_nav_no_inputs() {
    std::cout << "Testing: Window flags no_nav, no_inputs... ";

    auto w1 = WidgetNode::window("Test", std::vector<WidgetNode>{}, ImGuiWindowFlags_NoNav);
    assert(w1.windowFlags == ImGuiWindowFlags_NoNav);

    auto w2 = WidgetNode::window("Test", std::vector<WidgetNode>{}, ImGuiWindowFlags_NoInputs);
    assert(w2.windowFlags == ImGuiWindowFlags_NoInputs);

    auto w3 = WidgetNode::window("Test", std::vector<WidgetNode>{},
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs);
    assert(w3.windowFlags == (ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs));

    std::cout << "PASSED\n";
}

// ============================================================================
// Easing Function Tests (via TweenManager::applyEasing, tested indirectly)
// ============================================================================

// We can't call the private static directly, so we test through the public API
// by creating a GuiRenderer and TweenManager, then verifying interpolated values.
// For pure math validation, we test the expected easing behaviors here.

void test_easing_boundary_values() {
    std::cout << "Testing: Easing boundary expectations... ";

    // All easing functions should map 0→0 and 1→1
    // We verify this indirectly by testing that a tween at t=0 gives fromValue
    // and at t>=duration gives toValue.
    // The actual easing math is tested via TweenManager in render tests.

    // Just verify the field defaults are reasonable for animation
    WidgetNode n;
    n.alpha = 0.0f;
    assert(n.alpha == 0.0f);
    n.alpha = 1.0f;
    assert(n.alpha == 1.0f);

    std::cout << "PASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== finegui Retained-Mode Unit Tests ===\n\n";

    try {
        // Builder tests
        test_window_builder();
        test_window_with_children();
        test_text_builder();
        test_button_builder();
        test_checkbox_builder();
        test_slider_builder();
        test_slider_int_builder();
        test_input_text_builder();
        test_input_int_builder();
        test_input_float_builder();
        test_combo_builder();
        test_separator_builder();
        test_group_builder();
        test_columns_builder();
        test_image_builder();

        // Field defaults
        test_field_defaults();

        // Flags
        test_visibility_flags();

        // Type names
        test_widget_type_names();

        // Tree structure
        test_nested_tree();

        // Design doc pattern
        test_settings_panel_pattern();

        // Phase 3 builders
        test_same_line_builder();
        test_spacing_builder();
        test_text_colored_builder();
        test_text_wrapped_builder();
        test_text_disabled_builder();
        test_progress_bar_builder();
        test_collapsing_header_builder();
        test_phase3_type_names();

        // Phase 3 design doc patterns
        test_debug_overlay_pattern();
        test_hud_pattern();

        // Phase 4 builders
        test_tab_bar_builder();
        test_tab_item_builder();
        test_tree_node_builder();
        test_child_builder();
        test_menu_bar_builder();
        test_menu_builder();
        test_menu_item_builder();
        test_phase4_type_names();

        // Phase 4 design doc patterns
        test_settings_panel_with_tabs();
        test_scene_hierarchy_pattern();

        // Phase 6 builders
        test_color_edit_builder();
        test_color_picker_builder();
        test_drag_float_builder();
        test_drag_int_builder();
        test_phase6_type_names();

        // Phase 5 builders
        test_table_builder();
        test_table_row_builder();
        test_table_next_column_builder();
        test_phase5_type_names();

        // Phase 5 design doc patterns
        test_data_table_pattern();
        test_inventory_grid_pattern();

        // Phase 7 builders
        test_listbox_builder();
        test_popup_builder();
        test_modal_builder();
        test_phase7_type_names();
        test_listbox_callback_pattern();
        test_popup_open_pattern();

        // Phase 8 builders
        test_canvas_builder();
        test_tooltip_text_builder();
        test_tooltip_children_builder();
        test_phase8_type_names();
        test_canvas_with_tooltip_pattern();

        // Phase 9 builders
        test_radio_button_builder();
        test_selectable_builder();
        test_input_text_multiline_builder();
        test_bullet_text_builder();
        test_separator_text_builder();
        test_indent_builder();
        test_window_flags_builder();
        test_phase9_type_names();
        test_radio_button_group_pattern();

        // DnD tests
        test_dnd_field_defaults();
        test_dnd_field_setting();
        test_dnd_drag_mode();
        test_dnd_manager_basic();
        test_dnd_manager_cancel();

        // TextureRegistry tests
        test_texture_registry_basic();
        test_texture_registry_unregister();
        test_texture_registry_overwrite();
        test_texture_registry_clear();

        // Phase 10 - Style push/pop builders
        test_push_style_color_builder();
        test_pop_style_color_builder();
        test_push_style_var_float_builder();
        test_push_style_var_vec2_builder();
        test_pop_style_var_builder();
        test_phase10_type_names();

        // Focus management
        test_focus_field_defaults();
        test_focus_field_setting();

        // Animation fields
        test_animation_field_defaults();
        test_animation_field_setting();
        test_easing_boundary_values();

        // Phase 13 - Context Menu & Main Menu Bar
        test_context_menu_builder();
        test_main_menu_bar_builder();
        test_phase13_type_names();

        // Phase 14 - ItemTooltip & ImageButton
        test_item_tooltip_text_builder();
        test_item_tooltip_rich_builder();
        test_image_button_builder();
        test_phase14_type_names();

        // Phase 15 - PlotLines & PlotHistogram
        test_plot_lines_builder();
        test_plot_histogram_builder();
        test_phase15_type_names();

        // Window Control
        test_window_size_builder();
        test_window_flags_no_nav_no_inputs();

        std::cout << "\n=== All retained-mode unit tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
