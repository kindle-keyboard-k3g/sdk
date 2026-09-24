#include "kindle/input_debouncer.hpp"

namespace kindle {

InputDebouncer::InputDebouncer(const DebounceConfig& config) noexcept
    : config_(config) {}

void InputDebouncer::reset() noexcept {
    state_ = DebouncerState{};
}

bool InputDebouncer::filter_repeat(uint64_t current_time_ms) noexcept {
    if (!config_.repeat_enabled) {
        return false;
    }
    if (current_time_ms < state_.transition_ms + config_.repeat_delay_ms) {
        return false;
    }
    if (state_.last_repeat_ms == 0) {
        state_.last_repeat_ms = current_time_ms;
        return true;
    }
    if (current_time_ms < state_.last_repeat_ms + config_.repeat_interval_ms) {
        return false;
    }
    state_.last_repeat_ms = current_time_ms;
    return true;
}

bool InputDebouncer::filter_transition(const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    if (in_event.key == state_.key && in_event.type != state_.type) {
        if (current_time_ms < state_.transition_ms + config_.debounce_ms) {
            return false;
        }
    }

    state_.key = in_event.key;
    state_.type = in_event.type;
    state_.transition_ms = current_time_ms;
    state_.last_repeat_ms = 0;
    return true;
}

bool InputDebouncer::filter(const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    if (in_event.type == KeyEventType::Repeat) {
        return filter_repeat(current_time_ms);
    }
    return filter_transition(in_event, current_time_ms);
}

} // namespace kindle
