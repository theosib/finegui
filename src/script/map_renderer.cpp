#include <finegui/map_renderer.hpp>
#include <finescript/map_data.h>
#include <imgui.h>
#include <cstring>

namespace finegui {

using finescript::Value;
using finescript::MapData;
using finescript::ExecutionContext;

// -- InputText resize callback ------------------------------------------------

struct InputTextCallbackData {
    std::string* str;
};

static int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* userData = static_cast<InputTextCallbackData*>(data->UserData);
        userData->str->resize(static_cast<size_t>(data->BufTextLen));
        data->Buf = userData->str->data();
    }
    return 0;
}

// -- MapRenderer --------------------------------------------------------------

MapRenderer::MapRenderer(finescript::ScriptEngine& engine)
    : engine_(engine) {
    syms_.intern(engine);
}

int MapRenderer::show(Value rootMap, ExecutionContext& ctx) {
    int id = nextId_++;
    trees_[id] = Entry{std::move(rootMap), &ctx};
    return id;
}

void MapRenderer::hide(int id) {
    trees_.erase(id);
}

void MapRenderer::hideAll() {
    trees_.clear();
}

Value* MapRenderer::get(int id) {
    auto it = trees_.find(id);
    if (it != trees_.end()) {
        return &it->second.rootMap;
    }
    return nullptr;
}

void MapRenderer::renderAll() {
    for (auto& [id, entry] : trees_) {
        if (entry.rootMap.isMap()) {
            renderNode(entry.rootMap.asMap(), *entry.ctx);
        }
    }
}

// -- Helpers ------------------------------------------------------------------

std::string MapRenderer::getStringField(MapData& m, uint32_t key, const char* def) {
    auto val = m.get(key);
    if (val.isString()) return std::string(val.asString());
    return def;
}

double MapRenderer::getNumericField(MapData& m, uint32_t key, double def) {
    auto val = m.get(key);
    if (val.isNumeric()) return val.asNumber();
    return def;
}

bool MapRenderer::getBoolField(MapData& m, uint32_t key, bool def) {
    auto val = m.get(key);
    if (val.isBool()) return val.asBool();
    return def;
}

void MapRenderer::invokeCallback(MapData& m, uint32_t key,
                                  ExecutionContext& ctx,
                                  std::vector<Value> args) {
    auto handler = m.get(key);
    if (handler.isCallable()) {
        engine_.callFunction(handler, std::move(args), ctx);
    }
}

// -- Dispatch -----------------------------------------------------------------

