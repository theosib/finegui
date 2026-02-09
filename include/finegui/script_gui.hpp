#pragma once

#include <finescript/script_engine.h>
#include <finescript/execution_context.h>
#include <finescript/value.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace finegui {

class MapRenderer;

/// A single GUI driven by a finescript script.
///
/// Owns the ExecutionContext (keeping closures alive), the widget tree
/// (via a MapRenderer ID), and message handlers registered by the script.
/// The widget tree is stored as finescript maps — script mutations to maps
/// are automatically visible to the renderer via shared_ptr semantics.
///
/// Usage:
///   MapRenderer mapRenderer(engine);
///   ScriptGui gui(engine, mapRenderer);
///   gui.loadAndRun(R"(
///       ui.show {ui.window "Hello" [{ui.text "World"}]}
///   )");
///   // Each frame: gui.processPendingMessages(); mapRenderer.renderAll();
class ScriptGui {
public:
    ScriptGui(finescript::ScriptEngine& engine, MapRenderer& renderer);
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

    /// Get the MapRenderer ID for this GUI's widget tree (-1 if not showing).
    int guiId() const;

    /// Access the root map tree (for direct access from C++).
    /// Returns nullptr if not showing.
    finescript::Value* mapTree();

    /// Navigate the map tree to a child node.
    /// path: int (single child index) or array of ints (nested path).
    /// Returns the child map, or nil if not found.
    finescript::Value navigateMap(int guiId, const finescript::Value& path);

    /// Get the execution context (for advanced usage).
    finescript::ExecutionContext* context();

    /// Get the last error message (empty if no error).
    const std::string& lastError() const;

    // -- Internal methods called by script bindings ---------------------------
    // These are public so script_bindings.cpp can call them via ctx.userData().

    /// Called by ui.show binding: store map in MapRenderer, returns GUI ID.
    finescript::Value scriptShow(const finescript::Value& map);

    /// Called by ui.hide binding: remove tree.
    void scriptHide();

    /// Called by gui.on_message binding: register a message handler.
    void registerMessageHandler(uint32_t messageType, finescript::Value handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace finegui
