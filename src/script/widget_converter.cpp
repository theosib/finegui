#include <finegui/widget_converter.hpp>
#include <finescript/map_data.h>
#include <stdexcept>

namespace finegui {

// -- ConverterSymbols ---------------------------------------------------------

void ConverterSymbols::intern(finescript::ScriptEngine& engine) {
    // Field keys
    type     = engine.intern("type");
    label    = engine.intern("label");
    title    = engine.intern("title");
    text     = engine.intern("text");
    value    = engine.intern("value");
    min      = engine.intern("min");
    max      = engine.intern("max");
    id       = engine.intern("id");
    children = engine.intern("children");
    items    = engine.intern("items");
    width    = engine.intern("width");
    height   = engine.intern("height");
    count    = engine.intern("count");
    visible  = engine.intern("visible");
    enabled  = engine.intern("enabled");
    selected = engine.intern("selected");

    // Phase 3 field keys
    color        = engine.intern("color");
    overlay      = engine.intern("overlay");
    size         = engine.intern("size");
    offset       = engine.intern("offset");
    default_open = engine.intern("default_open");
    format       = engine.intern("format");

    // Phase 4 field keys
    border      = engine.intern("border");
    auto_scroll = engine.intern("auto_scroll");
    shortcut    = engine.intern("shortcut");
    checked     = engine.intern("checked");
    leaf        = engine.intern("leaf");

    // Phase 5 field keys
    num_columns = engine.intern("num_columns");
    headers     = engine.intern("headers");
    flags       = engine.intern("flags");

    // Table flag value symbols
    sym_flag_row_bg        = engine.intern("row_bg");
    sym_flag_borders       = engine.intern("borders");
    sym_flag_borders_h     = engine.intern("borders_h");
    sym_flag_borders_v     = engine.intern("borders_v");
    sym_flag_borders_inner = engine.intern("borders_inner");
    sym_flag_borders_outer = engine.intern("borders_outer");
    sym_flag_resizable     = engine.intern("resizable");
    sym_flag_sortable      = engine.intern("sortable");
    sym_flag_scroll_x      = engine.intern("scroll_x");
    sym_flag_scroll_y      = engine.intern("scroll_y");

    // Callback keys
    on_click  = engine.intern("on_click");
    on_change = engine.intern("on_change");
    on_submit = engine.intern("on_submit");
    on_close  = engine.intern("on_close");
    on_select = engine.intern("on_select");

    // Type name symbols - Phase 1
    sym_window     = engine.intern("window");
    sym_text       = engine.intern("text");
    sym_button     = engine.intern("button");
    sym_checkbox   = engine.intern("checkbox");
    sym_slider     = engine.intern("slider");
    sym_slider_int = engine.intern("slider_int");
    sym_input_text = engine.intern("input_text");
    sym_input_int  = engine.intern("input_int");
    sym_input_float= engine.intern("input_float");
    sym_combo      = engine.intern("combo");
    sym_separator  = engine.intern("separator");
    sym_group      = engine.intern("group");
    sym_columns    = engine.intern("columns");
    sym_image      = engine.intern("image");

    // Type name symbols - Phase 3
    sym_same_line          = engine.intern("same_line");
    sym_spacing            = engine.intern("spacing");
    sym_text_colored       = engine.intern("text_colored");
    sym_text_wrapped       = engine.intern("text_wrapped");
    sym_text_disabled      = engine.intern("text_disabled");
    sym_progress_bar       = engine.intern("progress_bar");
    sym_collapsing_header  = engine.intern("collapsing_header");

    // Type name symbols - Phase 4
    sym_tab_bar   = engine.intern("tab_bar");
    sym_tab       = engine.intern("tab");
    sym_tree_node = engine.intern("tree_node");
    sym_child     = engine.intern("child");
    sym_menu_bar  = engine.intern("menu_bar");
    sym_menu      = engine.intern("menu");
    sym_menu_item = engine.intern("menu_item");

    // Type name symbols - Phase 5
    sym_table             = engine.intern("table");
    sym_table_row         = engine.intern("table_row");
    sym_table_next_column = engine.intern("table_next_column");

    // Phase 6 field keys
    speed = engine.intern("speed");

    // Type name symbols - Phase 6
    sym_color_edit   = engine.intern("color_edit");
    sym_color_picker = engine.intern("color_picker");
    sym_drag_float   = engine.intern("drag_float");
    sym_drag_int     = engine.intern("drag_int");
}

// -- Type mapping -------------------------------------------------------------

static WidgetNode::Type symbolToType(uint32_t sym, const ConverterSymbols& s) {
    if (sym == s.sym_window)      return WidgetNode::Type::Window;
    if (sym == s.sym_text)        return WidgetNode::Type::Text;
    if (sym == s.sym_button)      return WidgetNode::Type::Button;
    if (sym == s.sym_checkbox)    return WidgetNode::Type::Checkbox;
    if (sym == s.sym_slider)      return WidgetNode::Type::Slider;
    if (sym == s.sym_slider_int)  return WidgetNode::Type::SliderInt;
    if (sym == s.sym_input_text)  return WidgetNode::Type::InputText;
    if (sym == s.sym_input_int)   return WidgetNode::Type::InputInt;
    if (sym == s.sym_input_float) return WidgetNode::Type::InputFloat;
    if (sym == s.sym_combo)       return WidgetNode::Type::Combo;
    if (sym == s.sym_separator)   return WidgetNode::Type::Separator;
    if (sym == s.sym_group)       return WidgetNode::Type::Group;
    if (sym == s.sym_columns)     return WidgetNode::Type::Columns;
    if (sym == s.sym_image)       return WidgetNode::Type::Image;
    return WidgetNode::Type::Text; // fallback
}

// -- Conversion ---------------------------------------------------------------

WidgetNode convertToWidget(const finescript::Value& map,
                           finescript::ScriptEngine& engine,
                           finescript::ExecutionContext& ctx,
                           const ConverterSymbols& syms) {
    if (!map.isMap()) {
        throw std::runtime_error("convertToWidget: expected map value");
    }

    WidgetNode node;
    const auto& m = map.asMap();

    // Type (required)
    auto typeVal = m.get(syms.type);
    if (typeVal.isSymbol()) {
        node.type = symbolToType(typeVal.asSymbol(), syms);
    } else {
        node.type = WidgetNode::Type::Text;
    }

    // Label / title
    auto labelVal = m.get(syms.label);
    if (!labelVal.isNil() && labelVal.isString()) {
        node.label = std::string(labelVal.asString());
    }
    auto titleVal = m.get(syms.title);
    if (!titleVal.isNil() && titleVal.isString()) {
        node.label = std::string(titleVal.asString());
    }

    // Text content
    auto textVal = m.get(syms.text);
    if (!textVal.isNil() && textVal.isString()) {
        node.textContent = std::string(textVal.asString());
    }

    // ID
    auto idVal = m.get(syms.id);
    if (!idVal.isNil() && idVal.isString()) {
        node.id = std::string(idVal.asString());
    }

    // Value (auto-detect type)
    auto valVal = m.get(syms.value);
    if (!valVal.isNil()) {
        if (valVal.isBool()) {
            node.boolValue = valVal.asBool();
        } else if (valVal.isInt()) {
            node.intValue = static_cast<int>(valVal.asInt());
        } else if (valVal.isFloat() || valVal.isNumeric()) {
            node.floatValue = static_cast<float>(valVal.asNumber());
        } else if (valVal.isString()) {
            node.stringValue = std::string(valVal.asString());
        }
    }

    // Min / max
    auto minVal = m.get(syms.min);
    if (minVal.isNumeric()) {
        node.minFloat = static_cast<float>(minVal.asNumber());
        node.minInt = static_cast<int>(minVal.asNumber());
    }
    auto maxVal = m.get(syms.max);
    if (maxVal.isNumeric()) {
        node.maxFloat = static_cast<float>(maxVal.asNumber());
        node.maxInt = static_cast<int>(maxVal.asNumber());
    }

    // Selected index (for combo)
    auto selVal = m.get(syms.selected);
    if (selVal.isInt()) {
        node.selectedIndex = static_cast<int>(selVal.asInt());
    }

    // Width / height
    auto widthVal = m.get(syms.width);
    if (widthVal.isNumeric()) {
        node.width = static_cast<float>(widthVal.asNumber());
    }
    auto heightVal = m.get(syms.height);
    if (heightVal.isNumeric()) {
        node.height = static_cast<float>(heightVal.asNumber());
    }

    // Column count
    auto countVal = m.get(syms.count);
    if (countVal.isInt()) {
        node.columnCount = static_cast<int>(countVal.asInt());
    }

    // Visible / enabled
    auto visVal = m.get(syms.visible);
    if (visVal.isBool()) {
        node.visible = visVal.asBool();
    }
    auto enVal = m.get(syms.enabled);
    if (enVal.isBool()) {
        node.enabled = enVal.asBool();
    }

    // Items (for combo, listbox)
    auto itemsVal = m.get(syms.items);
    if (itemsVal.isArray()) {
        const auto& arr = itemsVal.asArray();
        node.items.reserve(arr.size());
        for (const auto& item : arr) {
            if (item.isString()) {
                node.items.push_back(std::string(item.asString()));
            } else {
                node.items.push_back(item.toString(&engine.interner()));
            }
        }
    }

    // Callbacks — wrap script closures as C++ WidgetCallback
    auto onClickVal = m.get(syms.on_click);
    if (onClickVal.isCallable()) {
        node.onClick = [&engine, &ctx, closure = onClickVal](WidgetNode&) {
            engine.callFunction(closure, {}, ctx);
        };
    }

    auto onChangeVal = m.get(syms.on_change);
    if (onChangeVal.isCallable()) {
        node.onChange = [&engine, &ctx, closure = onChangeVal](WidgetNode& w) {
            auto scriptVal = widgetValueToScriptValue(w);
            engine.callFunction(closure, {scriptVal}, ctx);
        };
    }

    auto onSubmitVal = m.get(syms.on_submit);
    if (onSubmitVal.isCallable()) {
        node.onSubmit = [&engine, &ctx, closure = onSubmitVal](WidgetNode& w) {
            auto scriptVal = widgetValueToScriptValue(w);
            engine.callFunction(closure, {scriptVal}, ctx);
        };
    }

    auto onCloseVal = m.get(syms.on_close);
    if (onCloseVal.isCallable()) {
        node.onClose = [&engine, &ctx, closure = onCloseVal](WidgetNode&) {
            engine.callFunction(closure, {}, ctx);
        };
    }

    // Children (recurse)
    auto childrenVal = m.get(syms.children);
    if (childrenVal.isArray()) {
        const auto& arr = childrenVal.asArray();
        node.children.reserve(arr.size());
        for (const auto& child : arr) {
            if (child.isMap()) {
                node.children.push_back(convertToWidget(child, engine, ctx, syms));
            }
        }
    }

    return node;
}

// -- Value extraction ---------------------------------------------------------

finescript::Value widgetValueToScriptValue(const WidgetNode& widget) {
    switch (widget.type) {
        case WidgetNode::Type::Checkbox:
            return finescript::Value::boolean(widget.boolValue);
        case WidgetNode::Type::Slider:
        case WidgetNode::Type::InputFloat:
        case WidgetNode::Type::DragFloat:
            return finescript::Value::number(widget.floatValue);
        case WidgetNode::Type::SliderInt:
        case WidgetNode::Type::InputInt:
        case WidgetNode::Type::DragInt:
            return finescript::Value::integer(widget.intValue);
        case WidgetNode::Type::InputText:
            return finescript::Value::string(widget.stringValue);
        case WidgetNode::Type::Combo:
        case WidgetNode::Type::ListBox:
            return finescript::Value::integer(widget.selectedIndex);
        default:
            return finescript::Value::nil();
    }
}

} // namespace finegui