void MapRenderer::renderNode(MapData& m, ExecutionContext& ctx) {
    // Check visibility
    auto visVal = m.get(syms_.visible);
    if (visVal.isBool() && !visVal.asBool()) return;

    // Disabled state
    auto enVal = m.get(syms_.enabled);
    bool wasDisabled = false;
    if (enVal.isBool() && !enVal.asBool()) {
        ImGui::BeginDisabled(true);
        wasDisabled = true;
    }

    // Push ID if present
    auto idVal = m.get(syms_.id);
    bool pushId = idVal.isString() && !idVal.asString().empty();
    if (pushId) {
        ImGui::PushID(idVal.asString().c_str());
    }

    // Dispatch by type symbol
    auto typeVal = m.get(syms_.type);
    if (typeVal.isSymbol()) {
        uint32_t sym = typeVal.asSymbol();
        if (sym == syms_.sym_window)          renderWindow(m, ctx);
        else if (sym == syms_.sym_text)       renderText(m);
        else if (sym == syms_.sym_button)     renderButton(m, ctx);
        else if (sym == syms_.sym_checkbox)   renderCheckbox(m, ctx);
        else if (sym == syms_.sym_slider)     renderSlider(m, ctx);
        else if (sym == syms_.sym_slider_int) renderSliderInt(m, ctx);
        else if (sym == syms_.sym_input_text) renderInputText(m, ctx);
        else if (sym == syms_.sym_input_int)  renderInputInt(m, ctx);
        else if (sym == syms_.sym_input_float)renderInputFloat(m, ctx);
        else if (sym == syms_.sym_combo)      renderCombo(m, ctx);
        else if (sym == syms_.sym_separator)  renderSeparator();
        else if (sym == syms_.sym_group)      renderGroup(m, ctx);
        else if (sym == syms_.sym_columns)    renderColumns(m, ctx);
        // Phase 3
        else if (sym == syms_.sym_same_line)         renderSameLine(m);
        else if (sym == syms_.sym_spacing)            renderSpacing();
        else if (sym == syms_.sym_text_colored)       renderTextColored(m);
        else if (sym == syms_.sym_text_wrapped)       renderTextWrapped(m);
        else if (sym == syms_.sym_text_disabled)      renderTextDisabled(m);
        else if (sym == syms_.sym_progress_bar)       renderProgressBar(m);
        else if (sym == syms_.sym_collapsing_header)  renderCollapsingHeader(m, ctx);
        // Phase 4
        else if (sym == syms_.sym_tab_bar)    renderTabBar(m, ctx);
        else if (sym == syms_.sym_tab)        renderTab(m, ctx);
        else if (sym == syms_.sym_tree_node)  renderTreeNode(m, ctx);
        else if (sym == syms_.sym_child)      renderChild(m, ctx);
        else if (sym == syms_.sym_menu_bar)   renderMenuBar(m, ctx);
        else if (sym == syms_.sym_menu)       renderMenu(m, ctx);
        else if (sym == syms_.sym_menu_item)  renderMenuItem(m, ctx);
        // Phase 5
        else if (sym == syms_.sym_table)              renderTable(m, ctx);
        else if (sym == syms_.sym_table_row)          renderTableRow(m, ctx);
        else if (sym == syms_.sym_table_next_column)  renderTableNextColumn();
        // Phase 6
        else if (sym == syms_.sym_color_edit)    renderColorEdit(m, ctx);
        else if (sym == syms_.sym_color_picker)  renderColorPicker(m, ctx);
        else if (sym == syms_.sym_drag_float)    renderDragFloat(m, ctx);
        else if (sym == syms_.sym_drag_int)      renderDragInt(m, ctx);
        // Phase 7
        else if (sym == syms_.sym_listbox)   renderListBox(m, ctx);
        else if (sym == syms_.sym_popup)     renderPopup(m, ctx);
        else if (sym == syms_.sym_modal)     renderModal(m, ctx);
        // Phase 8
        else if (sym == syms_.sym_canvas)    renderCanvas(m, ctx);
        else if (sym == syms_.sym_tooltip)   renderTooltip(m, ctx);
        else {
            ImGui::TextColored({1, 0, 0, 1}, "[Unknown widget type]");
        }
    }

    if (pushId) ImGui::PopID();
    if (wasDisabled) ImGui::EndDisabled();
}

// -- Per-widget render methods ------------------------------------------------

void MapRenderer::renderWindow(MapData& m, ExecutionContext& ctx) {
    // Window uses :title for the window label
    auto title = getStringField(m, syms_.title, "Untitled");

    bool open = true;
    if (ImGui::Begin(title.c_str(), &open)) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
    }
    ImGui::End();

    if (!open) {
        m.set(syms_.visible, Value::boolean(false));
        invokeCallback(m, syms_.on_close, ctx);
    }
}

void MapRenderer::renderText(MapData& m) {
    auto text = getStringField(m, syms_.text);
    ImGui::TextUnformatted(text.c_str());
}

void MapRenderer::renderButton(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Button");
    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));

    bool clicked;
    if (w > 0 || h > 0) {
        clicked = ImGui::Button(label.c_str(), {w, h});
    } else {
        clicked = ImGui::Button(label.c_str());
    }

    if (clicked) {
        invokeCallback(m, syms_.on_click, ctx);
    }
}

void MapRenderer::renderCheckbox(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Checkbox");
    bool value = getBoolField(m, syms_.value, false);

    if (ImGui::Checkbox(label.c_str(), &value)) {
        m.set(syms_.value, Value::boolean(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::boolean(value)});
    }
}

