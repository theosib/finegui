/**
 * @file test_lifetime_unit.cpp
 * @brief Lifetimes: what's shown and loaded under one goes when it ends (no Vulkan required)
 */

#include <finegui/drag_drop_manager.hpp>
#include <finegui/hotkey_manager.hpp>
#include <finegui/lifetime.hpp>
#include <finegui/map_renderer.hpp>
#include <finegui/script_bindings.hpp>
#include <finegui/script_gui_manager.hpp>
#include <finegui/texture_registry.hpp>
#include <finescript/execution_context.h>
#include <finescript/map_data.h>
#include <finescript/native_function.h>
#include <finescript/script_engine.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace finegui;
using namespace finescript;

namespace {

int failures = 0;

void check(bool ok, const char* what, int line) {
    if (!ok) {
        std::cout << "\n  FAILED line " << line << ": " << what;
        ++failures;
    }
}
#define CHECK(x) check((x), #x, __LINE__)

constexpr Lifetime MENUS = 1;
constexpr Lifetime SESSION = 2;

Value window(ScriptEngine& engine, const std::string& title, std::vector<Value> children = {}) {
    Value w = Value::map();
    w.asMap().set(engine.intern("type"), Value::symbol(engine.intern("window")));
    w.asMap().set(engine.intern("title"), Value::string(title));
    w.asMap().set(engine.intern("children"), Value::array(std::move(children)));
    return w;
}

ImGuiContext* headlessImGui() {
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    io.Fonts->SetTexID(static_cast<ImTextureID>(1));
    return ctx;
}

void frame(MapRenderer& renderer) {
    ImGui::NewFrame();
    renderer.renderAll();
    ImGui::EndFrame();
}

}  // namespace

void test_current_lifetime_and_scope() {
    std::cout << "Testing: current lifetime, and a scope puts the last one back... ";
    CHECK(currentLifetime() == FOREVER);
    setCurrentLifetime(MENUS);
    {
        LifetimeScope scope(SESSION);
        CHECK(currentLifetime() == SESSION);
    }
    CHECK(currentLifetime() == MENUS);
    setCurrentLifetime(FOREVER);
    std::cout << "done\n";
}

void test_texture_names() {
    std::cout << "Testing: texture names go with their lifetime... ";
    TextureRegistry registry;
    registry.registerTexture("logo", TextureHandle{1, 8, 8});
    setCurrentLifetime(MENUS);
    registry.registerTexture("title_art", TextureHandle{2, 8, 8});
    setCurrentLifetime(SESSION);
    registry.registerTexture("hud", TextureHandle{3, 8, 8});
    setCurrentLifetime(FOREVER);

    endLifetime(MENUS);
    CHECK(registry.has("logo"));
    CHECK(!registry.has("title_art"));
    CHECK(registry.has("hud"));
    endLifetime(SESSION);
    CHECK(!registry.has("hud"));
    CHECK(registry.get("logo").id == 1);
    CHECK(registry.size() == 1);
    std::cout << "done\n";
}

void test_hotkeys_and_carried_item() {
    std::cout << "Testing: hotkeys and a carried item go with their lifetime... ";
    HotkeyManager hotkeys;
    DragDropManager dnd;
    hotkeys.bind(ImGuiKey_F1, [] {});
    setCurrentLifetime(SESSION);
    hotkeys.bind(ImGuiKey_F2, [] {});
    hotkeys.bind(ImGuiKey_F3, [] {});
    DragDropManager::CursorItem item;
    item.type = "item";
    dnd.pickUp(item);
    setCurrentLifetime(FOREVER);
    CHECK(hotkeys.bindingCount() == 3);
    CHECK(dnd.isHolding());

    endLifetime(MENUS);  // Nothing of this one
    CHECK(hotkeys.bindingCount() == 3);
    CHECK(dnd.isHolding());

    endLifetime(SESSION);
    CHECK(hotkeys.bindingCount() == 1);
    CHECK(!dnd.isHolding());

    // An item picked up under FOREVER stays through a session's end
    dnd.pickUp(item);
    endLifetime(SESSION);
    CHECK(dnd.isHolding());
    std::cout << "done\n";
}

void test_trees_go_with_their_lifetime() {
    std::cout << "Testing: map trees go with the lifetime they were shown in... ";
    ScriptEngine engine;
    registerGuiBindings(engine);
    ExecutionContext ctx(engine);
    MapRenderer renderer(engine);

    int always = renderer.show(window(engine, "Always"), ctx, true);
    setCurrentLifetime(MENUS);
    int title = renderer.show(window(engine, "Title"), ctx, true);
    int staged = renderer.stage(window(engine, "Options"), ctx);
    setCurrentLifetime(SESSION);
    int hud = renderer.show(window(engine, "HUD"), ctx, true);
    setCurrentLifetime(FOREVER);

    endLifetime(MENUS);
    CHECK(renderer.get(always) != nullptr);
    CHECK(renderer.get(title) == nullptr);
    CHECK(renderer.get(staged) == nullptr);
    CHECK(renderer.get(hud) != nullptr);
    endLifetime(SESSION);
    CHECK(renderer.get(hud) == nullptr);
    CHECK(renderer.get(always) != nullptr);
    std::cout << "done\n";
}

