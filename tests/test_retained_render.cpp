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

#include <finevk/finevk.hpp>

#include <iostream>
#include <cassert>

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