void MapRenderer::renderSlider(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Slider");
    float value = static_cast<float>(getNumericField(m, syms_.value, 0.0));
    float min = static_cast<float>(getNumericField(m, syms_.min, 0.0));
    float max = static_cast<float>(getNumericField(m, syms_.max, 1.0));

    if (ImGui::SliderFloat(label.c_str(), &value, min, max)) {
        m.set(syms_.value, Value::number(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::number(value)});
    }
}

void MapRenderer::renderSliderInt(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Slider");
    int value = static_cast<int>(getNumericField(m, syms_.value, 0));
    int min = static_cast<int>(getNumericField(m, syms_.min, 0));
    int max = static_cast<int>(getNumericField(m, syms_.max, 100));

    if (ImGui::SliderInt(label.c_str(), &value, min, max)) {
        m.set(syms_.value, Value::integer(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::integer(value)});
    }
}

void MapRenderer::renderInputText(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Input");
    auto valEntry = m.get(syms_.value);
    if (!valEntry.isString()) {
        // Initialize with empty string if not present
        m.set(syms_.value, Value::string(""));
        valEntry = m.get(syms_.value);
    }

    // Mutable string: shared_ptr means modifying this string also modifies
    // the one stored in the map — no explicit writeback needed.
    auto& str = valEntry.asStringMut();
    if (str.capacity() < 256) str.reserve(256);
    size_t cap = str.capacity();
    str.resize(cap);

    InputTextCallbackData cbData{&str};
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    auto onSubmit = m.get(syms_.on_submit);
    if (onSubmit.isCallable()) {
        flags |= ImGuiInputTextFlags_EnterReturnsTrue;
    }

    bool enterPressed = ImGui::InputText(
        label.c_str(), str.data(), str.size() + 1,
        flags, inputTextResizeCallback, &cbData);

    // Trim to actual string length
    str.resize(std::strlen(str.c_str()));

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        invokeCallback(m, syms_.on_change, ctx, {Value::string(str)});
    }

    if (enterPressed && onSubmit.isCallable()) {
        invokeCallback(m, syms_.on_submit, ctx, {Value::string(str)});
    }
}

void MapRenderer::renderInputInt(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Input");
    int value = static_cast<int>(getNumericField(m, syms_.value, 0));

    if (ImGui::InputInt(label.c_str(), &value)) {
        m.set(syms_.value, Value::integer(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::integer(value)});
    }
}

void MapRenderer::renderInputFloat(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Input");
    float value = static_cast<float>(getNumericField(m, syms_.value, 0.0));

    if (ImGui::InputFloat(label.c_str(), &value)) {
        m.set(syms_.value, Value::number(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::number(value)});
    }
}

