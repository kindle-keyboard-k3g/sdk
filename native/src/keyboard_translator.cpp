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
    }
    return (code == KeyCatalog::CODE_AA_CTRL_K3 || code == KeyCatalog::CODE_SYM_K3);
}

void KeyboardTranslator::update_modifier(uint16_t code, bool active) noexcept {
    if (code == KeyCatalog::CODE_SHIFT_L || code == KeyCatalog::CODE_SHIFT_R) {
        mods_.shift = active;
        return;
    }
    if (code == KeyCatalog::CODE_CTRL_L || code == KeyCatalog::CODE_CTRL_R) {
        mods_.ctrl = active;
        return;
    }
    if (code == KeyCatalog::CODE_ALT_L || code == KeyCatalog::CODE_ALT_R) {
        mods_.alt = active;
        return;
    }
    if (model_ == DeviceModel::KindleDX) {
        if (code == KeyCatalog::CODE_AA_CTRL_DX) { mods_.ctrl = active; return; }
        if (code == KeyCatalog::CODE_SYM_DX)     { mods_.sym = active; return; }
    }
    if (code == KeyCatalog::CODE_AA_CTRL_K3) { mods_.ctrl = active; return; }
    if (code == KeyCatalog::CODE_SYM_K3)     { mods_.sym = active; return; }
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
    if (ev.value == 0) {
        return {};
    }
    KeyCode key = KeyCatalog::map_scancode(ev.code, model_);
    return translate(key, mods_);
}

std::string_view KeyboardTranslator::translate_alt_symbols(KeyCode key) noexcept {
    switch (key) {
        case KeyCode::Q: return emit("1");
        case KeyCode::W: return emit("2");
        case KeyCode::E: return emit("3");
        case KeyCode::R: return emit("4");
        case KeyCode::T: return emit("5");
        case KeyCode::Y: return emit("6");
        case KeyCode::U: return emit("7");
        case KeyCode::I: return emit("8");
        case KeyCode::O: return emit("9");
        case KeyCode::P: return emit("0");
        case KeyCode::A: return emit("~");
        case KeyCode::S: return emit("!");
        case KeyCode::D: return emit("@");
        case KeyCode::F: return emit("#");
        case KeyCode::G: return emit("$");
        case KeyCode::H: return emit("%");
        case KeyCode::J: return emit("^");
        case KeyCode::K: return emit("&");
        case KeyCode::L: return emit("*");
        case KeyCode::Z: return emit("(");
        case KeyCode::X: return emit(")");
        case KeyCode::C: return emit("-");
        case KeyCode::V: return emit("+");
        case KeyCode::B: return emit("=");
        case KeyCode::N: return emit("[");
        case KeyCode::M: return emit("]");
        case KeyCode::Dot: return emit(",");
        case KeyCode::Slash: return emit("\\");
        case KeyCode::Space: return emit("_");
        case KeyCode::Backspace: return emit("\x7f");
        default: return {};
    }
}

std::string_view KeyboardTranslator::translate_sym_symbols(KeyCode key) noexcept {
    switch (key) {
        case KeyCode::Q: return emit("{");
        case KeyCode::W: return emit("}");
        case KeyCode::E: return emit("<");
        case KeyCode::R: return emit(">");
        case KeyCode::T: return emit(";");
        case KeyCode::Y: return emit(":");
        case KeyCode::U: return emit("'");
        case KeyCode::I: return emit("\"");
        case KeyCode::O: return emit("`");
        case KeyCode::P: return emit("|");
        default: return {};
    }
}

std::string_view KeyboardTranslator::translate_letters(KeyCode key, const ModifierState& mods) noexcept {
    if (mods.alt) {
        return translate_alt_symbols(key);
    }
    if (mods.sym) {
        return translate_sym_symbols(key);
    }
    auto offset = static_cast<uint16_t>(key) - static_cast<uint16_t>(KeyCode::A);
    if (mods.ctrl) {
        return emit_char(static_cast<char>(1 + offset));
    }
    if (mods.shift) {
        return emit_char(static_cast<char>('A' + offset));
    }
    return emit_char(static_cast<char>('a' + offset));
}

std::string_view KeyboardTranslator::translate_numbers(KeyCode key, const ModifierState& mods) noexcept {
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
            default: return {};
        }
    }
    if (key == KeyCode::Num0) {
        return emit("0");
    }
    auto num_offset = static_cast<uint16_t>(key) - static_cast<uint16_t>(KeyCode::Num1);
    return emit_char(static_cast<char>('1' + num_offset));
}

std::string_view KeyboardTranslator::translate_punctuation(KeyCode key, const ModifierState& mods) noexcept {
    if (mods.alt) {
        auto alt_res = translate_alt_symbols(key);
        if (!alt_res.empty()) {
            return alt_res;
        }
    }
    switch (key) {
        case KeyCode::Enter:        return emit("\r");
        case KeyCode::Space:        return emit(" ");
        case KeyCode::Backspace:    return emit("\x7f");
        case KeyCode::Dot:          return mods.shift ? emit(">") : emit(".");
        case KeyCode::Slash:        return mods.shift ? emit("?") : emit("/");
        case KeyCode::Comma:        return mods.shift ? emit("<") : emit(",");
        case KeyCode::Semicolon:    return mods.shift ? emit(":") : emit(";");
        case KeyCode::Apostrophe:   return mods.shift ? emit("\"") : emit("'");
        case KeyCode::Minus:        return mods.shift ? emit("_") : emit("-");
        case KeyCode::Equal:        return mods.shift ? emit("+") : emit("=");
        case KeyCode::LeftBracket:  return mods.shift ? emit("{") : emit("[");
        case KeyCode::RightBracket: return mods.shift ? emit("}") : emit("]");
        case KeyCode::Backslash:    return mods.shift ? emit("|") : emit("\\");
        default: return {};
    }
}

std::string_view KeyboardTranslator::translate_navigation(KeyCode key, const ModifierState& mods) noexcept {
    switch (key) {
        case KeyCode::Up:       return mods.shift ? emit("\033[5~") : emit("\033[A");
        case KeyCode::Down:     return mods.shift ? emit("\033[6~") : emit("\033[B");
        case KeyCode::Right:    return emit("\033[C");
        case KeyCode::Left:     return emit("\033[D");
        case KeyCode::PageUp:   return emit("\033[5~");
        case KeyCode::PageDown: return emit("\033[6~");
        case KeyCode::Home:     return emit("\033[H");
        case KeyCode::Back:     return emit("\033");
        default: return {};
    }
}

std::string_view KeyboardTranslator::translate(KeyCode key, const ModifierState& mods) noexcept {
    if (key >= KeyCode::A && key <= KeyCode::Z) {
        return translate_letters(key, mods);
    }
    if (key >= KeyCode::Num0 && key <= KeyCode::Num9) {
        return translate_numbers(key, mods);
    }
    auto punc = translate_punctuation(key, mods);
    if (!punc.empty()) {
        return punc;
    }
    return translate_navigation(key, mods);
}

} // namespace kindle
