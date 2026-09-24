#include "kindle/input_debouncer.hpp"

namespace kindle {

InputDebouncer::InputDebouncer(const DebounceConfig& config) noexcept
    : config_(config) {}

void InputDebouncer::reset() noexcept {
    keys_.fill(KeyDebounceState{});
}

KeyDebounceState* InputDebouncer::find_or_allocate(KeyCode key) noexcept {
    for (auto& k : keys_) {
        if (k.key == key) return &k;
    }
    for (auto& k : keys_) {
        if (k.key == KeyCode::Unknown && !k.has_pending_release) {
            k.key = key;
            return &k;
        }
    }
    KeyDebounceState* oldest = nullptr;
    for (auto& k : keys_) {
        if (k.emitted_type == KeyEventType::Release && !k.has_pending_release) {
            if (!oldest || k.transition_ms < oldest->transition_ms) oldest = &k;
        }
    }
    if (oldest) {
        *oldest = KeyDebounceState{};
        oldest->key = key;
        return oldest;
    }
    return &keys_[0];
}

bool InputDebouncer::filter_repeat(KeyDebounceState& key_state, uint64_t current_time_ms) noexcept {
    if (!config_.repeat_enabled || key_state.emitted_type != KeyEventType::Press) return false;
    if (current_time_ms < key_state.press_ms + config_.repeat_delay_ms) return false;
    if (key_state.last_repeat_ms == 0) {
        key_state.last_repeat_ms = current_time_ms;
        return true;
    }
    if (current_time_ms < key_state.last_repeat_ms + config_.repeat_interval_ms) return false;
    key_state.last_repeat_ms = current_time_ms;
    return true;
}

bool InputDebouncer::filter_press(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    if (key_state.emitted_type == KeyEventType::Press) {
        key_state.has_pending_release = false;
        return false;
    }
    key_state.has_pending_release = false;
    if (key_state.transition_ms > 0 && current_time_ms < key_state.transition_ms + config_.debounce_ms) {
        return false;
    }
    key_state.emitted_type = KeyEventType::Press;
    key_state.press_ms = current_time_ms;
    key_state.transition_ms = current_time_ms;
    key_state.last_repeat_ms = 0;
    key_state.raw_code = in_event.raw_code;
    return true;
}

bool InputDebouncer::filter_release(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    if (key_state.emitted_type == KeyEventType::Release) return false;
    if (current_time_ms < key_state.transition_ms + config_.debounce_ms) {
        key_state.has_pending_release = true;
        key_state.pending_release_ms = key_state.transition_ms + config_.debounce_ms;
        key_state.raw_code = in_event.raw_code;
        return false;
    }
    key_state.has_pending_release = false;
    key_state.emitted_type = KeyEventType::Release;
    key_state.transition_ms = current_time_ms;
    key_state.last_repeat_ms = 0;
    return true;
}

bool InputDebouncer::filter_transition(KeyDebounceState& key_state, const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    if (in_event.type == KeyEventType::Press) {
        return filter_press(key_state, in_event, current_time_ms);
    }
    return filter_release(key_state, in_event, current_time_ms);
}

bool InputDebouncer::filter(const InputEvent& in_event, uint64_t current_time_ms) noexcept {
    KeyDebounceState* entry = find_or_allocate(in_event.key);
    if (in_event.type == KeyEventType::Repeat) {
        return filter_repeat(*entry, current_time_ms);
    }
    return filter_transition(*entry, in_event, current_time_ms);
}

bool InputDebouncer::poll_pending(uint64_t current_time_ms, InputEvent& out_event) noexcept {
    for (auto& k : keys_) {
        if (!k.has_pending_release) continue;
        if (current_time_ms >= k.pending_release_ms) {
            k.has_pending_release = false;
            k.emitted_type = KeyEventType::Release;
            k.transition_ms = current_time_ms;
            k.last_repeat_ms = 0;
            out_event = InputEvent{k.key, KeyEventType::Release, k.raw_code, current_time_ms * 1000ULL};
            return true;
        }
    }
    return false;
}

} // namespace kindle