void MapRenderer::renderCombo(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Combo");
    int selected = static_cast<int>(getNumericField(m, syms_.selected, 0));

    // Build items list from array
    auto itemsVal = m.get(syms_.items);
    if (!itemsVal.isArray()) return;

    const auto& items = itemsVal.asArray();
    const char* preview = (selected >= 0 && selected < static_cast<int>(items.size())
                           && items[static_cast<size_t>(selected)].isString())
                          ? items[static_cast<size_t>(selected)].asString().c_str()
                          : "";

    if (ImGui::BeginCombo(label.c_str(), preview)) {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (!items[static_cast<size_t>(i)].isString()) continue;
            bool isSelected = (i == selected);
            if (ImGui::Selectable(items[static_cast<size_t>(i)].asString().c_str(), isSelected)) {
                selected = i;
                m.set(syms_.selected, Value::integer(selected));
                invokeCallback(m, syms_.on_change, ctx, {Value::integer(selected)});
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void MapRenderer::renderSeparator() {
    ImGui::Separator();
}

void MapRenderer::renderGroup(MapData& m, ExecutionContext& ctx) {
    auto childrenVal = m.get(syms_.children);
    if (childrenVal.isArray()) {
        for (auto& child : childrenVal.asArrayMut()) {
            if (child.isMap()) {
                renderNode(child.asMap(), ctx);
            }
        }
    }
}

void MapRenderer::renderColumns(MapData& m, ExecutionContext& ctx) {
    int colCount = static_cast<int>(getNumericField(m, syms_.count, 1));

    auto childrenVal = m.get(syms_.children);
    if (!childrenVal.isArray()) return;

    auto& children = childrenVal.asArrayMut();

    if (colCount <= 1) {
        for (auto& child : children) {
            if (child.isMap()) {
                renderNode(child.asMap(), ctx);
            }
        }
        return;
    }

    ImGui::Columns(colCount, nullptr, false);
    for (size_t i = 0; i < children.size(); i++) {
        if (children[i].isMap()) {
            renderNode(children[i].asMap(), ctx);
        }
        if (i + 1 < children.size()) {
            ImGui::NextColumn();
        }
    }
    ImGui::Columns(1);
}

// -- Phase 3: Layout & Display ------------------------------------------------

void MapRenderer::renderSameLine(MapData& m) {
    float offset = static_cast<float>(getNumericField(m, syms_.offset, 0.0));
    if (offset > 0) {
        ImGui::SameLine(offset);
    } else {
        ImGui::SameLine();
    }
}

void MapRenderer::renderSpacing() {
    ImGui::Spacing();
}

void MapRenderer::renderTextColored(MapData& m) {
    auto text = getStringField(m, syms_.text);

    // Read color from :color field — expects array [r, g, b, a]
    ImVec4 col{1.0f, 1.0f, 1.0f, 1.0f};
    auto colorVal = m.get(syms_.color);
    if (colorVal.isArray()) {
        const auto& arr = colorVal.asArray();
        if (arr.size() >= 3) {
            col.x = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
            col.y = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
            col.z = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
            if (arr.size() >= 4) {
                col.w = static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0);
            }
        }
    }

    ImGui::TextColored(col, "%s", text.c_str());
}

void MapRenderer::renderTextWrapped(MapData& m) {
    auto text = getStringField(m, syms_.text);
    ImGui::TextWrapped("%s", text.c_str());
}

void MapRenderer::renderTextDisabled(MapData& m) {
    auto text = getStringField(m, syms_.text);
    ImGui::TextDisabled("%s", text.c_str());
}

void MapRenderer::renderProgressBar(MapData& m) {
    float fraction = static_cast<float>(getNumericField(m, syms_.value, 0.0));

    // Size from :size field [w, h] or :width/:height
    float w = -FLT_MIN;  // ImGui default = full width
    float h = 0.0f;      // ImGui default
    auto sizeVal = m.get(syms_.size);
    if (sizeVal.isArray() && sizeVal.asArray().size() >= 2) {
        const auto& arr = sizeVal.asArray();
        w = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : -FLT_MIN);
        h = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 0.0);
    } else {
        double wd = getNumericField(m, syms_.width, 0.0);
        double hd = getNumericField(m, syms_.height, 0.0);
        if (wd > 0) w = static_cast<float>(wd);
        if (hd > 0) h = static_cast<float>(hd);
    }

    // Overlay text
    auto overlayStr = getStringField(m, syms_.overlay, "");
    const char* overlay = overlayStr.empty() ? nullptr : overlayStr.c_str();

    ImGui::ProgressBar(fraction, {w, h}, overlay);
}

void MapRenderer::renderCollapsingHeader(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Header");
    bool defaultOpen = getBoolField(m, syms_.default_open, false);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::CollapsingHeader(label.c_str(), flags)) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
    }
}

// -- Phase 4: Containers & Menus ----------------------------------------------

void MapRenderer::renderTabBar(MapData& m, ExecutionContext& ctx) {
    auto id = getStringField(m, syms_.id, "##tabbar");
    if (ImGui::BeginTabBar(id.c_str())) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndTabBar();
    }
}

void MapRenderer::renderTab(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Tab");
    if (ImGui::BeginTabItem(label.c_str())) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndTabItem();
    }
}

void MapRenderer::renderTreeNode(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Node");
    bool isLeaf = getBoolField(m, syms_.leaf, false);
    bool defaultOpen = getBoolField(m, syms_.default_open, false);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (isLeaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked()) {
        invokeCallback(m, syms_.on_select, ctx);
        invokeCallback(m, syms_.on_click, ctx);
    }

    if (open && !isLeaf) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::TreePop();
    }
}

