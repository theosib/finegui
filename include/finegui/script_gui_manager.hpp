#pragma once

#include <finegui/script_gui.hpp>
#include <finescript/script_engine.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>
#include <utility>

namespace finegui {

class MapRenderer;

/// Manages multiple ScriptGui instances. Provides broadcast messaging,
/// lifetime management, and a single processPendingMessages() call per frame.
class ScriptGuiManager {
public:
    ScriptGuiManager(finescript::ScriptEngine& engine, MapRenderer& renderer);
    ~ScriptGuiManager();

    /// Create and run a new ScriptGui from source code.
    /// Returns a pointer to the created GUI (owned by the manager).
    /// Returns nullptr if script execution fails.
    ScriptGui* showFromSource(std::string_view source, std::string_view name = "<gui>",
                              const std::vector<std::pair<std::string, finescript::Value>>& bindings = {});

    /// Deliver a message to a specific GUI by its renderer ID.
    bool deliverMessage(int guiId, uint32_t messageType, finescript::Value data);

    /// Broadcast a message to all active GUIs that have a handler for it.
    void broadcastMessage(uint32_t messageType, finescript::Value data);

    /// Queue a broadcast message (thread-safe, for delivery from non-GUI threads).
    void queueBroadcast(uint32_t messageType, finescript::Value data);

    /// Process all pending messages across all GUIs. Call once per frame on GUI thread.
    void processPendingMessages();

    /// Close a specific GUI by its renderer ID.
    void close(int guiId);

    /// Close all GUIs.
    void closeAll();

    /// Remove closed/inactive GUIs from the managed list.
    void cleanup();

    /// Get a GUI by its renderer ID.
    ScriptGui* findByGuiId(int guiId);

    /// Number of active GUIs.
    size_t activeCount() const;

private:
    finescript::ScriptEngine& engine_;
    MapRenderer& renderer_;
    std::vector<std::unique_ptr<ScriptGui>> guis_;

    struct PendingBroadcast {
        uint32_t type;
        finescript::Value data;
    };
    std::mutex broadcastMutex_;
    std::vector<PendingBroadcast> pendingBroadcasts_;
};

} // namespace finegui
