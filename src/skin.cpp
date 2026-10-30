#include "finegui/skin.hpp"

#include <algorithm>

ImGuiSkinDrawFn GImGuiSkinDraw = nullptr;

namespace finegui {

namespace {

const Skin* activeSkin = nullptr;

bool drawWithActiveSkin(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state,
                        const ImVec2& min, const ImVec2& max, const ImVec2& clipMin,
                        const ImVec2& clipMax) {
    if (!activeSkin->region(part, state)) return false;
    activeSkin->draw(drawList, part, state, min, max, clipMin, clipMax);
    return true;
}

/// Screen widths for two borders across a span, cut down together if they don't fit
void fitBorders(float span, float& first, float& second) {
    float total = first + second;
    if (total > span && total > 0.0f) {
        float shrink = span / total;
        first *= shrink;
        second *= shrink;
    }
}

}  // namespace

int layOutNineSlice(const SkinRegion& region, float unitsPerPixel, ImVec2 atlasSize, ImVec2 min,
                    ImVec2 max, ImVec2 clipMin, ImVec2 clipMax, std::array<SkinQuad, 9>& out) {
    float left = region.left * unitsPerPixel;
    float right = region.right * unitsPerPixel;
    float top = region.top * unitsPerPixel;
    float bottom = region.bottom * unitsPerPixel;
    fitBorders(max.x - min.x, left, right);
    fitBorders(max.y - min.y, top, bottom);

    const float xs[4] = {min.x, min.x + left, max.x - right, max.x};
    const float ys[4] = {min.y, min.y + top, max.y - bottom, max.y};
    const float us[4] = {region.x / atlasSize.x, (region.x + region.left) / atlasSize.x,
                         (region.x + region.w - region.right) / atlasSize.x,
                         (region.x + region.w) / atlasSize.x};
    const float vs[4] = {region.y / atlasSize.y, (region.y + region.top) / atlasSize.y,
                         (region.y + region.h - region.bottom) / atlasSize.y,
                         (region.y + region.h) / atlasSize.y};

    int count = 0;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            float x0 = xs[column], x1 = xs[column + 1];
            float y0 = ys[row], y1 = ys[row + 1];
            float cutX0 = std::max(x0, clipMin.x), cutX1 = std::min(x1, clipMax.x);
            float cutY0 = std::max(y0, clipMin.y), cutY1 = std::min(y1, clipMax.y);
            if (cutX0 >= cutX1 || cutY0 >= cutY1) continue;

            float u0 = us[column], u1 = us[column + 1];
            float v0 = vs[row], v1 = vs[row + 1];
            float perX = (u1 - u0) / (x1 - x0);
            float perY = (v1 - v0) / (y1 - y0);
            out[count++] = SkinQuad{ImVec2(cutX0, cutY0), ImVec2(cutX1, cutY1),
                                    ImVec2(u0 + (cutX0 - x0) * perX, v0 + (cutY0 - y0) * perY),
                                    ImVec2(u0 + (cutX1 - x0) * perX, v0 + (cutY1 - y0) * perY)};
        }
    }
    return count;
}

void Skin::setRegion(ImGuiSkinPart part, ImGuiSkinState state, const SkinRegion& region) {
    parts_[part][state] = region;
}

const SkinRegion* Skin::region(ImGuiSkinPart part, ImGuiSkinState state) const {
    const auto& states = parts_[part];
    if (states[state]) return &*states[state];
    if (state == ImGuiSkinState_Active && states[ImGuiSkinState_Hovered]) {
        return &*states[ImGuiSkinState_Hovered];
    }
    return states[ImGuiSkinState_Normal] ? &*states[ImGuiSkinState_Normal] : nullptr;
}

void Skin::draw(ImDrawList* drawList, ImGuiSkinPart part, ImGuiSkinState state, const ImVec2& min,
                const ImVec2& max, const ImVec2& clipMin, const ImVec2& clipMax) const {
    const SkinRegion* art = region(part, state);
    if (!art) return;

    std::array<SkinQuad, 9> quads;
    ImVec2 atlasSize(static_cast<float>(atlas_.width), static_cast<float>(atlas_.height));
    int count = layOutNineSlice(*art, unitsPerPixel_, atlasSize, min, max, clipMin, clipMax, quads);

    // Through GetColorU32 so disabled and fading widgets dim the art along with their text
    ImU32 tint = ImGui::GetColorU32(IM_COL32_WHITE);
    for (int i = 0; i < count; ++i) {
        drawList->AddImage(atlas_, quads[i].min, quads[i].max, quads[i].uvMin, quads[i].uvMax, tint);
    }
}

void Skin::activate() const {
    ImGui::GetStyle() = style_;
    if (font_) ImGui::GetIO().FontDefault = font_;
}

void setActiveSkin(const Skin* skin) {
    activeSkin = skin;
    GImGuiSkinDraw = skin ? drawWithActiveSkin : nullptr;
    if (skin) skin->activate();
}

}  // namespace finegui