void test_imgui_forgets_the_windows() {
    std::cout << "Testing: ImGui forgets where a lifetime's windows were... ";
    ImGuiContext* imgui = headlessImGui();
    ScriptEngine engine;
    registerGuiBindings(engine);
    ExecutionContext ctx(engine);
    MapRenderer renderer(engine);

    renderer.show(window(engine, "Kept"), ctx, true);
    setCurrentLifetime(SESSION);
    renderer.show(window(engine, "Inventory"), ctx, true);
    setCurrentLifetime(FOREVER);
    frame(renderer);
    frame(renderer);

    // The player moved both windows and opened something in one
    ImGui::SetWindowPos("Inventory", ImVec2(300.0f, 200.0f));
    ImGui::SetWindowPos("Kept", ImVec2(310.0f, 210.0f));
    ImGuiWindow* inventory = ImGui::FindWindowByName("Inventory");
    ImGuiWindow* kept = ImGui::FindWindowByName("Kept");
    CHECK(inventory && kept);
    inventory->StateStorage.SetInt(ImHashStr("open_node"), 1);
    CHECK(inventory->Pos.x == 300.0f);

    endLifetime(SESSION);
    CHECK(inventory->Pos.x != 300.0f);  // Back where a new window goes
    CHECK(inventory->StateStorage.GetInt(ImHashStr("open_node"), 0) == 0);
    CHECK(kept->Pos.x == 310.0f);

    ImGui::DestroyContext(imgui);
    std::cout << "done\n";
}

void test_ending_a_lifetime_from_its_own_button() {
    std::cout << "Testing: a lifetime ended by its own button goes when the frame ends... ";
    ImGuiContext* imgui = headlessImGui();
    ImGuiIO& io = ImGui::GetIO();
    ScriptEngine engine;
    registerGuiBindings(engine);
    ExecutionContext ctx(engine);
    MapRenderer renderer(engine);

    int session = -1, during = -1, menu = -1;
    bool clicked = false;
    Value button = Value::map();
    button.asMap().set(engine.intern("type"), Value::symbol(engine.intern("button")));
    button.asMap().set(engine.intern("label"), Value::string("Quit to Title"));
    button.asMap().set(engine.intern("width"), Value::number(300.0));
    button.asMap().set(engine.intern("height"), Value::number(200.0));
    button.asMap().set(engine.intern("on_click"),
        Value::nativeFunction(std::make_shared<SimpleLambdaFunction>(
            [&](ExecutionContext& cbCtx, const std::vector<Value>&) -> Value {
                clicked = true;
                // Shown under the session just before it ends: never drawn
                {
                    LifetimeScope scope(SESSION);
                    during = renderer.show(window(engine, "Late"), cbCtx, true);
                }
                endLifetime(SESSION);
                CHECK(renderer.get(session) == nullptr);
                CHECK(renderer.get(during) == nullptr);
                // The title screen comes up in its own lifetime, in the same frame
                LifetimeScope scope(MENUS);
                menu = renderer.show(window(engine, "Title"), cbCtx, true);
                return Value::nil();
            })));
    Value w = window(engine, "Game Menu", {button});
    w.asMap().set(engine.intern("window_pos_x"), Value::number(0.0));
    w.asMap().set(engine.intern("window_pos_y"), Value::number(0.0));
    {
        LifetimeScope scope(SESSION);
        session = renderer.show(w, ctx, true);
    }

    frame(renderer);
    io.AddMousePosEvent(50.0f, 50.0f);
    frame(renderer);
    io.AddMouseButtonEvent(0, true);
    frame(renderer);
    io.AddMouseButtonEvent(0, false);
    frame(renderer);
    CHECK(clicked);
    CHECK(renderer.get(session) == nullptr);
    CHECK(renderer.get(during) == nullptr);
    CHECK(renderer.get(menu) != nullptr);
    frame(renderer);
    CHECK(ImGui::FindWindowByName("Title") != nullptr);

    ImGui::DestroyContext(imgui);
    std::cout << "done\n";
}

void test_script_guis_go_with_their_lifetime() {
    std::cout << "Testing: script GUIs are closed and freed with their lifetime... ";
    ScriptEngine engine;
    registerGuiBindings(engine);
    MapRenderer renderer(engine);
    ScriptGuiManager manager(engine, renderer);

    CHECK(manager.showFromSource("ui.show {ui.window \"Kept\" []}") != nullptr);
    ScriptGui* gone = nullptr;
    {
        LifetimeScope scope(MENUS);
        gone = manager.showFromSource("ui.show {ui.window \"Title\" []}");
    }
    CHECK(gone != nullptr);
    int goneId = gone->guiId();
    CHECK(manager.activeCount() == 2);

    endLifetime(MENUS);
    CHECK(manager.activeCount() == 1);
    CHECK(manager.findByGuiId(goneId) == nullptr);
    CHECK(renderer.get(goneId) == nullptr);
    std::cout << "done\n";
}

void test_holders_leave_the_list() {
    std::cout << "Testing: a destroyed holder is off the list... ";
    {
        TextureRegistry gone;
        LifetimeScope scope(MENUS);
        gone.registerTexture("x", TextureHandle{9, 1, 1});
    }
    endLifetime(MENUS);  // Would touch freed memory if it were still listed

    // A holder that destroys another while its lifetime ends
    struct Owner : LifetimeHolder {
        std::unique_ptr<TextureRegistry> owned = std::make_unique<TextureRegistry>();
        void endLifetime(Lifetime) override { owned.reset(); }
    } owner;
    endLifetime(SESSION);
    CHECK(owner.owned == nullptr);
    std::cout << "done\n";
}

int main() {
    std::cout << "=== finegui Lifetime Unit Tests ===\n\n";
    test_current_lifetime_and_scope();
    test_texture_names();
    test_hotkeys_and_carried_item();
    test_trees_go_with_their_lifetime();
    test_imgui_forgets_the_windows();
    test_ending_a_lifetime_from_its_own_button();
    test_script_guis_go_with_their_lifetime();
    test_holders_leave_the_list();
    if (failures) {
        std::cout << "\n" << failures << " check(s) FAILED\n";
        return 1;
    }
    std::cout << "\nAll lifetime tests passed.\n";
    return 0;
}
