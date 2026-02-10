/**
 * @file retained_demo.cpp
 * @brief Visual demo of finegui retained-mode widget system
 *
 * Shows the same kind of UI as simple_demo, but built entirely
 * through the retained-mode WidgetNode / GuiRenderer API.
 */

#include <finegui/finegui.hpp>
#include <finegui/gui_renderer.hpp>

#include <finevk/finevk.hpp>
#include <imgui.h>

#include <iostream>
#include <cmath>

int main() {
    try {
        // Create Vulkan instance
        auto instance = finevk::Instance::create()
            .applicationName("retained_demo")
            .enableValidation(true)
            .build();

        // Create window
        auto window = finevk::Window::create(instance.get())
            .title("finegui Retained-Mode Demo")
            .size(1280, 720)
            .build();

        // Select physical device and create logical device
        auto physicalDevice = instance->selectPhysicalDevice(window.get());
        auto device = physicalDevice.createLogicalDevice()
            .surface(window->surface())
            .build();

        window->bindDevice(device.get());

        // Create renderer
        finevk::RendererConfig config;
        auto renderer = finevk::SimpleRenderer::create(window.get(), config);

        // Create input manager
        auto input = finevk::InputManager::create(window.get());

        // Create GUI system with high-DPI support
        finegui::GuiConfig guiConfig;
        guiConfig.msaaSamples = renderer->msaaSamples();
        auto contentScale = window->contentScale();
        guiConfig.dpiScale = contentScale.x;
        guiConfig.fontSize = 16.0f;

        finegui::GuiSystem gui(renderer->device(), guiConfig);
        gui.initialize(renderer.get());

        // Create retained-mode renderer
        finegui::GuiRenderer guiRenderer(gui);

        // -- Build the widget trees --

        // Demo state
        int counter = 0;

        // Main demo window
        int mainId = guiRenderer.show(finegui::WidgetNode::window("Retained-Mode Demo", {
            finegui::WidgetNode::text("Welcome to finegui retained mode!"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::slider("Float Slider", 0.5f, 0.0f, 1.0f),
            finegui::WidgetNode::sliderInt("Int Slider", 50, 0, 100),
            finegui::WidgetNode::checkbox("Checkbox", false),
            finegui::WidgetNode::button("Click me!", [&counter](finegui::WidgetNode&) {
                counter++;
            }),
            finegui::WidgetNode::text("Count: 0"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::inputText("Name", "World"),
            finegui::WidgetNode::inputInt("Integer", 42),
            finegui::WidgetNode::inputFloat("Float", 3.14f),
            finegui::WidgetNode::combo("Dropdown", {"Option A", "Option B", "Option C"}, 0),
        }));

        // A second window showing columns
        guiRenderer.show(finegui::WidgetNode::window("Layout Demo", {
            finegui::WidgetNode::text("Two-column layout:"),
            finegui::WidgetNode::columns(2, {
                finegui::WidgetNode::text("Left side"),
                finegui::WidgetNode::text("Right side"),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Nested groups:"),
            finegui::WidgetNode::group({
                finegui::WidgetNode::slider("Nested Slider A", 0.3f, 0.0f, 1.0f),
                finegui::WidgetNode::slider("Nested Slider B", 0.7f, 0.0f, 1.0f),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::button("Toggle Disabled", [&guiRenderer, mainId](finegui::WidgetNode& btn) {
                auto* main = guiRenderer.get(mainId);
                if (main && main->children.size() > 2) {
                    // Toggle enabled state on the float slider in the main window
                    auto& slider = main->children[2];
                    slider.enabled = !slider.enabled;
                    btn.label = slider.enabled ? "Toggle Disabled" : "Toggle Enabled";
                }
            }),
        }));

        // Phase 3: Layout & Display showcase
        guiRenderer.show(finegui::WidgetNode::window("Phase 3: Layout & Display", {
            finegui::WidgetNode::textColored(1.0f, 0.2f, 0.2f, 1.0f, "Colored text (red)"),
            finegui::WidgetNode::textColored(0.2f, 1.0f, 0.2f, 1.0f, "Colored text (green)"),
            finegui::WidgetNode::textColored(0.4f, 0.4f, 1.0f, 1.0f, "Colored text (blue)"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::textWrapped(
                "This is wrapped text that should flow across multiple lines "
                "when the window is narrow enough. Resize this window to see it wrap."),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::textDisabled("This text is disabled/grayed out"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("SameLine demo:"),
            finegui::WidgetNode::button("A"),
            finegui::WidgetNode::sameLine(),
            finegui::WidgetNode::button("B"),
            finegui::WidgetNode::sameLine(),
            finegui::WidgetNode::button("C"),
            finegui::WidgetNode::spacing(),
            finegui::WidgetNode::progressBar(0.65f, 0.0f, 0.0f, "65%"),
            finegui::WidgetNode::progressBar(0.3f),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::collapsingHeader("Collapsing Section", {
                finegui::WidgetNode::text("This content is inside a collapsing header."),
                finegui::WidgetNode::slider("Hidden Slider", 0.5f, 0.0f, 1.0f),
            }, true),
            finegui::WidgetNode::collapsingHeader("Another Section (closed by default)", {
                finegui::WidgetNode::text("You expanded this section!"),
            }),
        }));

        // Phase 4: Containers & Menus showcase
        guiRenderer.show(finegui::WidgetNode::window("Phase 4: Containers & Menus", {
            finegui::WidgetNode::tabBar("demo_tabs", {
                finegui::WidgetNode::tabItem("Tab 1", {
                    finegui::WidgetNode::text("Content of Tab 1"),
                    finegui::WidgetNode::slider("Tab1 Slider", 0.5f, 0.0f, 1.0f),
                }),
                finegui::WidgetNode::tabItem("Tab 2", {
                    finegui::WidgetNode::text("Content of Tab 2"),
                    finegui::WidgetNode::checkbox("Tab2 Check", false),
                }),
                finegui::WidgetNode::tabItem("Tab 3", {
                    finegui::WidgetNode::text("Content of Tab 3"),
                    finegui::WidgetNode::button("Tab3 Button"),
                }),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Tree nodes:"),
            finegui::WidgetNode::treeNode("Root Node", {
                finegui::WidgetNode::treeNode("Child A", {
                    finegui::WidgetNode::treeNode("Leaf 1", {}, true, true),
                    finegui::WidgetNode::treeNode("Leaf 2", {}, true, true),
                }),
                finegui::WidgetNode::treeNode("Child B", {
                    finegui::WidgetNode::text("Some content in B"),
                }),
            }, true),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Scrollable child region:"),
            finegui::WidgetNode::child("scroll_child", 0, 100, true, false, {
                finegui::WidgetNode::text("Line 1 inside child"),
                finegui::WidgetNode::text("Line 2 inside child"),
                finegui::WidgetNode::text("Line 3 inside child"),
                finegui::WidgetNode::text("Line 4 inside child"),
                finegui::WidgetNode::text("Line 5 inside child"),
                finegui::WidgetNode::text("Line 6 inside child"),
                finegui::WidgetNode::text("Line 7 inside child"),
                finegui::WidgetNode::text("Line 8 inside child"),
            }),
        }));

        // Phase 5: Tables showcase
        guiRenderer.show(finegui::WidgetNode::window("Phase 5: Tables", {
            finegui::WidgetNode::text("Table with headers:"),
            finegui::WidgetNode::table("demo_table", 3,
                {"Name", "Value", "Status"},
                {
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Alpha"),
                        finegui::WidgetNode::text("100"),
                        finegui::WidgetNode::textColored(0.2f, 1.0f, 0.2f, 1.0f, "OK"),
                    }),
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Beta"),
                        finegui::WidgetNode::text("200"),
                        finegui::WidgetNode::textColored(1.0f, 1.0f, 0.2f, 1.0f, "Warning"),
                    }),
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Gamma"),
                        finegui::WidgetNode::text("300"),
                        finegui::WidgetNode::textColored(1.0f, 0.2f, 0.2f, 1.0f, "Error"),
                    }),
                },
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
            ),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Table with interactive widgets:"),
            finegui::WidgetNode::table("interactive_table", 2,
                {"Setting", "Control"},
                {
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Volume"),
                        finegui::WidgetNode::slider("##vol", 0.75f, 0.0f, 1.0f),
                    }),
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Enabled"),
                        finegui::WidgetNode::checkbox("##en", true),
                    }),
                    finegui::WidgetNode::tableRow({
                        finegui::WidgetNode::text("Quality"),
                        finegui::WidgetNode::sliderInt("##q", 5, 1, 10),
                    }),
                },
                ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable
            ),
        }));

        // Phase 6: Advanced Input showcase
        guiRenderer.show(finegui::WidgetNode::window("Phase 6: Advanced Input", {
            finegui::WidgetNode::text("Color editors:"),
            finegui::WidgetNode::colorEdit("Accent Color", 0.2f, 0.4f, 0.8f, 1.0f),
            finegui::WidgetNode::colorEdit("Highlight", 1.0f, 0.8f, 0.0f, 1.0f),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Color picker:"),
            finegui::WidgetNode::colorPicker("Background", 0.1f, 0.1f, 0.15f, 1.0f),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Drag inputs:"),
            finegui::WidgetNode::dragFloat("Speed", 1.5f, 0.1f, 0.0f, 10.0f),
            finegui::WidgetNode::dragFloat("Scale", 1.0f, 0.01f),
            finegui::WidgetNode::dragInt("Count", 50, 1.0f, 0, 200),
            finegui::WidgetNode::dragInt("Level", 1, 0.5f, 1, 99),
        }));

        // Phase 7: ListBox, Popup, Modal showcase
        int phase7Id = guiRenderer.show(finegui::WidgetNode::window("Phase 7: ListBox, Popup, Modal", {
            finegui::WidgetNode::text("ListBox:"),
            finegui::WidgetNode::listBox("Fruits", {"Apple", "Banana", "Cherry", "Date", "Elderberry"}, 0, 4),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Popup (right-click or use button):"),
            finegui::WidgetNode::button("Open Context Menu"),
            finegui::WidgetNode::popup("context_popup", {
                finegui::WidgetNode::text("Context Menu"),
                finegui::WidgetNode::separator(),
                finegui::WidgetNode::button("Cut"),
                finegui::WidgetNode::button("Copy"),
                finegui::WidgetNode::button("Paste"),
            }),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Modal dialog:"),
            finegui::WidgetNode::button("Open Modal"),
            finegui::WidgetNode::modal("Confirm Action", {
                finegui::WidgetNode::text("Are you sure you want to proceed?"),
                finegui::WidgetNode::separator(),
                finegui::WidgetNode::button("OK"),
                finegui::WidgetNode::button("Cancel"),
            }),
        }));

        // Wire up popup/modal open buttons
        {
            auto* p7 = guiRenderer.get(phase7Id);
            if (p7 && p7->children.size() >= 10) {
                // "Open Context Menu" button (index 4) opens popup (index 5)
                p7->children[4].onClick = [p7](finegui::WidgetNode&) {
                    p7->children[5].boolValue = true;
                };
                // "Open Modal" button (index 8) opens modal (index 9)
                p7->children[8].onClick = [p7](finegui::WidgetNode&) {
                    p7->children[9].boolValue = true;
                };
                // OK button inside modal closes it
                p7->children[9].children[2].onClick = [](finegui::WidgetNode&) {
                    ImGui::CloseCurrentPopup();
                };
                // Cancel button inside modal closes it
                p7->children[9].children[3].onClick = [](finegui::WidgetNode&) {
                    ImGui::CloseCurrentPopup();
                };
            }
        }

        // Phase 8: Canvas & Tooltip showcase
        guiRenderer.show(finegui::WidgetNode::window("Phase 8: Canvas & Tooltip", {
            finegui::WidgetNode::text("Canvas with custom drawing:"),
            finegui::WidgetNode::canvas("##demo_canvas", 300.0f, 200.0f,
                [](finegui::WidgetNode& node) {
                    ImVec2 pos = ImGui::GetItemRectMin();
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    // Draw a grid
                    for (int i = 0; i <= 6; i++) {
                        float x = pos.x + i * 50.0f;
                        dl->AddLine({x, pos.y}, {x, pos.y + 200.0f},
                                    IM_COL32(60, 60, 60, 255));
                    }
                    for (int i = 0; i <= 4; i++) {
                        float y = pos.y + i * 50.0f;
                        dl->AddLine({pos.x, y}, {pos.x + 300.0f, y},
                                    IM_COL32(60, 60, 60, 255));
                    }
                    // Draw some shapes
                    dl->AddCircleFilled({pos.x + 150.0f, pos.y + 100.0f}, 40.0f,
                                        IM_COL32(80, 120, 200, 200));
                    dl->AddTriangle({pos.x + 50.0f, pos.y + 160.0f},
                                    {pos.x + 100.0f, pos.y + 40.0f},
                                    {pos.x + 150.0f, pos.y + 160.0f},
                                    IM_COL32(200, 80, 80, 255), 2.0f);
                    dl->AddText({pos.x + 200.0f, pos.y + 30.0f},
                                IM_COL32(255, 255, 255, 255), "Canvas!");
                }),
            finegui::WidgetNode::tooltip("Custom drawing area using ImDrawList"),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Tooltips:"),
            finegui::WidgetNode::button("Hover me!"),
            finegui::WidgetNode::tooltip("Simple text tooltip"),
            finegui::WidgetNode::button("Rich tooltip"),
            finegui::WidgetNode::tooltip({
                finegui::WidgetNode::text("Rich tooltip content:"),
                finegui::WidgetNode::separator(),
                finegui::WidgetNode::textColored(0.3f, 1.0f, 0.3f, 1.0f, "Status: OK"),
                finegui::WidgetNode::progressBar(0.8f, 150.0f, 0.0f, "80%"),
            }),
        }));

        // Offscreen 3D Preview — renders to an offscreen surface and displays in GUI
        auto offscreen = finevk::OffscreenSurface::create(device.get())
            .extent(256, 256)
            .enableDepth()
            .build();

        // Initial render so the texture has valid content
        offscreen->beginFrame();
        offscreen->beginRenderPass({0.2f, 0.4f, 0.8f, 1.0f});
        offscreen->endRenderPass();
        offscreen->endFrame();

        auto texHandle = gui.registerTexture(
            offscreen->colorImageView(),
            offscreen->colorSampler(),
            256, 256);

        int previewId = guiRenderer.show(finegui::WidgetNode::window("Offscreen 3D Preview", {
            finegui::WidgetNode::text("Offscreen render target displayed as texture:"),
            finegui::WidgetNode::image(texHandle, 256.0f, 256.0f),
            finegui::WidgetNode::separator(),
            finegui::WidgetNode::text("Color cycles over time"),
        }));

        int frameCount = 0;

        std::cout << "Retained-mode demo started. Close window to exit.\n";
        if (window->isHighDPI()) {
            std::cout << "High-DPI display detected (scale: " << contentScale.x << "x)\n";
        }

        // Main loop
        while (window->isOpen()) {
            window->pollEvents();

            // Process input events
            input->update();
            finevk::InputEvent event;
            while (input->pollEvent(event)) {
                gui.processInput(finegui::InputAdapter::fromFineVK(event));
            }

            if (auto frame = renderer->beginFrame()) {
                gui.beginFrame();

                // Update counter text by mutating the tree directly
                auto* main = guiRenderer.get(mainId);
                if (main && main->children.size() > 6) {
                    main->children[6].textContent = "Count: " + std::to_string(counter);
                }

                // Re-render offscreen surface with animated colors
                {
                    float t = frameCount * 0.02f;
                    float r = 0.5f + 0.5f * std::sin(t);
                    float g = 0.5f + 0.5f * std::sin(t + 2.1f);
                    float b = 0.5f + 0.5f * std::sin(t + 4.2f);
                    offscreen->beginFrame();
                    offscreen->beginRenderPass({r, g, b, 1.0f});
                    offscreen->endRenderPass();
                    offscreen->endFrame();
                    frameCount++;
                }

                // Render all retained-mode widget trees
                guiRenderer.renderAll();

                gui.endFrame();

                frame.beginRenderPass({0.1f, 0.1f, 0.15f, 1.0f});
                gui.render(frame);
                frame.endRenderPass();
                renderer->endFrame();
            }
        }

        renderer->waitIdle();
        gui.unregisterTexture(texHandle);
        std::cout << "Retained-mode demo finished.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
