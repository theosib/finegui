/**
 * @file test_retained_render.cpp
 * @brief Integration tests for retained-mode widget rendering (requires Vulkan)
 *
 * Note: Resources are created inline per-test to avoid finevk smart pointer
 * move constructor issues (see MEMORY.md). This matches test_phase2.cpp pattern.
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/drag_drop_manager.hpp>
#include <finegui/tween_manager.hpp>

#include <finevk/finevk.hpp>

#include <iostream>
#include <cassert>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace finegui;

// Helper: run N frames with a GuiRenderer
static void runFrames(finevk::Window* window, finevk::SimpleRenderer* renderer,
                      GuiSystem& gui, GuiRenderer& guiRenderer, int count) {
    for (int i = 0; i < count && window->isOpen(); i++) {
        window->pollEvents();
        if (auto frame = renderer->beginFrame()) {
            gui.beginFrame();
            guiRenderer.renderAll();
            gui.endFrame();

            frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
            gui.render(frame);
            frame.endRenderPass();
            renderer->endFrame();
        }
    }
}

// ============================================================================
// Single comprehensive test to avoid multiple Vulkan context teardown/creation
// ============================================================================

void test_retained_rendering() {
    std::cout << "Testing: Retained-mode rendering (comprehensive)... ";

    // Create resources once
    auto instance = finevk::Instance::create()
        .applicationName("test_retained")
        .enableValidation(true)
        .build();
    auto window = finevk::Window::create(instance.get())
        .title("test_retained")
        .size(800, 600)
        .build();
    auto physicalDevice = instance->selectPhysicalDevice(window.get());
    auto device = physicalDevice.createLogicalDevice()
        .surface(window->surface())
        .build();
    window->bindDevice(device.get());
    finevk::RendererConfig rendererConfig;
    auto renderer = finevk::SimpleRenderer::create(window.get(), rendererConfig);
    GuiConfig guiConfig;
    guiConfig.msaaSamples = renderer->msaaSamples();
    GuiSystem gui(renderer->device(), guiConfig);
    gui.initialize(renderer.get());

    GuiRenderer guiRenderer(gui);

    // --- Test 1: Basic show ---
    std::cout << "\n  1. Basic show... ";
    int id1 = guiRenderer.show(WidgetNode::window("Test", {
        WidgetNode::text("Hello retained mode!"),
        WidgetNode::button("OK"),
    }));
    assert(id1 > 0);
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 2: Show second, hide first ---
    std::cout << "\n  2. Show/hide... ";
    int id2 = guiRenderer.show(WidgetNode::window("Window 2", {
        WidgetNode::text("Second"),
    }));
    assert(id1 != id2);
    assert(guiRenderer.get(id1) != nullptr);
    assert(guiRenderer.get(id2) != nullptr);

    guiRenderer.hide(id1);
    assert(guiRenderer.get(id1) == nullptr);
    assert(guiRenderer.get(id2) != nullptr);
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    std::cout << "ok";

    // --- Test 3: Update tree ---
    std::cout << "\n  3. Update tree... ";
    guiRenderer.update(id2, WidgetNode::window("Dynamic", {
        WidgetNode::text("Updated!"),
        WidgetNode::button("New Button"),
    }));
    auto* tree = guiRenderer.get(id2);
    assert(tree != nullptr);
    assert(tree->children.size() == 2);
    assert(tree->children[0].textContent == "Updated!");
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    std::cout << "ok";

    // --- Test 4: Direct mutation ---
    std::cout << "\n  4. Direct mutation... ";
    guiRenderer.hideAll();
    int id3 = guiRenderer.show(WidgetNode::window("Mutable", {
        WidgetNode::slider("Value", 0.0f, 0.0f, 1.0f),
        WidgetNode::checkbox("Toggle", false),
    }));
    tree = guiRenderer.get(id3);
    tree->children[0].floatValue = 0.75f;
    tree->children[1].boolValue = true;
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    tree = guiRenderer.get(id3);
    assert(tree->children[0].floatValue == 0.75f);
    assert(tree->children[1].boolValue == true);
    std::cout << "ok";

    // --- Test 5: All Phase 1 widgets ---
    std::cout << "\n  5. All Phase 1 widgets... ";
    guiRenderer.hideAll();
    guiRenderer.show(WidgetNode::window("All Widgets", {
        WidgetNode::text("Static text"),
        WidgetNode::button("Click me"),
        WidgetNode::checkbox("Check", false),
        WidgetNode::slider("Float slider", 0.5f, 0.0f, 1.0f),
        WidgetNode::sliderInt("Int slider", 50, 0, 100),
        WidgetNode::inputText("Text input", "hello"),
        WidgetNode::inputInt("Int input", 42),
        WidgetNode::inputFloat("Float input", 3.14f),
        WidgetNode::combo("Dropdown", {"A", "B", "C"}, 0),
        WidgetNode::separator(),
        WidgetNode::group({
            WidgetNode::text("Inside group"),
        }),
        WidgetNode::columns(2, {
            WidgetNode::text("Left column"),
            WidgetNode::text("Right column"),
        }),
    }));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);
    std::cout << "ok";

    // --- Test 6: Disabled widgets ---
    std::cout << "\n  6. Disabled widgets... ";
    guiRenderer.hideAll();
    auto btn = WidgetNode::button("Disabled");
    btn.enabled = false;
    int id4 = guiRenderer.show(WidgetNode::window("Disabled Test", {
        std::move(btn),
        WidgetNode::slider("Also disabled", 0.5f, 0.0f, 1.0f),
    }));
    tree = guiRenderer.get(id4);
    tree->children[1].enabled = false;
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 7: Hidden widgets ---
    std::cout << "\n  7. Hidden widgets... ";
    guiRenderer.hideAll();
    int id5 = guiRenderer.show(WidgetNode::window("Hidden Test", {
        WidgetNode::text("Visible text"),
        WidgetNode::text("Hidden text"),
    }));
    tree = guiRenderer.get(id5);
    tree->children[1].visible = false;
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 8: Settings panel pattern ---
    std::cout << "\n  8. Settings panel... ";
    guiRenderer.hideAll();
    guiRenderer.show(WidgetNode::window("Settings", {
        WidgetNode::text("Audio"),
        WidgetNode::slider("Volume", 0.5f, 0.0f, 1.0f),
        WidgetNode::checkbox("Mute", false),
        WidgetNode::separator(),
        WidgetNode::combo("Resolution", {"1920x1080", "2560x1440"}, 0),
        WidgetNode::separator(),
        WidgetNode::button("Apply"),
    }));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 9: Unimplemented widget placeholder ---
    std::cout << "\n  9. Placeholder for unimplemented type... ";
    guiRenderer.hideAll();
    WidgetNode tabBar;
    tabBar.type = WidgetNode::Type::TabBar;
    tabBar.label = "Tabs";
    guiRenderer.show(WidgetNode::window("Placeholder", {
        std::move(tabBar),
    }));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 10: Hide all and render empty ---
    std::cout << "\n  10. Render empty... ";
    guiRenderer.hideAll();
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    std::cout << "ok";

    // --- Test 11: Phase 9 widgets rendering ---
    std::cout << "\n  11. Phase 9 widgets... ";
    guiRenderer.hideAll();
    guiRenderer.show(WidgetNode::window("Phase 9 Test", {
        WidgetNode::separatorText("Radio Group"),
        WidgetNode::radioButton("Option A", 0, 0),
        WidgetNode::radioButton("Option B", 0, 1),
        WidgetNode::radioButton("Option C", 0, 2),
        WidgetNode::separatorText("Selectable Items"),
        WidgetNode::selectable("Item 1", false),
        WidgetNode::selectable("Item 2", true),
        WidgetNode::separator(),
        WidgetNode::inputTextMultiline("Notes", "Some text\nLine 2", 0.0f, 100.0f),
        WidgetNode::separatorText("Bullets"),
        WidgetNode::indent(20.0f),
        WidgetNode::bulletText("Point A"),
        WidgetNode::bulletText("Point B"),
        WidgetNode::unindent(20.0f),
    }, ImGuiWindowFlags_AlwaysAutoResize));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);
    std::cout << "ok";

    // --- Test 12: DnD widgets rendering ---
    std::cout << "\n  12. DnD widgets... ";
    guiRenderer.hideAll();

    DragDropManager dndManager;
    guiRenderer.setDragDropManager(&dndManager);

    // Create inventory-style DnD widgets
    auto slot1 = WidgetNode::button("Sword");
    slot1.id = "slot1";
    slot1.dragType = "item";
    slot1.dragData = "sword_01";
    slot1.dropAcceptType = "item";

    auto slot2 = WidgetNode::button("Empty");
    slot2.id = "slot2";
    slot2.dragType = "item";
    slot2.dragData = "";
    slot2.dropAcceptType = "item";
    std::string lastDroppedData;
    slot2.onDrop = [&lastDroppedData](WidgetNode& w) {
        lastDroppedData = w.dragData;
    };

    auto slot3 = WidgetNode::button("Click-only Slot");
    slot3.id = "slot3";
    slot3.dragType = "item";
    slot3.dragData = "shield_01";
    slot3.dragMode = 2;  // click-to-pick-up only

    guiRenderer.show(WidgetNode::window("Inventory DnD", {
        std::move(slot1),
        WidgetNode::sameLine(),
        std::move(slot2),
        WidgetNode::sameLine(),
        std::move(slot3),
    }, ImGuiWindowFlags_AlwaysAutoResize));

    // Render several frames (no actual mouse input, just verify no crashes)
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);

    // Also test renderCursorItem when nothing is held
    assert(!dndManager.isHolding());
    dndManager.renderCursorItem();  // should be a no-op

    // Test pick-up/drop cycle
    DragDropManager::CursorItem testItem;
    testItem.type = "item";
    testItem.data = "potion_01";
    testItem.fallbackText = "Potion";
    dndManager.pickUp(testItem);
    assert(dndManager.isHolding());
    assert(dndManager.isHolding("item"));

    auto dropped = dndManager.dropItem();
    assert(dropped.data == "potion_01");
    assert(!dndManager.isHolding());

    guiRenderer.setDragDropManager(nullptr);  // cleanup
    std::cout << "ok";

    // --- Test 13: Style push/pop rendering ---
    std::cout << "\n  13. Style push/pop... ";
    guiRenderer.hideAll();
    guiRenderer.show(WidgetNode::window("Style Test", {
        // Push red button color
        WidgetNode::pushStyleColor(ImGuiCol_Button, 0.8f, 0.1f, 0.1f, 1.0f),
        WidgetNode::pushStyleColor(ImGuiCol_ButtonHovered, 1.0f, 0.2f, 0.2f, 1.0f),
        WidgetNode::pushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f),
        WidgetNode::pushStyleVar(ImGuiStyleVar_FramePadding, 10.0f, 5.0f),
        WidgetNode::button("Red Round Button"),
        WidgetNode::popStyleVar(2),
        WidgetNode::popStyleColor(2),
        // Normal button after pop
        WidgetNode::button("Normal Button"),
    }, ImGuiWindowFlags_AlwaysAutoResize));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);
    std::cout << "ok";

    // --- Test 14: Focus management rendering ---
    std::cout << "\n  14. Focus management... ";
    guiRenderer.hideAll();

    auto focusInput = WidgetNode::inputText("Name", "");
    focusInput.id = "name_input";
    focusInput.autoFocus = true;

    auto noTabInput = WidgetNode::inputText("Skip", "");
    noTabInput.id = "skip_input";
    noTabInput.focusable = false;

    bool focusFired = false;
    auto trackedInput = WidgetNode::inputText("Tracked", "");
    trackedInput.id = "tracked_input";
    trackedInput.onFocus = [&focusFired](WidgetNode&) { focusFired = true; };
    trackedInput.onBlur = [](WidgetNode&) {};

    guiRenderer.show(WidgetNode::window("Focus Test", {
        std::move(focusInput),
        std::move(noTabInput),
        std::move(trackedInput),
    }, ImGuiWindowFlags_AlwaysAutoResize));

    // Test setFocus (just verify no crash, focus won't actually fire
    // without real input events driving ImGui's navigation)
    guiRenderer.setFocus("name_input");
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);

    guiRenderer.setFocus("tracked_input");
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    std::cout << "ok";

    // --- Test 15: Window with alpha < 1.0 ---
    std::cout << "\n  15. Window alpha... ";
    guiRenderer.hideAll();
    {
        auto win = WidgetNode::window("Alpha Test", {
            WidgetNode::text("Semi-transparent"),
            WidgetNode::button("Click me"),
        });
        win.alpha = 0.5f;
        int alphaId = guiRenderer.show(std::move(win));
        runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
        auto* alphaWin = guiRenderer.get(alphaId);
        assert(alphaWin != nullptr);
        assert(alphaWin->alpha == 0.5f);
    }
    std::cout << "ok";

    // --- Test 16: Window with explicit position ---
    std::cout << "\n  16. Window position... ";
    guiRenderer.hideAll();
    {
        auto win = WidgetNode::window("Positioned", {
            WidgetNode::text("At 100,200"),
        });
        win.windowPosX = 100.0f;
        win.windowPosY = 200.0f;
        int posId = guiRenderer.show(std::move(win));
        runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
        auto* posWin = guiRenderer.get(posId);
        assert(posWin != nullptr);
        assert(posWin->windowPosX == 100.0f);
        assert(posWin->windowPosY == 200.0f);
    }
    std::cout << "ok";

    // --- Test 17: TweenManager fadeIn ---
    std::cout << "\n  17. TweenManager fadeIn... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Fade Test", {
            WidgetNode::text("Fading in..."),
        });
        win.alpha = 0.0f;
        int fadeId = guiRenderer.show(std::move(win));

        bool fadeComplete = false;
        tweens.fadeIn(fadeId, 0.5f, Easing::Linear,
                      [&fadeComplete](int) { fadeComplete = true; });
        assert(tweens.activeCount() == 1);

        // Simulate 30 frames at ~60fps (0.5s total)
        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        auto* fadeWin = guiRenderer.get(fadeId);
        assert(fadeWin != nullptr);
        assert(fadeWin->alpha >= 0.99f);  // should be ~1.0
        assert(fadeComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 18: TweenManager slideTo ---
    std::cout << "\n  18. TweenManager slideTo... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Slide Test", {
            WidgetNode::text("Sliding..."),
        });
        win.windowPosX = 50.0f;
        win.windowPosY = 50.0f;
        int slideId = guiRenderer.show(std::move(win));

        bool slideComplete = false;
        tweens.slideTo(slideId, 300.0f, 400.0f, 0.3f, Easing::Linear,
                        [&slideComplete](int) { slideComplete = true; });
        assert(tweens.activeCount() == 2);  // PosX + PosY

        // Simulate enough frames
        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        auto* slideWin = guiRenderer.get(slideId);
        assert(slideWin != nullptr);
        assert(std::abs(slideWin->windowPosX - 300.0f) < 1.0f);
        assert(std::abs(slideWin->windowPosY - 400.0f) < 1.0f);
        assert(slideComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 19: TweenManager cancel ---
    std::cout << "\n  19. TweenManager cancel... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Cancel Test", {});
        win.alpha = 1.0f;
        int cancelId = guiRenderer.show(std::move(win));

        int tweenId = tweens.fadeOut(cancelId, 10.0f);  // very long duration
        assert(tweens.isActive(tweenId));
        assert(tweens.activeCount() == 1);

        tweens.cancel(tweenId);
        assert(!tweens.isActive(tweenId));
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 20: TweenManager shake ---
    std::cout << "\n  20. TweenManager shake... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Shake Test", {
            WidgetNode::text("Shaking!"),
        });
        win.windowPosX = 200.0f;
        win.windowPosY = 200.0f;
        int shakeGuiId = guiRenderer.show(std::move(win));

        bool shakeComplete = false;
        tweens.shake(shakeGuiId, 0.4f, 8.0f, 15.0f,
                     [&shakeComplete](int) { shakeComplete = true; });
        assert(tweens.activeCount() == 1);

        // Run frames
        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        // After shake, position should be restored to base
        auto* shakeWin = guiRenderer.get(shakeGuiId);
        assert(shakeWin != nullptr);
        assert(std::abs(shakeWin->windowPosX - 200.0f) < 1.0f);
        assert(std::abs(shakeWin->windowPosY - 200.0f) < 1.0f);
        assert(shakeComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 21: findById ---
    std::cout << "\n  21. findById... ";
    guiRenderer.hideAll();
    {
        auto hp = WidgetNode::progressBar(0.85f, 200.0f, 20.0f);
        hp.id = "hp_bar";
        auto mp = WidgetNode::progressBar(0.6f, 200.0f, 20.0f);
        mp.id = "mp_bar";

        guiRenderer.show(WidgetNode::window("HUD", {
            WidgetNode::text("Health"),
            std::move(hp),
            WidgetNode::text("Mana"),
            std::move(mp),
        }));

        // Find by ID
        auto* found = guiRenderer.findById("hp_bar");
        assert(found != nullptr);
        assert(found->type == WidgetNode::Type::ProgressBar);
        assert(found->floatValue == 0.85f);

        auto* found2 = guiRenderer.findById("mp_bar");
        assert(found2 != nullptr);
        assert(found2->floatValue == 0.6f);

        // Mutate via findById
        found->floatValue = 0.5f;
        assert(guiRenderer.findById("hp_bar")->floatValue == 0.5f);

        // Not found
        assert(guiRenderer.findById("nonexistent") == nullptr);
        assert(guiRenderer.findById("") == nullptr);

        runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    }
    std::cout << "ok";

    // --- Test 22: findById across multiple trees ---
    std::cout << "\n  22. findById multi-tree... ";
    guiRenderer.hideAll();
    {
        auto btn1 = WidgetNode::button("Action");
        btn1.id = "action_btn";
        guiRenderer.show(WidgetNode::window("Window 1", {
            std::move(btn1),
        }));

        auto btn2 = WidgetNode::button("Settings");
        btn2.id = "settings_btn";
        guiRenderer.show(WidgetNode::window("Window 2", {
            std::move(btn2),
        }));

        assert(guiRenderer.findById("action_btn") != nullptr);
        assert(guiRenderer.findById("settings_btn") != nullptr);
        assert(guiRenderer.findById("action_btn")->label == "Action");
        assert(guiRenderer.findById("settings_btn")->label == "Settings");

        runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    }
    std::cout << "ok";

    // --- Test 23: findById deeply nested ---
    std::cout << "\n  23. findById nested... ";
    guiRenderer.hideAll();
    {
        auto deep = WidgetNode::slider("Deep", 0.0f, 0.0f, 1.0f);
        deep.id = "deep_slider";

        guiRenderer.show(WidgetNode::window("Outer", {
            WidgetNode::group({
                WidgetNode::group({
                    std::move(deep),
                }),
            }),
        }));

        auto* found = guiRenderer.findById("deep_slider");
        assert(found != nullptr);
        assert(found->type == WidgetNode::Type::Slider);
        assert(found->label == "Deep");

        runFrames(window.get(), renderer.get(), gui, guiRenderer, 2);
    }
    std::cout << "ok";

    // --- Test 24: Window with scaleX/scaleY renders without crash ---
    std::cout << "\n  24. Window with scale... ";
    guiRenderer.hideAll();
    {
        auto win = WidgetNode::window("Scaled Window", {
            WidgetNode::text("Scaled content"),
            WidgetNode::button("Scaled button"),
        });
        win.scaleX = 0.5f;
        win.scaleY = 0.5f;
        guiRenderer.show(std::move(win));
        runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    }
    std::cout << "ok";

    // --- Test 25: Window with rotationY renders without crash ---
    std::cout << "\n  25. Window with rotation... ";
    guiRenderer.hideAll();
    {
        auto win = WidgetNode::window("Rotated Window", {
            WidgetNode::text("Flipped content"),
        });
        win.rotationY = 1.0f;  // ~57 degrees
        guiRenderer.show(std::move(win));
        runFrames(window.get(), renderer.get(), gui, guiRenderer, 3);
    }
    std::cout << "ok";

    // --- Test 26: TweenManager zoomIn ---
    std::cout << "\n  26. TweenManager zoomIn... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Zoom Test", {
            WidgetNode::text("Zooming in..."),
        });
        win.scaleX = 0.0f;
        win.scaleY = 0.0f;
        int zoomId = guiRenderer.show(std::move(win));

        bool zoomComplete = false;
        tweens.zoomIn(zoomId, 0.5f, Easing::Linear,
                      [&zoomComplete](int) { zoomComplete = true; });
        assert(tweens.activeCount() == 2);  // ScaleX + ScaleY

        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        auto* zoomWin = guiRenderer.get(zoomId);
        assert(zoomWin != nullptr);
        assert(zoomWin->scaleX >= 0.99f);
        assert(zoomWin->scaleY >= 0.99f);
        assert(zoomComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 27: TweenManager flipY ---
    std::cout << "\n  27. TweenManager flipY... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("Flip Test", {
            WidgetNode::text("Flipping..."),
        });
        int flipId = guiRenderer.show(std::move(win));

        bool flipComplete = false;
        tweens.flipY(flipId, 0.5f, Easing::Linear,
                     [&flipComplete](int) { flipComplete = true; });
        assert(tweens.activeCount() == 1);

        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        auto* flipWin = guiRenderer.get(flipId);
        assert(flipWin != nullptr);
        assert(std::abs(flipWin->rotationY - static_cast<float>(M_PI)) < 0.1f);
        assert(flipComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 28: TweenManager zoomOut ---
    std::cout << "\n  28. TweenManager zoomOut... ";
    guiRenderer.hideAll();
    {
        TweenManager tweens(guiRenderer);

        auto win = WidgetNode::window("ZoomOut Test", {
            WidgetNode::text("Zooming out..."),
        });
        int zoomOutId = guiRenderer.show(std::move(win));

        bool zoomOutComplete = false;
        tweens.zoomOut(zoomOutId, 0.5f, Easing::Linear,
                       [&zoomOutComplete](int) { zoomOutComplete = true; });
        assert(tweens.activeCount() == 2);

        for (int i = 0; i < 30 && window->isOpen(); i++) {
            window->pollEvents();
            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();
                tweens.update(1.0f / 60.0f);
                guiRenderer.renderAll();
                gui.endFrame();
                frame.beginRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        auto* zoomOutWin = guiRenderer.get(zoomOutId);
        assert(zoomOutWin != nullptr);
        assert(zoomOutWin->scaleX < 0.01f);
        assert(zoomOutWin->scaleY < 0.01f);
        assert(zoomOutComplete);
        assert(tweens.activeCount() == 0);
    }
    std::cout << "ok";

    // --- Test 29: Phase 12 Advanced Input widgets ---
    std::cout << "\n  29. Phase 12 Advanced Input widgets... ";
    guiRenderer.hideAll();
    guiRenderer.show(WidgetNode::window("Phase 12 Test", {
        WidgetNode::text("DragFloat3:"),
        WidgetNode::dragFloat3("Position", 1.0f, 2.0f, 3.0f, 0.1f, -10.0f, 10.0f),
        WidgetNode::separator(),
        WidgetNode::text("SliderAngle:"),
        WidgetNode::sliderAngle("Rotation", 0.0f, -180.0f, 180.0f),
        WidgetNode::separator(),
        WidgetNode::text("InputTextWithHint:"),
        WidgetNode::inputTextWithHint("Search", "Type to search...", ""),
        WidgetNode::separator(),
        WidgetNode::text("SmallButtons:"),
        WidgetNode::smallButton("Compact"),
        WidgetNode::sameLine(),
        WidgetNode::smallButton("Row"),
        WidgetNode::separator(),
        WidgetNode::text("ColorButtons:"),
        WidgetNode::colorButton("Red", 1.0f, 0.0f, 0.0f, 1.0f),
        WidgetNode::sameLine(),
        WidgetNode::colorButton("Green", 0.0f, 1.0f, 0.0f, 1.0f),
        WidgetNode::sameLine(),
        WidgetNode::colorButton("Blue", 0.0f, 0.0f, 1.0f, 1.0f),
    }, ImGuiWindowFlags_AlwaysAutoResize));
    runFrames(window.get(), renderer.get(), gui, guiRenderer, 5);
    std::cout << "ok";

    renderer->waitIdle();
    std::cout << "\nPASSED\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== finegui Retained-Mode Rendering Tests ===\n\n";

    try {
        test_retained_rendering();

        std::cout << "\n=== All retained-mode rendering tests PASSED ===\n";
    } catch (const std::exception& e) {
        std::cerr << "\nTest FAILED with exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
