#pragma once

/**
 * @file gui_state.hpp
 * @brief State update types for message-passing GUI architecture
 */

#include <cstdint>
#include <atomic>

namespace finegui {

// ============================================================================
// Type ID generation
// ============================================================================

namespace detail {

/// Thread-safe type ID counter
inline std::atomic<uint32_t>& typeIdCounter() {
    static std::atomic<uint32_t> counter{1};
    return counter;
}

/// Generate next unique type ID
inline uint32_t nextTypeId() {
    return typeIdCounter().fetch_add(1, std::memory_order_relaxed);
}

} // namespace detail

// ============================================================================
// State update base types
// ============================================================================

/**
 * @brief Base class for all state updates
 *
 * Game logic sends state updates to GUI rather than GUI querying game state.
 * This decouples systems and enables networking/threading.
 */
struct GuiStateUpdate {
    virtual ~GuiStateUpdate() = default;

    /// Get the runtime type ID for this update
    [[nodiscard]] virtual uint32_t typeId() const = 0;
};

/**
 * @brief Type-safe state update with automatic type ID generation
 *
 * Derive your state update types from this template:
 * @code
 * struct HealthUpdate : TypedStateUpdate<HealthUpdate> {
 *     float current, max;
 * };
 * @endcode
 */
template<typename T>
struct TypedStateUpdate : GuiStateUpdate {
    /// Get the static type ID for this update type
    static uint32_t staticTypeId() {
        static uint32_t id = detail::nextTypeId();
        return id;
    }

    /// Get the runtime type ID (calls static version)
    [[nodiscard]] uint32_t typeId() const override {
        return staticTypeId();
    }
};

} // namespace finegui
