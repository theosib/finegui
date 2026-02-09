#include <finegui/widget_node.hpp>

namespace finegui {

WidgetNode WidgetNode::window(std::string title, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Window;
    n.label = std::move(title);
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::text(std::string content) {
    WidgetNode n;
    n.type = Type::Text;
    n.textContent = std::move(content);
    return n;
}

WidgetNode WidgetNode::button(std::string label, WidgetCallback onClick) {
    WidgetNode n;
    n.type = Type::Button;
    n.label = std::move(label);
    n.onClick = std::move(onClick);
    return n;
}

WidgetNode WidgetNode::checkbox(std::string label, bool value, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::Checkbox;
    n.label = std::move(label);
    n.boolValue = value;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::slider(std::string label, float value, float min, float max,
                              WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::Slider;
    n.label = std::move(label);
    n.floatValue = value;
    n.minFloat = min;
    n.maxFloat = max;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::sliderInt(std::string label, int value, int min, int max,
                                 WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::SliderInt;
    n.label = std::move(label);
    n.intValue = value;
    n.minInt = min;
    n.maxInt = max;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::inputText(std::string label, std::string value,
                                 WidgetCallback onChange, WidgetCallback onSubmit) {
    WidgetNode n;
    n.type = Type::InputText;
    n.label = std::move(label);
    n.stringValue = std::move(value);
    n.onChange = std::move(onChange);
    n.onSubmit = std::move(onSubmit);
    return n;
}

WidgetNode WidgetNode::inputInt(std::string label, int value, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::InputInt;
    n.label = std::move(label);
    n.intValue = value;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::inputFloat(std::string label, float value, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::InputFloat;
    n.label = std::move(label);
    n.floatValue = value;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::combo(std::string label, std::vector<std::string> items,
                             int selected, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::Combo;
    n.label = std::move(label);
    n.items = std::move(items);
    n.selectedIndex = selected;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::separator() {
    WidgetNode n;
    n.type = Type::Separator;
    return n;
}

WidgetNode WidgetNode::group(std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Group;
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::columns(int count, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Columns;
    n.columnCount = count;
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::image(TextureHandle texture, float width, float height) {
    WidgetNode n;
    n.type = Type::Image;
    n.texture = texture;
    n.imageWidth = width;
    n.imageHeight = height;
    return n;
}

// Phase 3 builders

WidgetNode WidgetNode::sameLine(float offset) {
    WidgetNode n;
    n.type = Type::SameLine;
    n.offsetX = offset;
    return n;
}

WidgetNode WidgetNode::spacing() {
    WidgetNode n;
    n.type = Type::Spacing;
    return n;
}

WidgetNode WidgetNode::textColored(float r, float g, float b, float a, std::string content) {
    WidgetNode n;
    n.type = Type::TextColored;
    n.textContent = std::move(content);
    n.colorR = r;
    n.colorG = g;
    n.colorB = b;
    n.colorA = a;
    return n;
}

WidgetNode WidgetNode::textWrapped(std::string content) {
    WidgetNode n;
    n.type = Type::TextWrapped;
    n.textContent = std::move(content);
    return n;
}

WidgetNode WidgetNode::textDisabled(std::string content) {
    WidgetNode n;
    n.type = Type::TextDisabled;
    n.textContent = std::move(content);
    return n;
}

WidgetNode WidgetNode::progressBar(float fraction, float width, float height,
                                    std::string overlay) {
    WidgetNode n;
    n.type = Type::ProgressBar;
    n.floatValue = fraction;
    n.width = width;
    n.height = height;
    n.overlayText = std::move(overlay);
    return n;
}

WidgetNode WidgetNode::collapsingHeader(std::string label, std::vector<WidgetNode> children,
                                         bool defaultOpen) {
    WidgetNode n;
    n.type = Type::CollapsingHeader;
    n.label = std::move(label);
    n.children = std::move(children);
    n.defaultOpen = defaultOpen;
    return n;
}

// Phase 4 builders

WidgetNode WidgetNode::tabBar(std::string id, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::TabBar;
    n.id = std::move(id);
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::tabItem(std::string label, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::TabItem;
    n.label = std::move(label);
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::treeNode(std::string label, std::vector<WidgetNode> children,
                                 bool defaultOpen, bool isLeaf) {
    WidgetNode n;
    n.type = Type::TreeNode;
    n.label = std::move(label);
    n.children = std::move(children);
    n.defaultOpen = defaultOpen;
    n.leaf = isLeaf;
    return n;
}

WidgetNode WidgetNode::child(std::string id, float width, float height,
                              bool border, bool autoScroll,
                              std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Child;
    n.id = std::move(id);
    n.width = width;
    n.height = height;
    n.border = border;
    n.autoScroll = autoScroll;
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::menuBar(std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::MenuBar;
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::menu(std::string label, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Menu;
    n.label = std::move(label);
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::menuItem(std::string label, WidgetCallback onClick,
                                 std::string shortcut, bool checked) {
    WidgetNode n;
    n.type = Type::MenuItem;
    n.label = std::move(label);
    n.onClick = std::move(onClick);
    n.shortcutText = std::move(shortcut);
    n.checked = checked;
    return n;
}

// Phase 6 builders

WidgetNode WidgetNode::colorEdit(std::string label, float r, float g, float b, float a,
                                  WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::ColorEdit;
    n.label = std::move(label);
    n.colorR = r; n.colorG = g; n.colorB = b; n.colorA = a;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::colorPicker(std::string label, float r, float g, float b, float a,
                                    WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::ColorPicker;
    n.label = std::move(label);
    n.colorR = r; n.colorG = g; n.colorB = b; n.colorA = a;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::dragFloat(std::string label, float value, float speed,
                                  float min, float max, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::DragFloat;
    n.label = std::move(label);
    n.floatValue = value;
    n.dragSpeed = speed;
    n.minFloat = min;
    n.maxFloat = max;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::dragInt(std::string label, int value, float speed,
                                int min, int max, WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::DragInt;
    n.label = std::move(label);
    n.intValue = value;
    n.dragSpeed = speed;
    n.minInt = min;
    n.maxInt = max;
    n.onChange = std::move(onChange);
    return n;
}

// Phase 7 builders

WidgetNode WidgetNode::listBox(std::string label, std::vector<std::string> items,
                                int selected, int heightInItems,
                                WidgetCallback onChange) {
    WidgetNode n;
    n.type = Type::ListBox;
    n.label = std::move(label);
    n.items = std::move(items);
    n.selectedIndex = selected;
    n.heightInItems = heightInItems;
    n.onChange = std::move(onChange);
    return n;
}

WidgetNode WidgetNode::popup(std::string id, std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Popup;
    n.id = std::move(id);
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::modal(std::string title, std::vector<WidgetNode> children,
                              WidgetCallback onClose) {
    WidgetNode n;
    n.type = Type::Modal;
    n.label = std::move(title);
    n.children = std::move(children);
    n.onClose = std::move(onClose);
    return n;
}

// Phase 8 builders

WidgetNode WidgetNode::canvas(std::string id, float width, float height,
                               WidgetCallback onDraw, WidgetCallback onClick) {
    WidgetNode n;
    n.type = Type::Canvas;
    n.id = std::move(id);
    n.width = width;
    n.height = height;
    n.onDraw = std::move(onDraw);
    n.onClick = std::move(onClick);
    return n;
}

WidgetNode WidgetNode::tooltip(std::string text) {
    WidgetNode n;
    n.type = Type::Tooltip;
    n.textContent = std::move(text);
    return n;
}

WidgetNode WidgetNode::tooltip(std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::Tooltip;
    n.children = std::move(children);
    return n;
}

// Phase 5 builders

WidgetNode WidgetNode::table(std::string id, int numColumns,
                              std::vector<std::string> headers,
                              std::vector<WidgetNode> children,
                              int flags) {
    WidgetNode n;
    n.type = Type::Table;
    n.id = std::move(id);
    n.columnCount = numColumns;
    n.items = std::move(headers);  // items stores header labels for Table
    n.children = std::move(children);
    n.tableFlags = flags;
    return n;
}

WidgetNode WidgetNode::tableRow(std::vector<WidgetNode> children) {
    WidgetNode n;
    n.type = Type::TableRow;
    n.children = std::move(children);
    return n;
}

WidgetNode WidgetNode::tableNextColumn() {
    WidgetNode n;
    n.type = Type::TableColumn;
    return n;
}

const char* widgetTypeName(WidgetNode::Type type) {
    switch (type) {
        case WidgetNode::Type::Window:            return "Window";
        case WidgetNode::Type::Text:              return "Text";
        case WidgetNode::Type::Button:            return "Button";
        case WidgetNode::Type::Checkbox:          return "Checkbox";
        case WidgetNode::Type::Slider:            return "Slider";
        case WidgetNode::Type::SliderInt:         return "SliderInt";
        case WidgetNode::Type::InputText:         return "InputText";
        case WidgetNode::Type::InputInt:          return "InputInt";
        case WidgetNode::Type::InputFloat:        return "InputFloat";
        case WidgetNode::Type::Combo:             return "Combo";
        case WidgetNode::Type::Separator:         return "Separator";
        case WidgetNode::Type::Group:             return "Group";
        case WidgetNode::Type::Columns:           return "Columns";
        case WidgetNode::Type::Image:             return "Image";
        case WidgetNode::Type::SameLine:          return "SameLine";
        case WidgetNode::Type::Spacing:           return "Spacing";
        case WidgetNode::Type::TextColored:       return "TextColored";
        case WidgetNode::Type::TextWrapped:       return "TextWrapped";
        case WidgetNode::Type::TextDisabled:      return "TextDisabled";
        case WidgetNode::Type::ProgressBar:       return "ProgressBar";
        case WidgetNode::Type::CollapsingHeader:  return "CollapsingHeader";
        case WidgetNode::Type::TabBar:            return "TabBar";
        case WidgetNode::Type::TabItem:           return "TabItem";
        case WidgetNode::Type::TreeNode:          return "TreeNode";
        case WidgetNode::Type::Child:             return "Child";
        case WidgetNode::Type::MenuBar:           return "MenuBar";
        case WidgetNode::Type::Menu:              return "Menu";
        case WidgetNode::Type::MenuItem:          return "MenuItem";
        case WidgetNode::Type::Table:             return "Table";
        case WidgetNode::Type::TableColumn:       return "TableColumn";
        case WidgetNode::Type::TableRow:          return "TableRow";
        case WidgetNode::Type::ColorEdit:         return "ColorEdit";
        case WidgetNode::Type::ColorPicker:       return "ColorPicker";
        case WidgetNode::Type::DragFloat:         return "DragFloat";
        case WidgetNode::Type::DragInt:           return "DragInt";
        case WidgetNode::Type::ListBox:           return "ListBox";
        case WidgetNode::Type::Popup:             return "Popup";
        case WidgetNode::Type::Modal:             return "Modal";
        case WidgetNode::Type::Canvas:            return "Canvas";
        case WidgetNode::Type::Tooltip:           return "Tooltip";
        default:                                  return "Unknown";
    }
}

} // namespace finegui
