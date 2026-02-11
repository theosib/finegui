#include <finegui/map_renderer.hpp>
#include <finescript/map_data.h>
#include <imgui.h>
#include <cstring>
#include <cfloat>
#include <cmath>

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

void MapRenderer::setDragDropManager(DragDropManager* manager) {
    dndManager_ = manager;
}

void MapRenderer::setTextureRegistry(TextureRegistry* registry) {
    textureRegistry_ = registry;
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

void MapRenderer::setFocus(const std::string& widgetId) {
    pendingFocusId_ = widgetId;
}

finescript::Value MapRenderer::findByIdRecursive(finescript::Value& node, uint32_t symId, const std::string& strId) {
    if (!node.isMap()) return finescript::Value::nil();
    auto& m = node.asMap();
    auto idVal = m.get(syms_.id);
    if (idVal.isSymbol()) {
        if (idVal.asSymbol() == symId) return node;
    } else if (idVal.isString()) {
        if (idVal.asString() == strId) return node;
    }
    auto childrenVal = m.get(syms_.children);
    if (childrenVal.isArray()) {
        for (auto& child : childrenVal.asArrayMut()) {
            auto found = findByIdRecursive(child, symId, strId);
            if (!found.isNil()) return found;
        }
    }
    return finescript::Value::nil();
}

finescript::Value MapRenderer::findById(const std::string& widgetId) {
    if (widgetId.empty()) return finescript::Value::nil();
    uint32_t sym = engine_.intern(widgetId);
    for (auto& [id, entry] : trees_) {
        auto found = findByIdRecursive(entry.rootMap, sym, widgetId);
        if (!found.isNil()) return found;
    }
    return finescript::Value::nil();
}

finescript::Value MapRenderer::findById(uint32_t symbolId) {
    if (symbolId == 0) return finescript::Value::nil();
    std::string str(engine_.lookupSymbol(symbolId));
    if (str.empty()) return finescript::Value::nil();
    for (auto& [id, entry] : trees_) {
        auto found = findByIdRecursive(entry.rootMap, symbolId, str);
        if (!found.isNil()) return found;
    }
    return finescript::Value::nil();
}

void MapRenderer::renderAll() {
    currentFocusedId_.clear();
    for (auto& [id, entry] : trees_) {
        if (entry.rootMap.isMap()) {
            renderNode(entry.rootMap.asMap(), *entry.ctx);
        }
    }
    lastFocusedId_ = currentFocusedId_;
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

    // Focus: exclude from tab navigation if not focusable
    bool pushedNoTabStop = false;
    bool isFocusable = getBoolField(m, syms_.focusable, true);
    if (!isFocusable) {
        ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
        pushedNoTabStop = true;
    }

    // Focus: programmatic focus request
    std::string widgetId;
    if (idVal.isString()) widgetId = std::string(idVal.asString());
    if (!pendingFocusId_.empty() && !widgetId.empty() && widgetId == pendingFocusId_) {
        ImGui::SetKeyboardFocusHere(0);
        pendingFocusId_.clear();
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
        else if (sym == syms_.sym_image)      renderImage(m, ctx);
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
        // Phase 9
        else if (sym == syms_.sym_radio_button)    renderRadioButton(m, ctx);
        else if (sym == syms_.sym_selectable)      renderSelectable(m, ctx);
        else if (sym == syms_.sym_input_multiline) renderInputTextMultiline(m, ctx);
        else if (sym == syms_.sym_bullet_text)     renderBulletText(m);
        else if (sym == syms_.sym_separator_text)  renderSeparatorText(m);
        else if (sym == syms_.sym_indent)          renderIndent(m);
        else if (sym == syms_.sym_unindent)        renderUnindent(m);
        // Phase 10
        else if (sym == syms_.sym_push_color)    renderPushStyleColor(m);
        else if (sym == syms_.sym_pop_color)     renderPopStyleColor(m);
        else if (sym == syms_.sym_push_var)      renderPushStyleVar(m);
        else if (sym == syms_.sym_pop_var)       renderPopStyleVar(m);
        // Phase 11
        else if (sym == syms_.sym_dummy)         renderDummy(m);
        else if (sym == syms_.sym_new_line)      renderNewLine();
        // Phase 12
        else if (sym == syms_.sym_drag_float3)       renderDragFloat3(m, ctx);
        else if (sym == syms_.sym_input_with_hint)   renderInputTextWithHint(m, ctx);
        else if (sym == syms_.sym_slider_angle)      renderSliderAngle(m, ctx);
        else if (sym == syms_.sym_small_button)      renderSmallButton(m, ctx);
        else if (sym == syms_.sym_color_button)      renderColorButton(m, ctx);
        else {
            ImGui::TextColored({1, 0, 0, 1}, "[Unknown widget type]");
        }
    }

    // Focus: auto-focus on first appearance
    bool wantAutoFocus = getBoolField(m, syms_.auto_focus, false);
    if (wantAutoFocus) {
        ImGui::SetItemDefaultFocus();
    }

    // Focus: track focus changes for on_focus/on_blur callbacks
    if (!widgetId.empty()) {
        if (ImGui::IsItemFocused()) {
            currentFocusedId_ = widgetId;
            if (widgetId != lastFocusedId_) {
                invokeCallback(m, syms_.on_focus, ctx);
            }
        } else if (widgetId == lastFocusedId_) {
            invokeCallback(m, syms_.on_blur, ctx);
        }
    }

    if (pushedNoTabStop) {
        ImGui::PopItemFlag();
    }

    // DnD handling (after widget is rendered so ImGui has the item rect)
    handleDragDrop(m, ctx);

    if (pushId) ImGui::PopID();
    if (wasDisabled) ImGui::EndDisabled();
}

// -- Per-widget render methods ------------------------------------------------

void MapRenderer::renderWindow(MapData& m, ExecutionContext& ctx) {
    // Window uses :title for the window label
    auto title = getStringField(m, syms_.title, "Untitled");
    int wflags = parseWindowFlags(m);

    // Animation: explicit window position
    float posX = getNumericField(m, syms_.window_pos_x, FLT_MAX);
    float posY = getNumericField(m, syms_.window_pos_y, FLT_MAX);
    if (posX != FLT_MAX && posY != FLT_MAX) {
        ImGui::SetNextWindowPos(ImVec2(posX, posY));
    }

    // Animation: window alpha
    float alpha = getNumericField(m, syms_.window_alpha, 1.0f);
    bool pushedAlpha = alpha < 1.0f;
    if (pushedAlpha) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    }

    // Animation: scale and rotation
    float scaleX = static_cast<float>(getNumericField(m, syms_.scale_x, 1.0));
    float scaleY = static_cast<float>(getNumericField(m, syms_.scale_y, 1.0));
    float rotY = static_cast<float>(getNumericField(m, syms_.rotation_y, 0.0));

    bool open = true;
    bool windowOpen = ImGui::Begin(title.c_str(), &open, static_cast<ImGuiWindowFlags>(wflags));

    // Capture draw list and window geometry for vertex post-processing
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    int vtxStart = drawList->VtxBuffer.Size;

    if (windowOpen) {
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

    if (pushedAlpha) {
        ImGui::PopStyleVar();
    }

    // Post-process vertices for zoom/flip transforms
    bool needsTransform = scaleX != 1.0f || scaleY != 1.0f || rotY != 0.0f;
    if (needsTransform && drawList->VtxBuffer.Size > vtxStart) {
        float cx = windowPos.x + windowSize.x * 0.5f;
        float cy = windowPos.y + windowSize.y * 0.5f;
        float cosR = std::cos(rotY);
        float sinR = std::sin(rotY);
        constexpr float perspD = 800.0f;

        for (int i = vtxStart; i < drawList->VtxBuffer.Size; i++) {
            ImDrawVert& v = drawList->VtxBuffer.Data[i];
            float dx = v.pos.x - cx;
            float dy = v.pos.y - cy;

            dx *= scaleX;
            dy *= scaleY;

            if (rotY != 0.0f) {
                float xRot = dx * cosR;
                float z = dx * sinR;
                float pScale = perspD / (perspD + z);
                dx = xRot * pScale;
                dy *= pScale;
            }

            v.pos.x = cx + dx;
            v.pos.y = cy + dy;
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        for (int i = 0; i < drawList->CmdBuffer.Size; i++) {
            ImDrawCmd& cmd = drawList->CmdBuffer.Data[i];
            cmd.ClipRect = ImVec4(0, 0, displaySize.x, displaySize.y);
        }
    }

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

void MapRenderer::renderImage(MapData& m, ExecutionContext& ctx) {
    auto texName = getStringField(m, syms_.texture, "");
    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));

    if (texName.empty() || !textureRegistry_) {
        ImGui::TextDisabled("[image: %s]",
                            texName.empty() ? "no texture" : texName.c_str());
        return;
    }

    auto handle = textureRegistry_->get(texName);
    if (!handle.valid()) {
        ImGui::TextDisabled("[image: %s not found]", texName.c_str());
        return;
    }

    // Default to texture dimensions if width/height not specified
    if (w <= 0) w = static_cast<float>(handle.width);
    if (h <= 0) h = static_cast<float>(handle.height);

    ImGui::Image(static_cast<ImTextureID>(handle), {w, h});

    if (ImGui::IsItemClicked()) {
        invokeCallback(m, syms_.on_click, ctx);
    }
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
        // Escape key closes the modal (ImGui doesn't do this for modals by default)
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            open = false;
            ImGui::CloseCurrentPopup();
        }

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

// -- Phase 9 ------------------------------------------------------------------

void MapRenderer::renderRadioButton(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Radio");
    int activeValue = static_cast<int>(getNumericField(m, syms_.value, 0));
    int myValue = static_cast<int>(getNumericField(m, syms_.my_value, 0));

    if (ImGui::RadioButton(label.c_str(), &activeValue, myValue)) {
        m.set(syms_.value, Value::integer(activeValue));
        invokeCallback(m, syms_.on_change, ctx, {Value::integer(activeValue)});
    }
}

void MapRenderer::renderSelectable(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Selectable");
    bool selected = getBoolField(m, syms_.value, false);

    if (ImGui::Selectable(label.c_str(), &selected)) {
        m.set(syms_.value, Value::boolean(selected));
        invokeCallback(m, syms_.on_click, ctx);
    }
}

void MapRenderer::renderInputTextMultiline(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Input");
    auto valEntry = m.get(syms_.value);
    if (!valEntry.isString()) {
        m.set(syms_.value, Value::string(""));
        valEntry = m.get(syms_.value);
    }

    auto& str = valEntry.asStringMut();
    if (str.capacity() < 1024) str.reserve(1024);
    size_t cap = str.capacity();
    str.resize(cap);

    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));

    InputTextCallbackData cbData{&str};
    ImGui::InputTextMultiline(
        label.c_str(), str.data(), str.size() + 1,
        {w, h},
        ImGuiInputTextFlags_CallbackResize, inputTextResizeCallback, &cbData);

    str.resize(std::strlen(str.c_str()));

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        invokeCallback(m, syms_.on_change, ctx, {Value::string(str)});
    }
}