void MapRenderer::renderChild(MapData& m, ExecutionContext& ctx) {
    auto id = getStringField(m, syms_.id, "##child");
    bool hasBorder = getBoolField(m, syms_.border, false);
    bool autoScroll = getBoolField(m, syms_.auto_scroll, false);

    // Size from :size field [w, h] or :width/:height
    float w = 0.0f, h = 0.0f;
    auto sizeVal = m.get(syms_.size);
    if (sizeVal.isArray() && sizeVal.asArray().size() >= 2) {
        const auto& arr = sizeVal.asArray();
        w = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 0.0);
        h = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 0.0);
    } else {
        w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
        h = static_cast<float>(getNumericField(m, syms_.height, 0.0));
    }

    ImGuiChildFlags childFlags = ImGuiChildFlags_None;
    if (hasBorder) childFlags |= ImGuiChildFlags_Borders;

    if (ImGui::BeginChild(id.c_str(), {w, h}, childFlags)) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

void MapRenderer::renderMenuBar(MapData& m, ExecutionContext& ctx) {
    if (ImGui::BeginMenuBar()) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndMenuBar();
    }
}

void MapRenderer::renderMenu(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Menu");
    if (ImGui::BeginMenu(label.c_str())) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void MapRenderer::renderMenuItem(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Item");
    auto shortcut = getStringField(m, syms_.shortcut, "");
    const char* sc = shortcut.empty() ? nullptr : shortcut.c_str();

    auto checkedVal = m.get(syms_.checked);
    if (checkedVal.isBool()) {
        // Toggleable menu item
        bool ch = checkedVal.asBool();
        if (ImGui::MenuItem(label.c_str(), sc, &ch)) {
            m.set(syms_.checked, Value::boolean(ch));
            invokeCallback(m, syms_.on_click, ctx);
        }
    } else {
        // Simple menu item
        if (ImGui::MenuItem(label.c_str(), sc)) {
            invokeCallback(m, syms_.on_click, ctx);
        }
    }
}

// -- Phase 5: Tables ----------------------------------------------------------

int MapRenderer::parseTableFlags(MapData& m) {
    int result = 0;
    auto flagsVal = m.get(syms_.flags);
    if (flagsVal.isArray()) {
        for (const auto& f : flagsVal.asArray()) {
            if (!f.isSymbol()) continue;
            uint32_t s = f.asSymbol();
            if (s == syms_.sym_flag_row_bg)             result |= ImGuiTableFlags_RowBg;
            else if (s == syms_.sym_flag_borders)       result |= ImGuiTableFlags_Borders;
            else if (s == syms_.sym_flag_borders_h)     result |= ImGuiTableFlags_BordersH;
            else if (s == syms_.sym_flag_borders_v)     result |= ImGuiTableFlags_BordersV;
            else if (s == syms_.sym_flag_borders_inner) result |= ImGuiTableFlags_BordersInner;
            else if (s == syms_.sym_flag_borders_outer) result |= ImGuiTableFlags_BordersOuter;
            else if (s == syms_.sym_flag_resizable)     result |= ImGuiTableFlags_Resizable;
            else if (s == syms_.sym_flag_sortable)      result |= ImGuiTableFlags_Sortable;
            else if (s == syms_.sym_flag_scroll_x)      result |= ImGuiTableFlags_ScrollX;
            else if (s == syms_.sym_flag_scroll_y)      result |= ImGuiTableFlags_ScrollY;
        }
    } else if (flagsVal.isInt()) {
        result = static_cast<int>(flagsVal.asInt());
    }
    return result;
}

void MapRenderer::renderTable(MapData& m, ExecutionContext& ctx) {
    auto id = getStringField(m, syms_.id, "##table");
    int numCols = static_cast<int>(getNumericField(m, syms_.num_columns, 1));
    if (numCols < 1) numCols = 1;
    int flags = parseTableFlags(m);

    if (ImGui::BeginTable(id.c_str(), numCols, static_cast<ImGuiTableFlags>(flags))) {
        // Headers from :headers array of strings
        auto headersVal = m.get(syms_.headers);
        if (headersVal.isArray()) {
            for (const auto& h : headersVal.asArray()) {
                if (h.isString()) {
                    ImGui::TableSetupColumn(h.asString().c_str());
                } else {
                    ImGui::TableSetupColumn("");
                }
            }
            ImGui::TableHeadersRow();
        }

        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndTable();
    }
}

void MapRenderer::renderTableRow(MapData& m, ExecutionContext& ctx) {
    ImGui::TableNextRow();
    auto childrenVal = m.get(syms_.children);
    if (childrenVal.isArray()) {
        // Container mode: each child goes into the next column
        for (auto& child : childrenVal.asArrayMut()) {
            if (child.isMap()) {
                ImGui::TableNextColumn();
                renderNode(child.asMap(), ctx);
            }
        }
    }
}

void MapRenderer::renderTableNextColumn() {
    ImGui::TableNextColumn();
}

// -- Phase 6: Advanced Input --------------------------------------------------

void MapRenderer::renderColorEdit(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Color");

    // Read color from :color array [r, g, b, a] or :value array
    float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    auto colorVal = m.get(syms_.color);
    if (!colorVal.isArray()) colorVal = m.get(syms_.value);
    if (colorVal.isArray()) {
        const auto& arr = colorVal.asArray();
        if (arr.size() >= 3) {
            col[0] = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
            col[1] = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
            col[2] = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
            if (arr.size() >= 4)
                col[3] = static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0);
        }
    }

    if (ImGui::ColorEdit4(label.c_str(), col)) {
        auto newColor = Value::array({
            Value::number(col[0]), Value::number(col[1]),
            Value::number(col[2]), Value::number(col[3])
        });
        m.set(syms_.color, newColor);
        invokeCallback(m, syms_.on_change, ctx, {newColor});
    }
}

