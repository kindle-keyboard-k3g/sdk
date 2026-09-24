#include "kindle/modifier_tracker.hpp"

namespace kindle {

static constexpr bool is_modifier_key(KeyCode key) noexcept {
    return key == KeyCode::Shift || key == KeyCode::Alt ||
           key == KeyCode::Ctrl  || key == KeyCode::Sym;
}

ModifierTracker::ModifierTracker(LatchMode mode) noexcept
    : mode_(mode) {}

void ModifierTracker::reset() noexcept {
    state_ = TrackerState{};
}

void ModifierTracker::set_modifier_flag(ModifierState& s, KeyCode key, bool val) noexcept {
    if (key == KeyCode::Shift) { s.shift = val; return; }
    if (key == KeyCode::Ctrl)  { s.ctrl = val; return; }
    if (key == KeyCode::Alt)   { s.alt = val; return; }
    if (key == KeyCode::Sym)   { s.sym = val; return; }
}

void ModifierTracker::handle_press(KeyCode key) noexcept {
    set_modifier_flag(state_.physical, key, true);
    set_modifier_flag(state_.current, key, true);
    if (mode_ == LatchMode::Lockable && state_.latched) {
        state_.locked = true;
    }
}

void ModifierTracker::handle_release(KeyCode key) noexcept {
    set_modifier_flag(state_.physical, key, false);
    if (mode_ == LatchMode::Disabled) {
        set_modifier_flag(state_.current, key, false);
        return;
    }
    if (state_.consumed_while_held) {
        state_.consumed_while_held = false;
        set_modifier_flag(state_.current, key, false);
        return;
    }
    if (!state_.locked) {
        state_.latched = true;
    }
}

void ModifierTracker::update(KeyCode key, KeyEventType type) noexcept {
    if (!is_modifier_key(key)) {
        return;
    }
    if (type == KeyEventType::Press) {
        handle_press(key);
        return;
    }
    if (type == KeyEventType::Release) {
        handle_release(key);
        return;
    }
}

void ModifierTracker::consume_latch() noexcept {
    if (state_.physical.to_mask() != 0) {
        state_.consumed_while_held = true;
    }
    if (!state_.latched) {
        return;
    }
    state_.latched = false;
    if (state_.locked) {
        return;
    }
    state_.current = state_.physical;
}

} // namespace kindle