void MapRenderer::renderBulletText(MapData& m) {
    auto text = getStringField(m, syms_.text);
    ImGui::BulletText("%s", text.c_str());
}

void MapRenderer::renderSeparatorText(MapData& m) {
    auto label = getStringField(m, syms_.label, "");
    ImGui::SeparatorText(label.c_str());
}

void MapRenderer::renderIndent(MapData& m) {
    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    ImGui::Indent(w > 0 ? w : 0.0f);
}

void MapRenderer::renderUnindent(MapData& m) {
    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    ImGui::Unindent(w > 0 ? w : 0.0f);
}

// -- Phase 11: Layout Helpers -------------------------------------------------

void MapRenderer::renderDummy(MapData& m) {
    float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
    float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));
    ImGui::Dummy(ImVec2(w, h));
}

void MapRenderer::renderNewLine() {
    ImGui::NewLine();
}

// -- Phase 12: Advanced Input (continued) -------------------------------------

void MapRenderer::renderDragFloat3(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Drag3");
    float speed = static_cast<float>(getNumericField(m, syms_.speed, 1.0));
    float min = static_cast<float>(getNumericField(m, syms_.min, 0.0));
    float max = static_cast<float>(getNumericField(m, syms_.max, 0.0));

    // Read value from :value array [x, y, z]
    float v[3] = {0.0f, 0.0f, 0.0f};
    auto valArr = m.get(syms_.value);
    if (valArr.isArray()) {
        const auto& arr = valArr.asArray();
        if (arr.size() >= 1 && arr[0].isNumeric()) v[0] = static_cast<float>(arr[0].asNumber());
        if (arr.size() >= 2 && arr[1].isNumeric()) v[1] = static_cast<float>(arr[1].asNumber());
        if (arr.size() >= 3 && arr[2].isNumeric()) v[2] = static_cast<float>(arr[2].asNumber());
    }

    if (ImGui::DragFloat3(label.c_str(), v, speed, min, max)) {
        auto newVal = Value::array({
            Value::number(v[0]), Value::number(v[1]), Value::number(v[2])
        });
        m.set(syms_.value, newVal);
        invokeCallback(m, syms_.on_change, ctx, {newVal});
    }
}

