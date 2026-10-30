/**
 * @file test_skin_unit.cpp
 * @brief Skin geometry, state fallback, and definition loading (no Vulkan required)
 */

#include <finegui/skin.hpp>
#include <finegui/skin_loader.hpp>
#include <finegui/script_bindings.hpp>
#include <finescript/execution_context.h>
#include <finescript/script_engine.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace finegui;

static int failures = 0;

#define CHECK(condition) \
    do { if (!(condition)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; } } while (0)

static bool near(float a, float b) { return std::abs(a - b) < 0.001f; }

static const SkinRegion PANEL{100, 200, 64, 48, 8, 4, 16, 12};
static const ImVec2 ATLAS(1024, 512);

// Worked by hand from the numbers above, so this doesn't restate the code it checks
static void testCornersKeepTheirSizeAndEdgesStretch() {
    std::array<SkinQuad, 9> quads;
    ImVec2 min(10, 20), max(210, 120);
    int count = layOutNineSlice(PANEL, 1.0f, ATLAS, min, max, min, max, quads);
    CHECK(count == 9);

    const SkinQuad& topLeft = quads[0];
    CHECK(near(topLeft.min.x, 10) && near(topLeft.max.x, 18));
    CHECK(near(topLeft.min.y, 20) && near(topLeft.max.y, 24));
    CHECK(near(topLeft.uvMin.x, 100.0f / 1024) && near(topLeft.uvMax.x, 108.0f / 1024));
    CHECK(near(topLeft.uvMin.y, 200.0f / 512) && near(topLeft.uvMax.y, 204.0f / 512));

    const SkinQuad& centre = quads[4];
    CHECK(near(centre.min.x, 18) && near(centre.max.x, 194));
    CHECK(near(centre.min.y, 24) && near(centre.max.y, 108));
    CHECK(near(centre.uvMin.x, 108.0f / 1024) && near(centre.uvMax.x, 148.0f / 1024));

    const SkinQuad& bottomRight = quads[8];
    CHECK(near(bottomRight.min.x, 194) && near(bottomRight.max.x, 210));
    CHECK(near(bottomRight.min.y, 108) && near(bottomRight.max.y, 120));
    CHECK(near(bottomRight.uvMax.x, 164.0f / 1024) && near(bottomRight.uvMax.y, 248.0f / 512));
}

static void testArtDrawnAt2xIsHalfAsBigOnScreen() {
    std::array<SkinQuad, 9> quads;
    ImVec2 min(0, 0), max(100, 100);
    (void)layOutNineSlice(PANEL, 0.5f, ATLAS, min, max, min, max, quads);
    CHECK(near(quads[0].max.x, 4));
    CHECK(near(quads[0].max.y, 2));
    CHECK(near(quads[0].uvMax.x, 108.0f / 1024)) ;
}

static void testATinyBoxShrinksItsCornersWithoutOverlap() {
    std::array<SkinQuad, 9> quads;
    ImVec2 min(0, 0), max(12, 8);
    int count = layOutNineSlice(PANEL, 1.0f, ATLAS, min, max, min, max, quads);
    CHECK(count >= 4);
    for (int i = 0; i < count; ++i) {
        CHECK(quads[i].min.x >= -0.001f && quads[i].max.x <= 12.001f);
        CHECK(quads[i].min.y >= -0.001f && quads[i].max.y <= 8.001f);
        CHECK(quads[i].min.x < quads[i].max.x);
    }
    // Borders of 8 and 16 in a span of 12 keep their 1:2 ratio
    CHECK(near(quads[0].max.x, 4));
}

static void testClippingCutsUvsInStep() {
    // Half full: the art must not squash, so the visible half shows the left half of the texture
    std::array<SkinQuad, 9> quads;
    ImVec2 min(0, 0), max(64, 48);
    SkinRegion flat{0, 0, 64, 48, 0, 0, 0, 0};
    int count = layOutNineSlice(flat, 1.0f, ImVec2(64, 48), min, max, min, ImVec2(32, 48), quads);
    CHECK(count == 1);
    CHECK(near(quads[0].max.x, 32));
    CHECK(near(quads[0].uvMax.x, 0.5f));
    CHECK(near(quads[0].uvMax.y, 1.0f));
}