void MapRenderer::renderColorPicker(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Color");

    float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    auto colorVal = m.get(syms_.color);
    if (!colorVal.isArray()) colorVal = m.get(syms_.value);
    if (colorVal.isArray()) {
        const auto& arr = colorVal.asArray();
        if (arr.size() >= 3) {
            col[0] = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
            col[1] = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
            col[2] = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
            if (arr.size() >= 4)
                col[3] = static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0);
        }
    }

    if (ImGui::ColorPicker4(label.c_str(), col)) {
        auto newColor = Value::array({
            Value::number(col[0]), Value::number(col[1]),
            Value::number(col[2]), Value::number(col[3])
        });
        m.set(syms_.color, newColor);
        invokeCallback(m, syms_.on_change, ctx, {newColor});
    }
}

void MapRenderer::renderDragFloat(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Drag");
    float value = static_cast<float>(getNumericField(m, syms_.value, 0.0));
    float speed = static_cast<float>(getNumericField(m, syms_.speed, 1.0));
    float min = static_cast<float>(getNumericField(m, syms_.min, 0.0));
    float max = static_cast<float>(getNumericField(m, syms_.max, 0.0));

    if (ImGui::DragFloat(label.c_str(), &value, speed, min, max)) {
        m.set(syms_.value, Value::number(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::number(value)});
    }
}

void MapRenderer::renderDragInt(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Drag");
    int value = static_cast<int>(getNumericField(m, syms_.value, 0));
    float speed = static_cast<float>(getNumericField(m, syms_.speed, 1.0));
    int min = static_cast<int>(getNumericField(m, syms_.min, 0));
    int max = static_cast<int>(getNumericField(m, syms_.max, 0));

    if (ImGui::DragInt(label.c_str(), &value, speed, min, max)) {
        m.set(syms_.value, Value::integer(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::integer(value)});
    }
}

// -- Phase 7: Misc ------------------------------------------------------------