void MapRenderer::renderInputTextWithHint(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Input");
    auto hint = getStringField(m, syms_.hint, "");

    auto valEntry = m.get(syms_.value);
    if (!valEntry.isString()) {
        m.set(syms_.value, Value::string(""));
        valEntry = m.get(syms_.value);
    }

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

    bool enterPressed = ImGui::InputTextWithHint(
        label.c_str(), hint.c_str(),
        str.data(), str.size() + 1,
        flags, inputTextResizeCallback, &cbData);

    str.resize(std::strlen(str.c_str()));

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        invokeCallback(m, syms_.on_change, ctx, {Value::string(str)});
    }

    if (enterPressed && onSubmit.isCallable()) {
        invokeCallback(m, syms_.on_submit, ctx, {Value::string(str)});
    }
}

void MapRenderer::renderSliderAngle(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Angle");
    float value = static_cast<float>(getNumericField(m, syms_.value, 0.0));
    float minDeg = static_cast<float>(getNumericField(m, syms_.min, -360.0));
    float maxDeg = static_cast<float>(getNumericField(m, syms_.max, 360.0));

    if (ImGui::SliderAngle(label.c_str(), &value, minDeg, maxDeg)) {
        m.set(syms_.value, Value::number(value));
        invokeCallback(m, syms_.on_change, ctx, {Value::number(value)});
    }
}

