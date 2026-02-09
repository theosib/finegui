#include <finegui/script_gui_manager.hpp>
#include <finegui/gui_renderer.hpp>
#include <algorithm>

namespace finegui {

ScriptGuiManager::ScriptGuiManager(finescript::ScriptEngine& engine, GuiRenderer& renderer)
    : engine_(engine), renderer_(renderer) {
}

ScriptGuiManager::~ScriptGuiManager() {
    closeAll();
}

ScriptGui* ScriptGuiManager::showFromSource(
    std::string_view source, std::string_view name,
    const std::vector<std::pair<std::string, finescript::Value>>& bindings) {

    auto gui = std::make_unique<ScriptGui>(engine_, renderer_);
    if (!gui->loadAndRun(source, name, bindings)) {
        return nullptr;
    }
    auto* ptr = gui.get();
    guis_.push_back(std::move(gui));
    return ptr;
}

bool ScriptGuiManager::deliverMessage(int guiId, uint32_t messageType,
                                       finescript::Value data) {
    auto* gui = findByGuiId(guiId);
    if (!gui) return false;
    return gui->deliverMessage(messageType, std::move(data));
}

void ScriptGuiManager::broadcastMessage(uint32_t messageType, finescript::Value data) {
    for (auto& gui : guis_) {
        if (gui->isActive()) {
            gui->deliverMessage(messageType, data);
        }
    }
}

void ScriptGuiManager::queueBroadcast(uint32_t messageType, finescript::Value data) {
    std::lock_guard<std::mutex> lock(broadcastMutex_);
    pendingBroadcasts_.push_back({messageType, std::move(data)});
}

void ScriptGuiManager::processPendingMessages() {
    // Drain broadcast queue
    std::vector<PendingBroadcast> broadcasts;
    {
        std::lock_guard<std::mutex> lock(broadcastMutex_);
        broadcasts.swap(pendingBroadcasts_);
    }
    for (auto& bc : broadcasts) {
        broadcastMessage(bc.type, std::move(bc.data));
    }

    // Process per-GUI queues
    for (auto& gui : guis_) {
        if (gui->isActive()) {
            gui->processPendingMessages();
        }
    }
}

void ScriptGuiManager::close(int guiId) {
    auto* gui = findByGuiId(guiId);
    if (gui) {
        gui->close();
    }
}

void ScriptGuiManager::closeAll() {
    for (auto& gui : guis_) {
        gui->close();
    }
}

void ScriptGuiManager::cleanup() {
    guis_.erase(
        std::remove_if(guis_.begin(), guis_.end(),
                        [](const std::unique_ptr<ScriptGui>& g) { return !g->isActive(); }),
        guis_.end());
}

ScriptGui* ScriptGuiManager::findByGuiId(int guiId) {
    for (auto& gui : guis_) {
        if (gui->guiId() == guiId) {
            return gui.get();
        }
    }
    return nullptr;
}

size_t ScriptGuiManager::activeCount() const {
    size_t count = 0;
    for (const auto& gui : guis_) {
        if (gui->isActive()) count++;
    }
    return count;
}

} // namespace finegui
