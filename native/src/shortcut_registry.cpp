#include "kindle/shortcut_registry.hpp"

namespace kindle {

ShortcutRegistry::ShortcutRegistry() noexcept {
    register_defaults();
}

void ShortcutRegistry::register_defaults() noexcept {
    count_ = 0;
    bind_action(ModifierState::MOD_ALT, KeyCode::G, ShortcutAction::Ghostbuster);
    bind_action(0, KeyCode::VolumeUp, ShortcutAction::VolumeUp);
    bind_action(0, KeyCode::VolumeDown, ShortcutAction::VolumeDown);
}

void ShortcutRegistry::reset() noexcept {
    register_defaults();
}

bool ShortcutRegistry::bind_action(uint8_t mod_mask, KeyCode key, ShortcutAction action) noexcept {
    for (size_t i = 0; i < count_; ++i) {
        if (bindings_[i].mod_mask == mod_mask && bindings_[i].key == key) {
            bindings_[i].action = action;
            bindings_[i].handler = nullptr;
            bindings_[i].user_data = nullptr;
            return true;
        }
    }
    if (count_ >= MAX_BINDINGS) {
        return false;
    }
    bindings_[count_++] = ShortcutBinding{mod_mask, key, action, nullptr, nullptr};
    return true;
}

bool ShortcutRegistry::bind_callback(uint8_t mod_mask, KeyCode key, ShortcutHandler handler, void* user_data) noexcept {
    for (size_t i = 0; i < count_; ++i) {
        if (bindings_[i].mod_mask == mod_mask && bindings_[i].key == key) {
            bindings_[i].action = ShortcutAction::None;
            bindings_[i].handler = handler;
            bindings_[i].user_data = user_data;
            return true;
        }
    }
    if (count_ >= MAX_BINDINGS) {
        return false;
    }
    bindings_[count_++] = ShortcutBinding{mod_mask, key, ShortcutAction::None, handler, user_data};
    return true;
}

ShortcutAction ShortcutRegistry::check(KeyCode key, const ModifierState& mods) noexcept {
    uint8_t mask = mods.to_mask();
    for (size_t i = 0; i < count_; ++i) {
        if (bindings_[i].key == key && bindings_[i].mod_mask == mask) {
            if (bindings_[i].handler != nullptr) {
                bindings_[i].handler(bindings_[i].user_data);
            }
            return bindings_[i].action;
        }
    }
    return ShortcutAction::None;
}

bool ShortcutRegistry::has_binding(KeyCode key, const ModifierState& mods) const noexcept {
    uint8_t mask = mods.to_mask();
    for (size_t i = 0; i < count_; ++i) {
        if (bindings_[i].key == key && bindings_[i].mod_mask == mask) {
            return true;
        }
    }
    return false;
}

} // namespace kindle