void MapRenderer::renderSmallButton(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Button");
    if (ImGui::SmallButton(label.c_str())) {
        invokeCallback(m, syms_.on_click, ctx);
    }
}

void MapRenderer::renderColorButton(MapData& m, ExecutionContext& ctx) {
    auto label = getStringField(m, syms_.label, "Color");

    // Read color from :color array [r, g, b, a]
    ImVec4 col{1.0f, 1.0f, 1.0f, 1.0f};
    auto colorVal = m.get(syms_.color);
    if (colorVal.isArray()) {
        const auto& arr = colorVal.asArray();
        if (arr.size() >= 3) {
            col.x = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
            col.y = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
            col.z = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
            if (arr.size() >= 4)
                col.w = static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0);
        }
    }

    if (ImGui::ColorButton(label.c_str(), col)) {
        invokeCallback(m, syms_.on_click, ctx);
    }
}

// -- Phase 10: Style Push/Pop -------------------------------------------------

static bool isStyleVarVec2(int idx) {
    switch (idx) {
        case ImGuiStyleVar_WindowPadding:
        case ImGuiStyleVar_WindowMinSize:
        case ImGuiStyleVar_WindowTitleAlign:
        case ImGuiStyleVar_FramePadding:
        case ImGuiStyleVar_ItemSpacing:
        case ImGuiStyleVar_ItemInnerSpacing:
        case ImGuiStyleVar_CellPadding:
        case ImGuiStyleVar_TableAngledHeadersTextAlign:
        case ImGuiStyleVar_ButtonTextAlign:
        case ImGuiStyleVar_SelectableTextAlign:
        case ImGuiStyleVar_SeparatorTextAlign:
        case ImGuiStyleVar_SeparatorTextPadding:
            return true;
        default:
            return false;
    }
}