static void testNothingWhenClippedAway() {
    std::array<SkinQuad, 9> quads;
    ImVec2 min(0, 0), max(64, 48);
    CHECK(layOutNineSlice(PANEL, 1.0f, ATLAS, min, max, ImVec2(100, 100), ImVec2(200, 200), quads) == 0);
}

static void testStatesFallBackTowardNormal() {
    Skin skin;
    CHECK(skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Normal) == nullptr);

    SkinRegion normal{0, 0, 10, 10};
    SkinRegion hovered{10, 0, 10, 10};
    skin.setRegion(ImGuiSkinPart_Button, ImGuiSkinState_Normal, normal);
    CHECK(skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Active)->x == 0);
    CHECK(skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Selected)->x == 0);

    skin.setRegion(ImGuiSkinPart_Button, ImGuiSkinState_Hovered, hovered);
    CHECK(skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Active)->x == 10);
    CHECK(skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Normal)->x == 0);
}

static finescript::Value evaluate(finescript::ScriptEngine& engine, const char* source) {
    finescript::ExecutionContext context(engine);
    auto result = engine.executeCommand(source, context);
    CHECK(result.success);
    return result.returnValue;
}

static void testDefinitionsLoad() {
    finescript::ScriptEngine engine;
    ImGui::CreateContext();

    finescript::Value value = evaluate(engine,
        R"({=atlas "a.png" =units_per_pixel 0.5 =font "f.ttf" =font_size 18 )"
        R"(=regions {=btn {=x 4 =y 8 =w 60 =h 30 =inset [6 5 4 3]} =hot {=x 70 =y 8 =w 60 =h 30}} )"
        R"(=parts {=button {=normal "btn" =hovered "hot"}} )"
        R"(=style {=frame_padding [12 7] =scrollbar_size 18} )"
        R"(=colors {=Text [1 0.5 0.25 1]}})");

    std::string error;
    auto definition = readSkinDefinition(value, engine, error);
    CHECK(definition.has_value());
    if (definition) {
        CHECK(definition->atlasFile == "a.png");
        CHECK(definition->fontFile == "f.ttf");
        CHECK(near(definition->fontSize, 18));
        const SkinRegion* button = definition->skin.region(ImGuiSkinPart_Button, ImGuiSkinState_Normal);
        CHECK(button && near(button->x, 4) && near(button->left, 6) && near(button->bottom, 3));
        CHECK(definition->skin.region(ImGuiSkinPart_WindowBg, ImGuiSkinState_Normal) == nullptr);
        CHECK(near(definition->skin.style().FramePadding.x, 12));
        CHECK(near(definition->skin.style().ScrollbarSize, 18));
        CHECK(near(definition->skin.style().Colors[ImGuiCol_Text].y, 0.5f));
        CHECK(near(definition->skin.style().FrameRounding, 0));
    }

    // A typo in a region name should say so, rather than draw nothing
    finescript::Value broken = evaluate(engine, R"({=atlas "a.png" =regions {=btn {=x 1 =y 1 =w 2 =h 2}} =parts {=button {=normal "nope"}}})");
    CHECK(!readSkinDefinition(broken, engine, error).has_value());
    CHECK(error.find("nope") != std::string::npos);

    finescript::Value noAtlas = evaluate(engine, R"({=regions {=btn {=x 1 =y 1 =w 2 =h 2}} =parts {=button {=normal "btn"}}})");
    CHECK(!readSkinDefinition(noAtlas, engine, error).has_value());

    ImGui::DestroyContext();
}

int main() {
    testCornersKeepTheirSizeAndEdgesStretch();
    testArtDrawnAt2xIsHalfAsBigOnScreen();
    testATinyBoxShrinksItsCornersWithoutOverlap();
    testClippingCutsUvsInStep();
    testNothingWhenClippedAway();
    testStatesFallBackTowardNormal();
    testDefinitionsLoad();
    if (failures == 0) std::printf("All skin tests passed\n");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
