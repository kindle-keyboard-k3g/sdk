#pragma once
#include "kindle/input.hpp"
#include <array>
#include <cstdint>
#include <cstddef>

namespace kindle {

/**
 * Configuration parameters for input event debouncing and auto-repeat regulation.
 */
struct DebounceConfig {
    uint32_t debounce_ms{20};        ///< Mechanical chatter suppression window in milliseconds
    bool     repeat_enabled{true};   ///< Whether auto-repeat events are processed or suppressed
    uint32_t repeat_delay_ms{400};   ///< Initial hold delay before repeat events start firing
    uint32_t repeat_interval_ms{80}; ///< Interval between successive auto-repeat events
};

/**
 * Internal state for an individual tracked key.
 */
struct KeyDebounceState {
    KeyCode      key{KeyCode::Unknown};
    KeyEventType emitted_type{KeyEventType::Release};
    uint64_t     transition_ms{0};
    uint64_t     press_ms{0};
    uint64_t     last_repeat_ms{0};
    bool         has_pending_release{false};
    uint64_t     pending_release_ms{0};
    uint16_t     raw_code{0};
};

/**
 * Filters raw input events to suppress mechanical contact bounce (<20ms)
 * and regulate or drop kernel auto-repeat flooding for E-Ink displays.
 * Tracks per-key state in a fixed-size table with zero heap allocation.
 */
class InputDebouncer {
public:
    explicit InputDebouncer(const DebounceConfig& config = {}) noexcept;

    /**
     * Filters raw input events.
     *
     * @param in_event Raw decoded input event to filter.
     * @param current_time_ms Current timestamp in milliseconds.
     * @return true if event is valid and accepted; false if suppressed as bounce or filtered repeat.
     */
    bool filter(const InputEvent& in_event, uint64_t current_time_ms) noexcept;

    /**
     * Checks if any pending release event has matured past the debounce window.
     *
     * @param current_time_ms Current timestamp in milliseconds.
     * @param out_event Output parameter filled with the pending release event if matured.
     * @return true if a pending release was emitted; false otherwise.
     */
    bool poll_pending(uint64_t current_time_ms, InputEvent& out_event) noexcept;

    /**
     * Resets all internal key states and timestamps.
     */
    void reset() noexcept;

private:
    static constexpr size_t MAX_TRACKED_KEYS = 8;

    KeyDebounceState* find_or_allocate(KeyCode key) noexcept;
    bool filter_repeat(KeyDebounceState& key_state, uint64_t current_time_ms) noexcept;
    bool filter_press(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept;
    bool filter_release(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept;
    bool filter_transition(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept;

    DebounceConfig config_;
    std::array<KeyDebounceState, MAX_TRACKED_KEYS> keys_{};
};

} // namespace kindle
