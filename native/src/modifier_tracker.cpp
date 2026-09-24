#include "kindle/modifier_tracker.hpp"

namespace kindle {

static constexpr uint8_t key_to_mod_bit(KeyCode key) noexcept {
    if (key == KeyCode::Shift) return ModifierState::MOD_SHIFT;
    if (key == KeyCode::Ctrl)  return ModifierState::MOD_CTRL;
    if (key == KeyCode::Alt)   return ModifierState::MOD_ALT;
    if (key == KeyCode::Sym)   return ModifierState::MOD_SYM;
    return 0;
}

static constexpr ModifierState mask_to_state(uint8_t mask) noexcept {
    ModifierState s{};
    s.shift = (mask & ModifierState::MOD_SHIFT) != 0;
    s.ctrl  = (mask & ModifierState::MOD_CTRL)  != 0;
    s.alt   = (mask & ModifierState::MOD_ALT)   != 0;
    s.sym   = (mask & ModifierState::MOD_SYM)   != 0;
    return s;
}

ModifierTracker::ModifierTracker(LatchMode mode) noexcept
    : mode_(mode) {}

void ModifierTracker::reset() noexcept {
    state_ = TrackerState{};
}

void ModifierTracker::sync_current() noexcept {
    uint8_t active = state_.physical_mask | state_.latched_mask | state_.locked_mask;
    state_.current = mask_to_state(active);
}

void ModifierTracker::handle_press(KeyCode key) noexcept {
    uint8_t bit = key_to_mod_bit(key);
    state_.physical_mask |= bit;

    if (mode_ == LatchMode::Lockable) {
        if (state_.locked_mask & bit) {
            state_.locked_mask &= ~bit;
            state_.latched_mask &= ~bit;
        } else if (state_.latched_mask & bit) {
            state_.locked_mask |= bit;
            state_.latched_mask &= ~bit;
        }
    }
    sync_current();
}

void ModifierTracker::handle_release(KeyCode key) noexcept {
    uint8_t bit = key_to_mod_bit(key);
    state_.physical_mask &= ~bit;

    if (mode_ == LatchMode::Disabled) {
        sync_current();
        return;
    }
    if (state_.consumed_mask & bit) {
        state_.consumed_mask &= ~bit;
        sync_current();
        return;
    }
    if (!(state_.locked_mask & bit)) {
        state_.latched_mask |= bit;
    }
    sync_current();
}

void ModifierTracker::update(KeyCode key, KeyEventType type) noexcept {
    if (key_to_mod_bit(key) == 0) return;
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
    state_.consumed_mask |= state_.physical_mask;
    state_.latched_mask = 0;
    sync_current();
}

} // namespace kindle
