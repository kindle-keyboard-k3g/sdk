#pragma once
#include "kindle/input.hpp"
#include "kindle/key_catalog.hpp"
#include "kindle/modifier_tracker.hpp"
#include <linux/input.h>
#include <string_view>
#include <cstdint>

namespace kindle {

/**
 * Translates low-level Linux evdev input events or KeyCode pairs into ASCII characters,
 * symbols, and ANSI escape sequences with zero heap allocations.
 */
class KeyboardTranslator {
public:
    explicit KeyboardTranslator(DeviceModel model = DeviceModel::Kindle3) noexcept;

    [[nodiscard]] const ModifierState& modifiers() const noexcept { return mods_; }
    void reset_modifiers() noexcept { mods_ = ModifierState{}; }

    /**
     * Updates internal modifier state and translates an evdev key event into a character or escape string.
     * Returns an empty string_view if the event is a modifier, key release, or unhandled.
     */
    std::string_view process_event(const struct input_event& ev) noexcept;

    /**
     * Translates a decoded KeyCode and explicit ModifierState into a character or escape string.
     */
    std::string_view translate(KeyCode key, const ModifierState& mods) noexcept;

private:
    bool is_modifier_key(uint16_t code) const noexcept;
    void update_modifier(uint16_t code, bool active) noexcept;
    std::string_view emit(const char* str) noexcept;
    std::string_view emit_char(char c) noexcept;

    std::string_view translate_letters(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_alt_symbols(KeyCode key) noexcept;
    std::string_view translate_sym_symbols(KeyCode key) noexcept;
    std::string_view translate_numbers(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_punctuation(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_navigation(KeyCode key, const ModifierState& mods) noexcept;

    DeviceModel   model_;
    ModifierState mods_{};
    char          buffer_[16]{};
};

} // namespace kindle
