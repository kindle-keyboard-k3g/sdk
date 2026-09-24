#include "kindle/keyboard_translator.hpp"
#include <cstring>

namespace kindle {

KeyboardTranslator::KeyboardTranslator(DeviceModel model) noexcept
    : model_(model) {}

bool KeyboardTranslator::is_modifier_key(uint16_t code) const noexcept {
    if (code == KeyCatalog::CODE_SHIFT_L || code == KeyCatalog::CODE_SHIFT_R ||
        code == KeyCatalog::CODE_CTRL_L  || code == KeyCatalog::CODE_CTRL_R  ||
        code == KeyCatalog::CODE_ALT_L   || code == KeyCatalog::CODE_ALT_R) {
        return true;
    }

    if (model_ == DeviceModel::KindleDX) {
        return (code == KeyCatalog::CODE_AA_CTRL_DX || code == KeyCatalog::CODE_SYM_DX);
    } else {
        return (code == KeyCatalog::CODE_AA_CTRL_K3 || code == KeyCatalog::CODE_SYM_K3);
    }
}

void KeyboardTranslator::update_modifier(uint16_t code, bool active) noexcept {
    if (code == KeyCatalog::CODE_SHIFT_L || code == KeyCatalog::CODE_SHIFT_R) {
        mods_.shift = active;
    } else if (code == KeyCatalog::CODE_CTRL_L || code == KeyCatalog::CODE_CTRL_R) {
        mods_.ctrl = active;
    } else if (code == KeyCatalog::CODE_ALT_L || code == KeyCatalog::CODE_ALT_R) {
        mods_.alt = active;
    } else if (model_ == DeviceModel::KindleDX) {
        if (code == KeyCatalog::CODE_AA_CTRL_DX) {
            mods_.ctrl = active;
        } else if (code == KeyCatalog::CODE_SYM_DX) {
            mods_.sym = active;
        }
    } else {
        if (code == KeyCatalog::CODE_AA_CTRL_K3) {
            mods_.ctrl = active;
        } else if (code == KeyCatalog::CODE_SYM_K3) {
            mods_.sym = active;
        }
    }
}

std::string_view KeyboardTranslator::emit(const char* str) noexcept {
    return std::string_view(str);
}

std::string_view KeyboardTranslator::emit_char(char c) noexcept {
    buffer_[0] = c;
    buffer_[1] = '\0';
    return std::string_view(buffer_, 1);
}

std::string_view KeyboardTranslator::process_event(const struct input_event& ev) noexcept {
    if (ev.type != EV_KEY) {
        return {};
    }

    if (is_modifier_key(ev.code)) {
        update_modifier(ev.code, ev.value != 0);
        return {};
    }

    // Only process key press (1) and repeat (2). Ignore key release (0).
    if (ev.value == 0) {
        return {};
    }

    KeyCode key = KeyCatalog::map_scancode(ev.code, model_);
    return translate(key, mods_);
}

std::string_view KeyboardTranslator::translate(KeyCode key, const ModifierState& mods) noexcept {
    // 1. Letters A through Z
    if (key >= KeyCode::A && key <= KeyCode::Z) {
        auto offset = static_cast<uint16_t>(key) - static_cast<uint16_t>(KeyCode::A);
        if (mods.ctrl) {
            // ASCII Control Characters \x01 through \x1A
            return emit_char(static_cast<char>(1 + offset));
        }
        if (mods.shift) {
            return emit_char(static_cast<char>('A' + offset));
        }
        return emit_char(static_cast<char>('a' + offset));
    }

    // 2. Numbers 0 through 9 and Shifted Symbols
    if (key >= KeyCode::Num0 && key <= KeyCode::Num9) {
        if (mods.shift) {
            switch (key) {
                case KeyCode::Num1: return emit("!");
                case KeyCode::Num2: return emit("@");
                case KeyCode::Num3: return emit("#");
                case KeyCode::Num4: return emit("$");
                case KeyCode::Num5: return emit("%");
                case KeyCode::Num6: return emit("^");
                case KeyCode::Num7: return emit("&");
                case KeyCode::Num8: return emit("*");
                case KeyCode::Num9: return emit("(");
                case KeyCode::Num0: return emit(")");
                default: break;
            }
        }
        if (key == KeyCode::Num0) {
            return emit("0");
        }
        auto num_offset = static_cast<uint16_t>(key) - static_cast<uint16_t>(KeyCode::Num1);
        return emit_char(static_cast<char>('1' + num_offset));
    }

    // 3. Punctuation & Editing
    switch (key) {
        case KeyCode::Enter:     return emit("\r");
        case KeyCode::Space:     return emit(" ");
        case KeyCode::Backspace: return emit("\x7f");
        case KeyCode::Dot:       return mods.shift ? emit(">") : emit(".");
        case KeyCode::Slash:     return mods.shift ? emit("?") : emit("/");
        default: break;
    }

    // 4. Navigation & ANSI Escape Sequences
    switch (key) {
        case KeyCode::Up:       return emit("\033[A");
        case KeyCode::Down:     return emit("\033[B");
        case KeyCode::Right:    return emit("\033[C");
        case KeyCode::Left:     return emit("\033[D");
        case KeyCode::PageUp:   return emit("\033[5~");
        case KeyCode::PageDown: return emit("\033[6~");
        case KeyCode::Home:     return emit("\033[H");
        case KeyCode::Back:     return emit("\033"); // Escape
        default: break;
    }

    return {};
}

} // namespace kindle