void MapRenderer::renderListBox(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "ListBox");
    int selected = static_cast<int>(getNumericField(m, syms_.selected, 0));

    auto itemsVal = m.get(syms_.items);
    if (!itemsVal.isArray()) return;

    const auto& items = itemsVal.asArray();

    // Calculate height from height_in_items (-1 = auto)
    int heightItems = static_cast<int>(getNumericField(m, syms_.height_in_items, -1));
    float heightPx = 0.0f;
    if (heightItems > 0) {
        heightPx = ImGui::GetTextLineHeightWithSpacing() * heightItems
                   + ImGui::GetStyle().FramePadding.y * 2.0f;
    }

    if (ImGui::BeginListBox(label.c_str(), {0.0f, heightPx})) {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (!items[static_cast<size_t>(i)].isString()) continue;
            bool isSelected = (i == selected);
            if (ImGui::Selectable(items[static_cast<size_t>(i)].asString().c_str(), isSelected)) {
                selected = i;
                m.set(syms_.selected, Value::integer(selected));
                invokeCallback(m, syms_.on_change, ctx, {Value::integer(selected)});
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}

void MapRenderer::renderPopup(MapData& m, ExecutionContext& ctx) {
    auto id = getStringField(m, syms_.id, "##popup");

    // :value = true means "request open this frame"
    auto openVal = m.get(syms_.value);
    if (openVal.isBool() && openVal.asBool()) {
        ImGui::OpenPopup(id.c_str());
        m.set(syms_.value, Value::boolean(false));
    }

    if (ImGui::BeginPopup(id.c_str())) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndPopup();
    }
}

void MapRenderer::renderModal(MapData& m, ExecutionContext& ctx) {
    auto title = getStringField(m, syms_.title, "Modal");
    if (title.empty()) title = getStringField(m, syms_.label, "Modal");

    // :value = true means "request open this frame"
    auto openVal = m.get(syms_.value);
    if (openVal.isBool() && openVal.asBool()) {
        ImGui::OpenPopup(title.c_str());
        m.set(syms_.value, Value::boolean(false));
    }

    bool open = true;
    if (ImGui::BeginPopupModal(title.c_str(), &open)) {
        auto childrenVal = m.get(syms_.children);
        if (childrenVal.isArray()) {
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
        }
        ImGui::EndPopup();
    }

    if (!open) {
        invokeCallback(m, syms_.on_close, ctx);
    }
}

// -- Phase 8: Custom ----------------------------------------------------------

// Helper: read a 2-element array into two floats
static bool readVec2(const Value& val, float& x, float& y) {
    if (!val.isArray()) return false;
    const auto& arr = val.asArray();
    if (arr.size() < 2) return false;
    x = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 0.0);
    y = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 0.0);
    return true;
}

// Helper: read a color array into ImU32
static ImU32 readColorU32(const Value& val, ImU32 def = IM_COL32_WHITE) {
    if (!val.isArray()) return def;
    const auto& arr = val.asArray();
    if (arr.size() < 3) return def;
    float r = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
    float g = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
    float b = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
    float a = arr.size() >= 4 ? static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0) : 1.0f;
    return ImGui::ColorConvertFloat4ToU32({r, g, b, a});
}

