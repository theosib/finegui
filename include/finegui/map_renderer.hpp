#pragma once

#include <finegui/widget_converter.hpp>
#include <finescript/value.h>
#include <finescript/script_engine.h>
#include <finescript/execution_context.h>
#include <map>
#include <vector>

namespace finegui {

/// Renders GUI trees stored as finescript maps.
///
/// Unlike GuiRenderer (which renders WidgetNode trees), MapRenderer reads
/// directly from finescript maps each frame. Because finescript maps use
/// shared_ptr<MapData>, script mutations to maps are automatically visible
/// to the renderer, and ImGui value writebacks are automatically visible
/// to scripts.
///
/// Usage:
///   MapRenderer renderer(engine);
///   int id = renderer.show(mapTree, ctx);
///   // Each frame (between gui.beginFrame/endFrame):
///   renderer.renderAll();
class MapRenderer {
public:
    explicit MapRenderer(finescript::ScriptEngine& engine);

    /// Register a map tree to be rendered each frame.
    /// ctx is used for invoking callbacks (closures stored in the map).
    int show(finescript::Value rootMap, finescript::ExecutionContext& ctx);

    /// Remove a map tree.
    void hide(int id);

    /// Remove all map trees.
    void hideAll();

    /// Get a reference to a stored root map Value.
    /// Returns nullptr if the ID is not found.
    finescript::Value* get(int id);

    /// Render all registered map trees. Call between beginFrame/endFrame.
    void renderAll();

    /// Access the pre-interned symbols (for navigation by other classes).
    const ConverterSymbols& syms() const { return syms_; }

private:
    struct Entry {
        finescript::Value rootMap;
        finescript::ExecutionContext* ctx;
    };

    finescript::ScriptEngine& engine_;
    ConverterSymbols syms_;
    int nextId_ = 1;
    std::map<int, Entry> trees_;

    void renderNode(finescript::MapData& map, finescript::ExecutionContext& ctx);

    // Per-widget render methods
    void renderWindow(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderText(finescript::MapData& m);
    void renderButton(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderCheckbox(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderSlider(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderSliderInt(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderInputText(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderInputInt(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderInputFloat(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderCombo(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderSeparator();
    void renderGroup(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderColumns(finescript::MapData& m, finescript::ExecutionContext& ctx);

    // Phase 3 - Layout & Display
    void renderSameLine(finescript::MapData& m);
    void renderSpacing();
    void renderTextColored(finescript::MapData& m);
    void renderTextWrapped(finescript::MapData& m);
    void renderTextDisabled(finescript::MapData& m);
    void renderProgressBar(finescript::MapData& m);
    void renderCollapsingHeader(finescript::MapData& m, finescript::ExecutionContext& ctx);

    // Phase 4 - Containers & Menus
    void renderTabBar(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderTab(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderTreeNode(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderChild(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderMenuBar(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderMenu(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderMenuItem(finescript::MapData& m, finescript::ExecutionContext& ctx);

    // Phase 5 - Tables
    void renderTable(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderTableRow(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderTableNextColumn();
    int parseTableFlags(finescript::MapData& m);

    // Phase 6 - Advanced Input
    void renderColorEdit(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderColorPicker(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderDragFloat(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderDragInt(finescript::MapData& m, finescript::ExecutionContext& ctx);

    // Phase 7 - Misc
    void renderListBox(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderPopup(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderModal(finescript::MapData& m, finescript::ExecutionContext& ctx);

    // Phase 8 - Custom
    void renderCanvas(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderTooltip(finescript::MapData& m, finescript::ExecutionContext& ctx);
    void renderDrawCommands(finescript::Value& commandsVal, float originX, float originY);

    // Helpers
    std::string getStringField(finescript::MapData& m, uint32_t key, const char* def = "");
    double getNumericField(finescript::MapData& m, uint32_t key, double def = 0.0);
    bool getBoolField(finescript::MapData& m, uint32_t key, bool def = true);
    void invokeCallback(finescript::MapData& m, uint32_t key,
                        finescript::ExecutionContext& ctx,
                        std::vector<finescript::Value> args = {});
};

} // namespace finegui