void MapRenderer::renderPushStyleColor(MapData& m) {
    int colIdx = static_cast<int>(getNumericField(m, syms_.value, 0));

    // Read color from :color field — expects array [r, g, b, a]
    ImVec4 col{1.0f, 1.0f, 1.0f, 1.0f};
    auto colorVal = m.get(syms_.color);
    if (colorVal.isArray()) {
        const auto& arr = colorVal.asArray();
        if (arr.size() >= 3) {
            col.x = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 1.0);
            col.y = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 1.0);
            col.z = static_cast<float>(arr[2].isNumeric() ? arr[2].asNumber() : 1.0);
            if (arr.size() >= 4)
                col.w = static_cast<float>(arr[3].isNumeric() ? arr[3].asNumber() : 1.0);
        }
    }

    ImGui::PushStyleColor(colIdx, col);
}

void MapRenderer::renderPopStyleColor(MapData& m) {
    int count = static_cast<int>(getNumericField(m, syms_.count, 1));
    ImGui::PopStyleColor(count);
}

void MapRenderer::renderPushStyleVar(MapData& m) {
    int varIdx = static_cast<int>(getNumericField(m, syms_.value, 0));

    // Read from :size field — number for float vars, array [x, y] for Vec2 vars
    auto sizeVal = m.get(syms_.size);
    if (isStyleVarVec2(varIdx)) {
        float x = 0.0f, y = 0.0f;
        if (sizeVal.isArray() && sizeVal.asArray().size() >= 2) {
            const auto& arr = sizeVal.asArray();
            x = static_cast<float>(arr[0].isNumeric() ? arr[0].asNumber() : 0.0);
            y = static_cast<float>(arr[1].isNumeric() ? arr[1].asNumber() : 0.0);
        }
        ImGui::PushStyleVar(varIdx, ImVec2(x, y));
    } else {
        float val = sizeVal.isNumeric() ? static_cast<float>(sizeVal.asNumber()) : 0.0f;
        ImGui::PushStyleVar(varIdx, val);
    }
}

void MapRenderer::renderPopStyleVar(MapData& m) {
    int count = static_cast<int>(getNumericField(m, syms_.count, 1));
    ImGui::PopStyleVar(count);
}

int MapRenderer::parseWindowFlags(MapData& m) {
    int result = 0;
    auto flagsVal = m.get(syms_.window_flags);
    if (flagsVal.isArray()) {
        for (const auto& f : flagsVal.asArray()) {
            if (!f.isSymbol()) continue;
            uint32_t s = f.asSymbol();
            if (s == syms_.sym_flag_no_title_bar)          result |= ImGuiWindowFlags_NoTitleBar;
            else if (s == syms_.sym_flag_no_resize)        result |= ImGuiWindowFlags_NoResize;
            else if (s == syms_.sym_flag_no_move)          result |= ImGuiWindowFlags_NoMove;
            else if (s == syms_.sym_flag_no_scrollbar)     result |= ImGuiWindowFlags_NoScrollbar;
            else if (s == syms_.sym_flag_no_collapse)      result |= ImGuiWindowFlags_NoCollapse;
            else if (s == syms_.sym_flag_always_auto_resize) result |= ImGuiWindowFlags_AlwaysAutoResize;
            else if (s == syms_.sym_flag_no_background)    result |= ImGuiWindowFlags_NoBackground;
            else if (s == syms_.sym_flag_menu_bar)         result |= ImGuiWindowFlags_MenuBar;
        }
    } else if (flagsVal.isInt()) {
        result = static_cast<int>(flagsVal.asInt());
    }
    return result;
}

// -- Drag and Drop ------------------------------------------------------------