void MapRenderer::renderDrawCommands(Value& commandsVal, float originX, float originY) {
    if (!commandsVal.isArray()) return;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    for (auto& cmd : commandsVal.asArrayMut()) {
        if (!cmd.isMap()) continue;
        auto& cm = cmd.asMap();

        auto typeVal = cm.get(syms_.type);
        if (!typeVal.isSymbol()) continue;
        uint32_t sym = typeVal.asSymbol();

        ImU32 col = readColorU32(cm.get(syms_.color));
        float thick = static_cast<float>(getNumericField(cm, syms_.thickness, 1.0));
        bool isFilled = getBoolField(cm, syms_.filled, false);

        if (sym == syms_.sym_draw_line) {
            float x1, y1, x2, y2;
            if (readVec2(cm.get(syms_.p1), x1, y1) && readVec2(cm.get(syms_.p2), x2, y2)) {
                drawList->AddLine({originX + x1, originY + y1},
                                  {originX + x2, originY + y2}, col, thick);
            }
        } else if (sym == syms_.sym_draw_rect) {
            float x1, y1, x2, y2;
            if (readVec2(cm.get(syms_.p1), x1, y1) && readVec2(cm.get(syms_.p2), x2, y2)) {
                if (isFilled) {
                    drawList->AddRectFilled({originX + x1, originY + y1},
                                            {originX + x2, originY + y2}, col);
                } else {
                    drawList->AddRect({originX + x1, originY + y1},
                                      {originX + x2, originY + y2}, col, 0.0f, 0, thick);
                }
            }
        } else if (sym == syms_.sym_draw_circle) {
            float cx, cy;
            if (readVec2(cm.get(syms_.center), cx, cy)) {
                float r = static_cast<float>(getNumericField(cm, syms_.radius, 10.0));
                if (isFilled) {
                    drawList->AddCircleFilled({originX + cx, originY + cy}, r, col);
                } else {
                    drawList->AddCircle({originX + cx, originY + cy}, r, col, 0, thick);
                }
            }
        } else if (sym == syms_.sym_draw_text) {
            float px, py;
            if (readVec2(cm.get(syms_.pos), px, py)) {
                auto text = getStringField(cm, syms_.text, "");
                if (!text.empty()) {
                    drawList->AddText({originX + px, originY + py}, col, text.c_str());
                }
            }
        } else if (sym == syms_.sym_draw_triangle) {
            float x1, y1, x2, y2;
            // Triangle uses p1, p2, and center as the third point
            float x3, y3;
            if (readVec2(cm.get(syms_.p1), x1, y1) &&
                readVec2(cm.get(syms_.p2), x2, y2) &&
                readVec2(cm.get(syms_.center), x3, y3)) {
                if (isFilled) {
                    drawList->AddTriangleFilled(
                        {originX + x1, originY + y1},
                        {originX + x2, originY + y2},
                        {originX + x3, originY + y3}, col);
                } else {
                    drawList->AddTriangle(
                        {originX + x1, originY + y1},
                        {originX + x2, originY + y2},
                        {originX + x3, originY + y3}, col, thick);
                }
            }
        }
    }
}

void MapRenderer::renderCanvas(MapData& m, ExecutionContext& ctx) {
    auto id = getStringField(m, syms_.id, "##canvas");
    float w = static_cast<float>(getNumericField(m, syms_.width, 200.0));
    float h = static_cast<float>(getNumericField(m, syms_.height, 200.0));
    if (w <= 0) w = 200.0f;
    if (h <= 0) h = 200.0f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // Reserve space
    ImGui::InvisibleButton(id.c_str(), {w, h});
    bool isClicked = ImGui::IsItemClicked();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background color
    auto bgVal = m.get(syms_.bg_color);
    if (bgVal.isArray()) {
        ImU32 bgCol = readColorU32(bgVal);
        drawList->AddRectFilled(canvasPos, {canvasPos.x + w, canvasPos.y + h}, bgCol);
    }

    // Border
    auto borderVal = m.get(syms_.border);
    if (borderVal.isBool() && borderVal.asBool()) {
        ImU32 borderCol = ImGui::ColorConvertFloat4ToU32({0.5f, 0.5f, 0.5f, 1.0f});
        drawList->AddRect(canvasPos, {canvasPos.x + w, canvasPos.y + h}, borderCol);
    }

    // Render draw commands from :commands array
    auto cmdsVal = m.get(syms_.commands);
    if (cmdsVal.isArray()) {
        renderDrawCommands(cmdsVal, canvasPos.x, canvasPos.y);
    }

    if (isClicked) {
        invokeCallback(m, syms_.on_click, ctx);
    }
}

void MapRenderer::renderTooltip(MapData& m, ExecutionContext& ctx) {
    if (!ImGui::IsItemHovered()) return;

    auto text = getStringField(m, syms_.text, "");
    auto childrenVal = m.get(syms_.children);
    bool hasChildren = childrenVal.isArray() && !childrenVal.asArray().empty();

    if (!text.empty() && !hasChildren) {
        // Simple text tooltip
        ImGui::SetItemTooltip("%s", text.c_str());
    } else if (hasChildren) {
        // Rich tooltip
        if (ImGui::BeginTooltip()) {
            if (!text.empty()) {
                ImGui::TextUnformatted(text.c_str());
            }
            for (auto& child : childrenVal.asArrayMut()) {
                if (child.isMap()) {
                    renderNode(child.asMap(), ctx);
                }
            }
            ImGui::EndTooltip();
        }
    }

    (void)ctx;
}

} // namespace finegui
