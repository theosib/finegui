#pragma once

/**
 * @file skin.hpp
 * @brief Textured game skins for every ImGui widget
 *
 * A skin is an atlas of 9-slice art, a mapping from widget parts to it, and the style and
 * font that go with them. With one active, ImGui paints each widget's background from the
 * atlas through the hooks in imgui_skin_hooks.h. Parts a skin leaves out stay ImGui's own.
 * See docs/SKIN_SYSTEM.md.
 */

#include "finegui/imgui_skin_hooks.h"
#include "finegui/texture_handle.hpp"

#include <array>
#include <optional>

namespace finegui {

/// A rectangle of the atlas and the border widths that don't stretch. All in atlas pixels.
struct SkinRegion {
    float x = 0, y = 0, w = 0, h = 0;
    float left = 0, top = 0, right = 0, bottom = 0;
};

struct SkinQuad {
    ImVec2 min, max;      ///< On screen
    ImVec2 uvMin, uvMax;  ///< In the atlas, 0 to 1
};

/**
 * @brief Lay a region out over a box as up to nine quads
 *
 * Corners keep their size, edges stretch one way, the centre stretches both. A box too
 * small for its corners shrinks them evenly rather than letting them overlap. Everything is
 * cut to [clipMin, clipMax], with its UVs cut to match, so a bar can fill partway across art
 * laid out for the full box.
 *
 * @param unitsPerPixel Screen units for each atlas pixel: 0.5 for art drawn at 2x
 * @return How many quads were written to `out`
 */
[[nodiscard]] int layOutNineSlice(const SkinRegion& region, float unitsPerPixel,
                                  ImVec2 atlasSize, ImVec2 min, ImVec2 max, ImVec2 clipMin,
                                  ImVec2 clipMax, std::array<SkinQuad, 9>& out);

class Skin {
public:
    void setAtlas(TextureHandle atlas) { atlas_ = atlas; }
    void setUnitsPerPixel(float units) { unitsPerPixel_ = units; }

    void setRegion(ImGuiSkinPart part, ImGuiSkinState state, const SkinRegion& region);

    /// What to draw for a state, falling back Active to Hovered to Normal. Null if the
    /// part isn't skinned at all, which leaves it to ImGui.
    [[nodiscard]] const SkinRegion* region(ImGuiSkinPart part, ImGuiSkinState state) const;

    /// Applied whole whenever the skin becomes active
    ImGuiStyle& style() { return style_; }
    void setFont(ImFont* font) { font_ = font; }

    void draw(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state, const ImVec2& min,
              const ImVec2& max, const ImVec2& clipMin, const ImVec2& clipMax) const;

    void activate() const;

private:
    TextureHandle atlas_;
    float unitsPerPixel_ = 1.0f;
    std::array<std::array<std::optional<SkinRegion>, ImGuiSkinState_COUNT>, ImGuiSkinPart_COUNT> parts_;
    ImGuiStyle style_;
    ImFont* font_ = nullptr;
};

/// Make a skin the one in use, or pass null for plain ImGui. Must outlive its time as active.
void setActiveSkin(const Skin* skin);

}  // namespace finegui