void MapRenderer::handleDragDrop(MapData& m, ExecutionContext& ctx) {
    auto dragTypeStr = getStringField(m, syms_.drag_type, "");
    auto dropAcceptStr = getStringField(m, syms_.drop_accept, "");

    bool isDragSource = !dragTypeStr.empty();
    bool isDropTarget = !dropAcceptStr.empty();
    if (!isDragSource && !isDropTarget) return;

    int mode = static_cast<int>(getNumericField(m, syms_.drag_mode, 0));
    bool allowTraditional = (mode == 0 || mode == 1);
    bool allowClickPickup = (mode == 0 || mode == 2);

    // === DRAG SOURCE ===
    if (isDragSource) {
        auto dragDataStr = getStringField(m, syms_.drag_data, "");

        // Traditional ImGui DnD
        if (allowTraditional) {
            ImGuiDragDropFlags srcFlags = ImGuiDragDropFlags_SourceAllowNullID;
            if (ImGui::BeginDragDropSource(srcFlags)) {
                ImGui::SetDragDropPayload(dragTypeStr.c_str(),
                    dragDataStr.data(), dragDataStr.size());

                // Preview: show image if available, otherwise text
                bool previewShown = false;
                auto typeVal = m.get(syms_.type);
                if (typeVal.isSymbol() && typeVal.asSymbol() == syms_.sym_image
                    && textureRegistry_) {
                    auto texName = getStringField(m, syms_.texture, "");
                    if (!texName.empty()) {
                        auto handle = textureRegistry_->get(texName);
                        if (handle.valid()) {
                            float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
                            float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));
                            if (w <= 0) w = static_cast<float>(handle.width);
                            if (h <= 0) h = static_cast<float>(handle.height);
                            ImGui::Image(static_cast<ImTextureID>(handle), {w, h});
                            previewShown = true;
                        }
                    }
                }
                if (!previewShown) {
                    auto label = getStringField(m, syms_.label, "");
                    if (!label.empty()) {
                        ImGui::TextUnformatted(label.c_str());
                    } else if (!dragDataStr.empty()) {
                        ImGui::TextUnformatted(dragDataStr.c_str());
                    }
                }

                ImGui::EndDragDropSource();
            }
        }

        // Click-to-pick-up
        if (allowClickPickup && dndManager_ && !dndManager_->isHolding()) {
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                if (!ImGui::GetDragDropPayload()) {
                    DragDropManager::CursorItem item;
                    item.type = dragTypeStr;
                    item.data = dragDataStr;
                    item.fallbackText = getStringField(m, syms_.label, dragDataStr.c_str());

                    // Resolve texture icon for image widgets
                    auto typeVal2 = m.get(syms_.type);
                    if (typeVal2.isSymbol() && typeVal2.asSymbol() == syms_.sym_image
                        && textureRegistry_) {
                        auto texName = getStringField(m, syms_.texture, "");
                        if (!texName.empty()) {
                            auto handle = textureRegistry_->get(texName);
                            if (handle.valid()) {
                                item.textureId = static_cast<ImTextureID>(handle);
                                float w = static_cast<float>(getNumericField(m, syms_.width, 0.0));
                                float h = static_cast<float>(getNumericField(m, syms_.height, 0.0));
                                item.iconWidth = (w > 0) ? w : static_cast<float>(handle.width);
                                item.iconHeight = (h > 0) ? h : static_cast<float>(handle.height);
                            }
                        }
                    }

                    dndManager_->pickUp(item);
                    invokeCallback(m, syms_.on_drag, ctx);
                }
            }
        }
    }

    // === DROP TARGET ===
    if (isDropTarget) {
        // Traditional ImGui DnD
        if (allowTraditional) {
            if (ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(dropAcceptStr.c_str());
                if (payload) {
                    auto deliveredData = std::string(
                        static_cast<const char*>(payload->Data),
                        static_cast<size_t>(payload->DataSize));
                    m.set(syms_.drag_data, Value::string(deliveredData));
                    invokeCallback(m, syms_.on_drop, ctx,
                                   {Value::string(deliveredData)});
                }
                ImGui::EndDragDropTarget();
            }
        }

        // Click-to-pick-up
        if (dndManager_ && dndManager_->isHolding(dropAcceptStr)) {
            if (ImGui::IsItemHovered()) {
                // Visual highlight: yellow border
                ImVec2 rMin = ImGui::GetItemRectMin();
                ImVec2 rMax = ImGui::GetItemRectMax();
                ImGui::GetForegroundDrawList()->AddRect(
                    rMin, rMax,
                    ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 0.0f, 0.8f}),
                    0.0f, 0, 2.0f);

                // Click to deliver
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    auto delivered = dndManager_->dropItem();
                    m.set(syms_.drag_data, Value::string(delivered.data));
                    invokeCallback(m, syms_.on_drop, ctx,
                                   {Value::string(delivered.data)});
                }
            }
        }
    }
}

} // namespace finegui
