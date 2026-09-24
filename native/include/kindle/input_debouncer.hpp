#pragma once
#include "kindle/input.hpp"
#include <cstdint>

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
 * Internal state for key transitions and repeat timing.
 */
struct DebouncerState {
    KeyCode      key{KeyCode::Unknown};
    KeyEventType type{KeyEventType::Release};
    uint64_t     transition_ms{0};
    uint64_t     last_repeat_ms{0};
};

/**
 * Filters raw input events to suppress mechanical contact bounce (<20ms)
 * and regulate or drop kernel auto-repeat flooding for E-Ink displays.
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
     * Resets all internal transition and repeat timestamps.
     */
    void reset() noexcept;

private:
    bool filter_repeat(uint64_t current_time_ms) noexcept;
    bool filter_transition(const InputEvent& in_event, uint64_t current_time_ms) noexcept;

    DebounceConfig config_;
    DebouncerState state_{};
};

} // namespace kindle
