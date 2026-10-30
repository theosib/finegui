#include "finegui/skin_loader.hpp"
#include "finegui/gui_system.hpp"

#include <finescript/execution_context.h>
#include <finescript/map_data.h>
#include <finescript/script_engine.h>
#include <finescript/value.h>
#include <finevk/finevk.hpp>

namespace finegui {

using finescript::ScriptEngine;
using finescript::Value;

namespace {

struct PartName { const char* name; ImGuiSkinPart part; };
constexpr PartName PART_NAMES[] = {
    {"window", ImGuiSkinPart_WindowBg},
    {"popup", ImGuiSkinPart_PopupBg},
    {"tooltip", ImGuiSkinPart_TooltipBg},
    {"title_bar", ImGuiSkinPart_TitleBar},
    {"menu_bar", ImGuiSkinPart_MenuBar},
    {"button", ImGuiSkinPart_Button},
    {"close_button", ImGuiSkinPart_CloseButton},
    {"frame", ImGuiSkinPart_Frame},
    {"checkbox", ImGuiSkinPart_Checkbox},
    {"radio", ImGuiSkinPart_Radio},
    {"slider_track", ImGuiSkinPart_SliderTrack},
    {"slider_grab", ImGuiSkinPart_SliderGrab},
    {"progress_track", ImGuiSkinPart_ProgressTrack},
    {"progress_fill", ImGuiSkinPart_ProgressFill},
    {"scrollbar_track", ImGuiSkinPart_ScrollbarTrack},
    {"scrollbar_grab", ImGuiSkinPart_ScrollbarGrab},
    {"header", ImGuiSkinPart_Header},
    {"tab", ImGuiSkinPart_Tab},
};

struct StateName { const char* name; ImGuiSkinState state; };
constexpr StateName STATE_NAMES[] = {
    {"normal", ImGuiSkinState_Normal},
    {"hovered", ImGuiSkinState_Hovered},
    {"active", ImGuiSkinState_Active},
    {"selected", ImGuiSkinState_Selected},
};

Value field(const Value& map, ScriptEngine& engine, const char* name) {
    return const_cast<Value&>(map).asMap().get(engine.intern(name));
}

bool readRegion(const Value& value, ScriptEngine& engine, SkinRegion& out) {
    if (!value.isMap()) return false;
    auto number = [&](const char* name) {
        Value v = field(value, engine, name);
        return v.isNumeric() ? static_cast<float>(v.asNumber()) : 0.0f;
    };
    out.x = number("x");
    out.y = number("y");
    out.w = number("w");
    out.h = number("h");
    Value inset = field(value, engine, "inset");
    if (inset.isArray() && inset.asArray().size() == 4) {
        const auto& sides = inset.asArray();
        out.left = static_cast<float>(sides[0].asNumber());
        out.top = static_cast<float>(sides[1].asNumber());
        out.right = static_cast<float>(sides[2].asNumber());
        out.bottom = static_cast<float>(sides[3].asNumber());
    }
    return out.w > 0.0f && out.h > 0.0f;
}

void readStyle(const Value& style, ScriptEngine& engine, ImGuiStyle& out) {
    auto scalar = [&](const char* name, float& target) {
        Value v = field(style, engine, name);
        if (v.isNumeric()) target = static_cast<float>(v.asNumber());
    };
    auto pair = [&](const char* name, ImVec2& target) {
        Value v = field(style, engine, name);
        if (v.isArray() && v.asArray().size() == 2) {
            target = ImVec2(static_cast<float>(v.asArray()[0].asNumber()),
                            static_cast<float>(v.asArray()[1].asNumber()));
        }
    };
    pair("window_padding", out.WindowPadding);
    pair("frame_padding", out.FramePadding);
    pair("item_spacing", out.ItemSpacing);
    pair("item_inner_spacing", out.ItemInnerSpacing);
    pair("cell_padding", out.CellPadding);
    pair("window_title_align", out.WindowTitleAlign);
    pair("button_text_align", out.ButtonTextAlign);
    scalar("scrollbar_size", out.ScrollbarSize);
    scalar("grab_min_size", out.GrabMinSize);
    scalar("indent_spacing", out.IndentSpacing);
}

void readColors(const Value& colors, ScriptEngine& engine, ImGuiStyle& out) {
    // Keys are ImGui's own colour names, so a skin can set any of them
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        Value v = field(colors, engine, ImGui::GetStyleColorName(i));
        if (!v.isArray() || v.asArray().size() != 4) continue;
        const auto& c = v.asArray();
        out.Colors[i] = ImVec4(static_cast<float>(c[0].asNumber()), static_cast<float>(c[1].asNumber()),
                               static_cast<float>(c[2].asNumber()), static_cast<float>(c[3].asNumber()));
    }
}

}  // namespace

