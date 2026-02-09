#include <finegui/script_gui.hpp>
#include <finegui/gui_renderer.hpp>
#include <finegui/widget_converter.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace finegui {

// -- Impl ---------------------------------------------------------------------

struct ScriptGui::Impl {
    finescript::ScriptEngine& engine;
    GuiRenderer& renderer;
    ConverterSymbols syms;
    std::unique_ptr<finescript::ExecutionContext> ctx;
    int guiId = -1;
    std::string lastError;

    // Message handlers: symbol ID → closure Value
    std::unordered_map<uint32_t, finescript::Value> messageHandlers;

    // Thread-safe message queue
    struct PendingMessage {
        uint32_t type;
        finescript::Value data;
    };
    std::mutex messageMutex;
    std::vector<PendingMessage> pendingMessages;

    Impl(finescript::ScriptEngine& e, GuiRenderer& r)
        : engine(e), renderer(r) {
        syms.intern(e);
    }
};

// -- Construction / destruction -----------------------------------------------

ScriptGui::ScriptGui(finescript::ScriptEngine& engine, GuiRenderer& renderer)
    : impl_(std::make_unique<Impl>(engine, renderer)) {
}

ScriptGui::~ScriptGui() {
    if (impl_ && impl_->guiId >= 0) {
        // Remove tree from renderer first (closure safety)
        impl_->renderer.hide(impl_->guiId);
        impl_->guiId = -1;
    }
    // Then Impl destructor destroys ctx → frees closures
}

ScriptGui::ScriptGui(ScriptGui&&) noexcept = default;
ScriptGui& ScriptGui::operator=(ScriptGui&&) noexcept = default;

// -- Script execution ---------------------------------------------------------

bool ScriptGui::loadAndRun(std::string_view source, std::string_view name,
                           const std::vector<std::pair<std::string, finescript::Value>>& bindings) {
    impl_->lastError.clear();

    // Parse the source
    std::unique_ptr<finescript::CompiledScript> script;
    try {
        script = impl_->engine.parseString(source, name);
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        return false;
    }

    return run(*script, bindings);
}

bool ScriptGui::run(const finescript::CompiledScript& script,
                    const std::vector<std::pair<std::string, finescript::Value>>& bindings) {
    impl_->lastError.clear();

    // Create a fresh execution context
    impl_->ctx = std::make_unique<finescript::ExecutionContext>(impl_->engine);
    impl_->ctx->setUserData(this);

    // Pre-bind variables
    for (const auto& [name, value] : bindings) {
        impl_->ctx->set(name, value);
    }

    // Execute the script
    auto result = impl_->engine.execute(script, *impl_->ctx);
    if (!result.success) {
        impl_->lastError = result.error;
        return false;
    }

    // Collect event handlers registered via 'on :event do...end'
    for (const auto& handler : impl_->ctx->eventHandlers()) {
        impl_->messageHandlers[handler.eventSymbol] = handler.handlerFunction;
    }

    return true;
}

// -- Message delivery ---------------------------------------------------------

bool ScriptGui::deliverMessage(uint32_t messageType, finescript::Value data) {
    auto it = impl_->messageHandlers.find(messageType);
    if (it == impl_->messageHandlers.end()) {
        return false;
    }

    try {
        impl_->engine.callFunction(it->second, {std::move(data)}, *impl_->ctx);
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        return false;
    }

    return true;
}

void ScriptGui::queueMessage(uint32_t messageType, finescript::Value data) {
    std::lock_guard<std::mutex> lock(impl_->messageMutex);
    impl_->pendingMessages.push_back({messageType, std::move(data)});
}

void ScriptGui::processPendingMessages() {
    std::vector<Impl::PendingMessage> messages;
    {
        std::lock_guard<std::mutex> lock(impl_->messageMutex);
        messages.swap(impl_->pendingMessages);
    }
    for (auto& msg : messages) {
        deliverMessage(msg.type, std::move(msg.data));
    }
}

// -- Lifecycle ----------------------------------------------------------------

void ScriptGui::close() {
    if (impl_->guiId >= 0) {
        impl_->renderer.hide(impl_->guiId);
        impl_->guiId = -1;
    }
}

bool ScriptGui::isActive() const {
    return impl_->guiId >= 0;
}

int ScriptGui::guiId() const {
    return impl_->guiId;
}

WidgetNode* ScriptGui::widgetTree() {
    if (impl_->guiId < 0) return nullptr;
    return impl_->renderer.get(impl_->guiId);
}

finescript::ExecutionContext* ScriptGui::context() {
    return impl_->ctx.get();
}

const std::string& ScriptGui::lastError() const {
    return impl_->lastError;
}

// -- Script binding helpers ---------------------------------------------------

finescript::Value ScriptGui::scriptShow(const finescript::Value& map,
                                        finescript::ScriptEngine& engine,
                                        finescript::ExecutionContext& ctx) {
    auto widgetTree = convertToWidget(map, engine, ctx, impl_->syms);
    if (impl_->guiId < 0) {
        impl_->guiId = impl_->renderer.show(std::move(widgetTree));
    } else {
        impl_->renderer.update(impl_->guiId, std::move(widgetTree));
    }
    return finescript::Value::integer(impl_->guiId);
}

void ScriptGui::scriptUpdate(int id, const finescript::Value& map,
                              finescript::ScriptEngine& engine,
                              finescript::ExecutionContext& ctx) {
    auto widgetTree = convertToWidget(map, engine, ctx, impl_->syms);
    impl_->renderer.update(id, std::move(widgetTree));
}

void ScriptGui::scriptHide(int id) {
    impl_->renderer.hide(id);
    if (id == impl_->guiId) {
        impl_->guiId = -1;
    }
}

void ScriptGui::registerMessageHandler(uint32_t messageType, finescript::Value handler) {
    impl_->messageHandlers[messageType] = std::move(handler);
}

} // namespace finegui
