#include <finegui/script_gui_manager.hpp>
#include <finegui/map_renderer.hpp>
#include <algorithm>

namespace finegui {

ScriptGuiManager::ScriptGuiManager(finescript::ScriptEngine& engine, MapRenderer& renderer)
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
    lifetimes_.push_back(currentLifetime());
    return ptr;
}

int ScriptGuiManager::loadUIFromValue(finescript::Value widgetTree, bool immediate) {
    if (!widgetTree.isMap()) return -1;
    finescript::ExecutionContext ctx(engine_);
    return renderer_.show(std::move(widgetTree), ctx, immediate);
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
    size_t kept = 0;
    for (size_t i = 0; i < guis_.size(); ++i) {
        if (!guis_[i]->isActive()) continue;
        guis_[kept] = std::move(guis_[i]);
        lifetimes_[kept] = lifetimes_[i];
        ++kept;
    }
    guis_.resize(kept);
    lifetimes_.resize(kept);
}

void ScriptGuiManager::endLifetime(Lifetime lifetime) {
    size_t kept = 0;
    for (size_t i = 0; i < guis_.size(); ++i) {
        if (lifetimes_[i] == lifetime) {
            guis_[i]->close();
            guis_[i].reset();
            continue;
        }
        guis_[kept] = std::move(guis_[i]);
        lifetimes_[kept] = lifetimes_[i];
        ++kept;
    }
    guis_.resize(kept);
    lifetimes_.resize(kept);
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