std::optional<SkinDefinition> readSkinDefinition(const Value& definition, ScriptEngine& engine,
                                                 std::string& error) {
    if (!definition.isMap()) {
        error = "a skin definition must evaluate to a map";
        return std::nullopt;
    }

    SkinDefinition out;
    Value atlas = field(definition, engine, "atlas");
    if (!atlas.isString()) {
        error = "a skin needs =atlas";
        return std::nullopt;
    }
    out.atlasFile = std::string(atlas.asString());

    Value scale = field(definition, engine, "units_per_pixel");
    if (scale.isNumeric()) out.skin.setUnitsPerPixel(static_cast<float>(scale.asNumber()));

    Value font = field(definition, engine, "font");
    if (font.isString()) out.fontFile = std::string(font.asString());
    Value fontSize = field(definition, engine, "font_size");
    if (fontSize.isNumeric()) out.fontSize = static_cast<float>(fontSize.asNumber());

    // Start from a neutral dark style, with the square corners and no outlines that art needs
    ImGuiStyle& style = out.skin.style();
    ImGui::StyleColorsDark(&style);
    style.WindowRounding = style.FrameRounding = style.GrabRounding = style.ScrollbarRounding =
        style.TabRounding = style.PopupRounding = style.ChildRounding = 0.0f;
    style.WindowBorderSize = style.FrameBorderSize = style.PopupBorderSize = style.TabBorderSize = 0.0f;
    style.TabBarBorderSize = 0.0f;

    Value styleMap = field(definition, engine, "style");
    if (styleMap.isMap()) readStyle(styleMap, engine, style);
    Value colors = field(definition, engine, "colors");
    if (colors.isMap()) readColors(colors, engine, style);

    Value regions = field(definition, engine, "regions");
    Value parts = field(definition, engine, "parts");
    if (!regions.isMap() || !parts.isMap()) {
        error = "a skin needs =regions and =parts";
        return std::nullopt;
    }

    for (const PartName& partName : PART_NAMES) {
        Value states = field(parts, engine, partName.name);
        if (!states.isMap()) continue;
        for (const StateName& stateName : STATE_NAMES) {
            Value regionName = field(states, engine, stateName.name);
            if (regionName.isNil()) continue;

            std::string name = regionName.isString() ? std::string(regionName.asString()) : "";
            SkinRegion region;
            if (!readRegion(field(regions, engine, name.c_str()), engine, region)) {
                error = std::string("part ") + partName.name + " names region '" + name +
                        "', which isn't defined properly";
                return std::nullopt;
            }
            out.skin.setRegion(partName.part, stateName.state, region);
        }
    }
    return out;
}

std::unique_ptr<Skin> loadSkin(GuiSystem& gui, const std::filesystem::path& path, ScriptEngine& engine,
                               finescript::ExecutionContext& context, std::string& error) {
    auto* script = engine.loadScript(path);
    if (!script) {
        error = "can't read " + path.string();
        return nullptr;
    }
    auto result = engine.execute(*script, context);
    if (!result.success) {
        error = result.error;
        return nullptr;
    }

    auto definition = readSkinDefinition(result.returnValue, engine, error);
    if (!definition) return nullptr;

    std::filesystem::path directory = path.parent_path();
    std::filesystem::path atlasPath = directory / definition->atlasFile;
    if (!std::filesystem::is_regular_file(atlasPath)) {
        error = "no atlas at " + atlasPath.string();
        return nullptr;
    }

    TextureHandle atlas = gui.loadSkinAtlas(atlasPath.string());
    if (!atlas.valid()) {
        error = "can't load " + atlasPath.string();
        return nullptr;
    }

    auto skin = std::make_unique<Skin>(std::move(definition->skin));
    skin->setAtlas(atlas);

    if (!definition->fontFile.empty()) {
        std::filesystem::path fontPath = directory / definition->fontFile;
        ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), definition->fontSize);
        if (!font) {
            error = "can't load font " + fontPath.string();
            return nullptr;
        }
        skin->setFont(font);
    }
    return skin;
}

}  // namespace finegui
