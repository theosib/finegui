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
        case WidgetNode::Type::TabBar:            return "TabBar";
        case WidgetNode::Type::TabItem:           return "TabItem";
        case WidgetNode::Type::TreeNode:          return "TreeNode";
        case WidgetNode::Type::CollapsingHeader:  return "CollapsingHeader";
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
        case WidgetNode::Type::ProgressBar:       return "ProgressBar";
        case WidgetNode::Type::Tooltip:           return "Tooltip";
        default:                                  return "Unknown";
    }
}

} // namespace finegui
