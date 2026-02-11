#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cfloat>
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
        // Phase 3 - Layout & Display
        SameLine, Spacing,
        TextColored, TextWrapped, TextDisabled,
        ProgressBar, CollapsingHeader,
        // Phase 4 - Containers & Menus
        TabBar, TabItem, TreeNode, Child,
        MenuBar, Menu, MenuItem,
        // Phase 5 - Tables
        Table, TableColumn, TableRow,
        // Phase 6 - Advanced Input
        ColorEdit, ColorPicker,
        DragFloat, DragInt,
        // Phase 7 - Misc
        ListBox, Popup, Modal,
        // Phase 8 - Custom
        Canvas, Tooltip,
        // Phase 9 - New widgets
        RadioButton, Selectable, InputTextMultiline,
        BulletText, SeparatorText, Indent,
        // Phase 10 - Style push/pop
        PushStyleColor, PopStyleColor, PushStyleVar, PopStyleVar,
        // Phase 11 - Layout helpers
        Dummy, NewLine,
        // Phase 12 - Advanced Input (continued)
        DragFloat3, InputTextWithHint, SliderAngle, SmallButton, ColorButton,
        // Phase 13 - Menus & Popups (continued)
        ContextMenu, MainMenuBar
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

    /// Color (for TextColored, ProgressBar overlay, etc.) - RGBA 0-1.
    float colorR = 1.0f, colorG = 1.0f, colorB = 1.0f, colorA = 1.0f;

    /// Overlay text (for ProgressBar).
    std::string overlayText;

    /// Offset (for SameLine).
    float offsetX = 0.0f;

    /// Animation: window alpha (1.0 = fully opaque).
    float alpha = 1.0f;
    /// Animation: explicit window position (FLT_MAX = ImGui auto-positioning).
    float windowPosX = FLT_MAX;
    float windowPosY = FLT_MAX;
    /// Animation: window scale (1.0 = normal size, 0.0 = collapsed to center).
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    /// Animation: Y-axis rotation in radians (0 = facing forward, PI = flipped).
    float rotationY = 0.0f;

    /// Default-open state (for CollapsingHeader, TreeNode).
    bool defaultOpen = false;

    /// Child window properties.
    bool border = false;
    bool autoScroll = false;

    /// TreeNode properties.
    bool leaf = false;

    /// MenuItem properties.
    std::string shortcutText;
    bool checked = false;

    /// Table properties.
    int tableFlags = 0;        // ImGuiTableFlags bitmask

    /// Window properties.
    int windowFlags = 0;       // ImGuiWindowFlags bitmask

    /// Drag widget properties.
    float dragSpeed = 1.0f;

    /// DragFloat3 values (3-component vector).
    float floatX = 0.0f, floatY = 0.0f, floatZ = 0.0f;

    /// Hint text (for InputTextWithHint placeholder).
    std::string hintText;

    /// ListBox properties.
    int heightInItems = -1;   // -1 = auto height

    /// Canvas callback - called each frame to do custom drawing.
    /// User can call ImGui::GetWindowDrawList() in the callback.
    WidgetCallback onDraw;

    // -- Drag and Drop --------------------------------------------------------

    /// DnD type string (e.g., "item"). Empty = not a drag source.
    std::string dragType;

    /// Payload data string carried during drag.
    std::string dragData;

    /// Accepted DnD type. Empty = not a drop target.
    std::string dropAcceptType;

    /// Called on the DROP TARGET when an item is delivered.
    /// node.dragData will contain the delivered payload data.
    WidgetCallback onDrop;

    /// Called on the DRAG SOURCE when a drag/pick-up begins.
    WidgetCallback onDragBegin;

    /// Drag mode: 0 = both traditional + click-to-pick-up,
    ///            1 = traditional drag only,
    ///            2 = click-to-pick-up only.
    int dragMode = 0;

    // -- Focus management ----------------------------------------------------

    /// Whether this widget participates in tab navigation (default: true).
    /// Set to false to skip this widget when tabbing.
    bool focusable = true;

    /// Focus this widget when its parent window first appears.
    bool autoFocus = false;

    /// Called when this widget gains keyboard focus.
    WidgetCallback onFocus;

    /// Called when this widget loses keyboard focus.
    WidgetCallback onBlur;

    // -- Convenience builders ------------------------------------------------

    static WidgetNode window(std::string title, std::vector<WidgetNode> children = {},
                             int flags = 0);
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

    // Phase 3 builders
    static WidgetNode sameLine(float offset = 0.0f);
    static WidgetNode spacing();
    static WidgetNode textColored(float r, float g, float b, float a, std::string content);
    static WidgetNode textWrapped(std::string content);
    static WidgetNode textDisabled(std::string content);
    static WidgetNode progressBar(float fraction, float width = 0.0f, float height = 0.0f,
                                  std::string overlay = "");
    static WidgetNode collapsingHeader(std::string label, std::vector<WidgetNode> children = {},
                                       bool defaultOpen = false);

    // Phase 4 builders
    static WidgetNode tabBar(std::string id, std::vector<WidgetNode> children = {});
    static WidgetNode tabItem(std::string label, std::vector<WidgetNode> children = {});
    static WidgetNode treeNode(std::string label, std::vector<WidgetNode> children = {},
                               bool defaultOpen = false, bool leaf = false);
    static WidgetNode child(std::string id, float width = 0.0f, float height = 0.0f,
                            bool border = false, bool autoScroll = false,
                            std::vector<WidgetNode> children = {});
    static WidgetNode menuBar(std::vector<WidgetNode> children = {});
    static WidgetNode menu(std::string label, std::vector<WidgetNode> children = {});
    static WidgetNode menuItem(std::string label, WidgetCallback onClick = {},
                               std::string shortcut = "", bool checked = false);

    // Phase 6 builders
    static WidgetNode colorEdit(std::string label, float r = 1.0f, float g = 1.0f,
                                float b = 1.0f, float a = 1.0f,
                                WidgetCallback onChange = {});
    static WidgetNode colorPicker(std::string label, float r = 1.0f, float g = 1.0f,
                                  float b = 1.0f, float a = 1.0f,
                                  WidgetCallback onChange = {});
    static WidgetNode dragFloat(std::string label, float value, float speed = 1.0f,
                                float min = 0.0f, float max = 0.0f,
                                WidgetCallback onChange = {});
    static WidgetNode dragInt(std::string label, int value, float speed = 1.0f,
                              int min = 0, int max = 0,
                              WidgetCallback onChange = {});

    // Phase 7 builders
    static WidgetNode listBox(std::string label, std::vector<std::string> items,
                              int selected = 0, int heightInItems = -1,
                              WidgetCallback onChange = {});
    static WidgetNode popup(std::string id, std::vector<WidgetNode> children = {});
    static WidgetNode modal(std::string title, std::vector<WidgetNode> children = {},
                            WidgetCallback onClose = {});

    // Phase 8 builders
    static WidgetNode canvas(std::string id, float width, float height,
                             WidgetCallback onDraw = {},
                             WidgetCallback onClick = {});
    static WidgetNode tooltip(std::string text);
    static WidgetNode tooltip(std::vector<WidgetNode> children);

    // Phase 5 builders
    static WidgetNode table(std::string id, int numColumns,
                            std::vector<std::string> headers = {},
                            std::vector<WidgetNode> children = {},
                            int flags = 0);
    static WidgetNode tableRow(std::vector<WidgetNode> children = {});
    static WidgetNode tableNextColumn();

    // Phase 9 builders
    static WidgetNode radioButton(std::string label, int activeValue, int myValue,
                                   WidgetCallback onChange = {});
    static WidgetNode selectable(std::string label, bool selected = false,
                                  WidgetCallback onClick = {});
    static WidgetNode inputTextMultiline(std::string label, std::string value,
                                          float width = 0.0f, float height = 0.0f,
                                          WidgetCallback onChange = {});
    static WidgetNode bulletText(std::string content);
    static WidgetNode separatorText(std::string label);
    static WidgetNode indent(float width = 0.0f);
    static WidgetNode unindent(float width = 0.0f);

    // Phase 10 - Style push/pop builders
    static WidgetNode pushStyleColor(int colIdx, float r, float g, float b, float a);
    static WidgetNode popStyleColor(int count = 1);
    static WidgetNode pushStyleVar(int varIdx, float val);
    static WidgetNode pushStyleVar(int varIdx, float x, float y);
    static WidgetNode popStyleVar(int count = 1);

    // Phase 11 - Layout helper builders
    static WidgetNode dummy(float width, float height);
    static WidgetNode newLine();

    // Phase 12 - Advanced Input (continued)
    static WidgetNode dragFloat3(std::string label, float x, float y, float z,
                                  float speed = 1.0f, float min = 0.0f, float max = 0.0f,
                                  WidgetCallback onChange = {});
    static WidgetNode inputTextWithHint(std::string label, std::string hint,
                                         std::string value = "",
                                         WidgetCallback onChange = {},
                                         WidgetCallback onSubmit = {});
    static WidgetNode sliderAngle(std::string label, float valueRadians = 0.0f,
                                   float minDegrees = -360.0f, float maxDegrees = 360.0f,
                                   WidgetCallback onChange = {});
    static WidgetNode smallButton(std::string label, WidgetCallback onClick = {});
    static WidgetNode colorButton(std::string label, float r = 1.0f, float g = 1.0f,
                                   float b = 1.0f, float a = 1.0f,
                                   WidgetCallback onClick = {});

    // Phase 13 - Menus & Popups (continued)
    static WidgetNode contextMenu(std::vector<WidgetNode> children = {});
    static WidgetNode mainMenuBar(std::vector<WidgetNode> children = {});
};

/// Returns a human-readable name for a widget type (for debug/placeholder text).
const char* widgetTypeName(WidgetNode::Type type);

} // namespace finegui
