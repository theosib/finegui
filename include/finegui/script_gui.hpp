#pragma once

#include <finegui/widget_node.hpp>
#include <finegui/widget_converter.hpp>
#include <finescript/script_engine.h>
#include <finescript/execution_context.h>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace finegui {

class GuiRenderer;

/// A single GUI driven by a finescript script.
///
/// Owns the ExecutionContext (keeping closures alive), the widget tree
/// (via a GuiRenderer ID), and message handlers registered by the script.
///
/// Usage:
///   ScriptGui gui(engine, renderer);
///   gui.loadAndRun(R"(
///       ui.show {ui.window "Hello" [{ui.text "World"}]}
///   )");
///   // Each frame: gui.processPendingMessages(); renderer.renderAll();
///   // Deliver messages: gui.deliverMessage(sym, data);
///   // Close: gui.close();
class ScriptGui {
public:
    ScriptGui(finescript::ScriptEngine& engine, GuiRenderer& renderer);
    ~ScriptGui();

    ScriptGui(const ScriptGui&) = delete;
    ScriptGui& operator=(const ScriptGui&) = delete;
    ScriptGui(ScriptGui&&) noexcept;
    ScriptGui& operator=(ScriptGui&&) noexcept;

    /// Load and execute a script from source code.
    /// Pre-binds the given variables in the ExecutionContext before execution.
    /// The script should call ui.show to display widgets.
    /// Returns true if the script executed without errors.
    bool loadAndRun(std::string_view source, std::string_view name = "<gui>",
                    const std::vector<std::pair<std::string, finescript::Value>>& bindings = {});

    /// Execute a pre-compiled script.
    bool run(const finescript::CompiledScript& script,
             const std::vector<std::pair<std::string, finescript::Value>>& bindings = {});

    /// Deliver a message to this GUI's script (synchronous, call on GUI thread).
    /// Returns true if a handler was found and invoked.
    bool deliverMessage(uint32_t messageType, finescript::Value data);

    /// Queue a message for later delivery (thread-safe, from any thread).
    void queueMessage(uint32_t messageType, finescript::Value data);

    /// Process queued messages. Call once per frame on GUI thread.
    void processPendingMessages();

    /// Close this GUI (removes widget tree from renderer).
    void close();

    /// Check if this GUI is currently active.
    bool isActive() const;

    /// Get the GuiRenderer ID for this GUI's widget tree (-1 if not showing).
    int guiId() const;

    /// Access the underlying widget tree (for direct mutation from C++).
    WidgetNode* widgetTree();

    /// Get the execution context (for advanced usage).
    finescript::ExecutionContext* context();

    /// Get the last error message (empty if no error).
    const std::string& lastError() const;

    // -- Internal methods called by script bindings ---------------------------
    // These are public so script_bindings.cpp can call them via ctx.userData().

    /// Called by ui.show binding: convert map→WidgetNode, register with renderer.
    finescript::Value scriptShow(const finescript::Value& map,
                                 finescript::ScriptEngine& engine,
                                 finescript::ExecutionContext& ctx);

    /// Called by ui.update binding: re-convert and replace tree.
    void scriptUpdate(int id, const finescript::Value& map,
                      finescript::ScriptEngine& engine,
                      finescript::ExecutionContext& ctx);

    /// Called by ui.hide binding: remove specific tree.
    void scriptHide(int id);

    /// Called by gui.on_message binding: register a message handler.
    void registerMessageHandler(uint32_t messageType, finescript::Value handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace finegui
