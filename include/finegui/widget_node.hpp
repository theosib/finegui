#pragma once

#include <string>
#include <vector>
#include <functional>
#include "texture_handle.hpp"

namespace finegui {

struct WidgetNode;

/// Callback type for widget events.
/// The callback receives the widget node that triggered it.
using WidgetCallback = std::function<void(WidgetNode& widget)>;

/// A single node in the retained-mode widget tree.
struct WidgetNode {
    /// Widget type - determines which ImGui calls to make.
    enum class Type {
        // Phase 1 - Core widgets
        Window, Text, Button, Checkbox, Slider, SliderInt,
        InputText, InputInt, InputFloat,
        Combo, Separator, Group, Columns, Image,
        // Phase 2 - Layout & Navigation
        TabBar, TabItem, TreeNode, CollapsingHeader,
        MenuBar, Menu, MenuItem,
        // Phase 3 - Advanced
        Table, TableColumn, TableRow,
        ColorEdit, ColorPicker,
        DragFloat, DragInt,
        ListBox, Popup, Modal,
        // Phase 4 - Custom
        Canvas, ProgressBar, Tooltip
    };

    Type type;

    /// Display properties - which fields are used depends on type.
    std::string label;              // button text, window title, etc.
    std::string textContent;        // static text content
    std::string id;                 // ImGui ID (for disambiguating widgets)

    /// Value storage - widgets that hold state use these.
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    std::string stringValue;
    int selectedIndex = -1;         // for Combo, ListBox

    /// Range constraints (sliders, drags).
    float minFloat = 0.0f, maxFloat = 1.0f;
    int minInt = 0, maxInt = 100;

    /// Layout properties.
    float width = 0.0f;            // 0 = auto
    float height = 0.0f;
    int columnCount = 1;

    /// Items list (for Combo, ListBox).
    std::vector<std::string> items;

    /// Children (for Window, Group, Columns, TabBar, etc.)
    std::vector<WidgetNode> children;

    /// Visibility / enabled state.
    bool visible = true;
    bool enabled = true;

    /// Callbacks - invoked by GuiRenderer when interactions occur.
    WidgetCallback onClick;         // Button, MenuItem, Image
    WidgetCallback onChange;        // Checkbox, Slider, Input, Combo, ColorEdit
    WidgetCallback onSubmit;        // InputText (Enter pressed)
    WidgetCallback onClose;         // Window close button

    /// Texture handle (for Image widgets).
    TextureHandle texture{};
    float imageWidth = 0.0f;
    float imageHeight = 0.0f;

    // -- Convenience builders ------------------------------------------------

    static WidgetNode window(std::string title, std::vector<WidgetNode> children = {});
    static WidgetNode text(std::string content);
    static WidgetNode button(std::string label, WidgetCallback onClick = {});
    static WidgetNode checkbox(std::string label, bool value, WidgetCallback onChange = {});
    static WidgetNode slider(std::string label, float value, float min, float max,
                             WidgetCallback onChange = {});
    static WidgetNode sliderInt(std::string label, int value, int min, int max,
                                WidgetCallback onChange = {});
    static WidgetNode inputText(std::string label, std::string value,
                                WidgetCallback onChange = {}, WidgetCallback onSubmit = {});
    static WidgetNode inputInt(std::string label, int value, WidgetCallback onChange = {});
    static WidgetNode inputFloat(std::string label, float value, WidgetCallback onChange = {});
    static WidgetNode combo(std::string label, std::vector<std::string> items,
                            int selected, WidgetCallback onChange = {});
    static WidgetNode separator();
    static WidgetNode group(std::vector<WidgetNode> children);
    static WidgetNode columns(int count, std::vector<WidgetNode> children);
    static WidgetNode image(TextureHandle texture, float width, float height);
};

/// Returns a human-readable name for a widget type (for debug/placeholder text).
const char* widgetTypeName(WidgetNode::Type type);

} // namespace finegui
