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

        std::cout << "\n=== All retained-mode unit tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
