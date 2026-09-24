#pragma once
#include "kindle/input.hpp"
#include <cstdint>

namespace kindle {

enum class DeviceModel : uint8_t {
    Auto,
    Kindle3,
    KindleDX
};

struct KeyCatalog {
    // Linux standard scancodes (Alphanumeric 0-9)
    static constexpr uint16_t CODE_KEY_1 = 2;
    static constexpr uint16_t CODE_KEY_2 = 3;
    static constexpr uint16_t CODE_KEY_3 = 4;
    static constexpr uint16_t CODE_KEY_4 = 5;
    static constexpr uint16_t CODE_KEY_5 = 6;
    static constexpr uint16_t CODE_KEY_6 = 7;
    static constexpr uint16_t CODE_KEY_7 = 8;
    static constexpr uint16_t CODE_KEY_8 = 9;
    static constexpr uint16_t CODE_KEY_9 = 10;
    static constexpr uint16_t CODE_KEY_0 = 11;

    // Alphanumeric QWERTY row 1
    static constexpr uint16_t CODE_KEY_Q = 16;
    static constexpr uint16_t CODE_KEY_W = 17;
    static constexpr uint16_t CODE_KEY_E = 18;
    static constexpr uint16_t CODE_KEY_R = 19;
    static constexpr uint16_t CODE_KEY_T = 20;
    static constexpr uint16_t CODE_KEY_Y = 21;
    static constexpr uint16_t CODE_KEY_U = 22;
    static constexpr uint16_t CODE_KEY_I = 23;
    static constexpr uint16_t CODE_KEY_O = 24;
    static constexpr uint16_t CODE_KEY_P = 25;

    // Alphanumeric QWERTY row 2
    static constexpr uint16_t CODE_KEY_A = 30;
    static constexpr uint16_t CODE_KEY_S = 31;
    static constexpr uint16_t CODE_KEY_D = 32;
    static constexpr uint16_t CODE_KEY_F = 33;
    static constexpr uint16_t CODE_KEY_G = 34;
    static constexpr uint16_t CODE_KEY_H = 35;
    static constexpr uint16_t CODE_KEY_J = 36;
    static constexpr uint16_t CODE_KEY_K = 37;
    static constexpr uint16_t CODE_KEY_L = 38;

    // Alphanumeric QWERTY row 3
    static constexpr uint16_t CODE_KEY_Z = 44;
    static constexpr uint16_t CODE_KEY_X = 45;
    static constexpr uint16_t CODE_KEY_C = 46;
    static constexpr uint16_t CODE_KEY_V = 47;
    static constexpr uint16_t CODE_KEY_B = 48;
    static constexpr uint16_t CODE_KEY_N = 49;
    static constexpr uint16_t CODE_KEY_M = 50;

    // Punctuation & Editing
    static constexpr uint16_t CODE_MINUS        = 12;
    static constexpr uint16_t CODE_EQUAL        = 13;
    static constexpr uint16_t CODE_BACKSPACE    = 14;
    static constexpr uint16_t CODE_LEFTBRACKET  = 26;
    static constexpr uint16_t CODE_RIGHTBRACKET = 27;
    static constexpr uint16_t CODE_ENTER        = 28;
    static constexpr uint16_t CODE_SEMICOLON    = 39;
    static constexpr uint16_t CODE_APOSTROPHE   = 40;
    static constexpr uint16_t CODE_BACKSLASH    = 43;
    static constexpr uint16_t CODE_COMMA        = 51;
    static constexpr uint16_t CODE_DOT          = 52;
    static constexpr uint16_t CODE_SLASH        = 53;
    static constexpr uint16_t CODE_SPACE        = 57;

    // Modifiers
    static constexpr uint16_t CODE_SHIFT_L   = 42;
    static constexpr uint16_t CODE_SHIFT_R   = 54;
    static constexpr uint16_t CODE_CTRL_L    = 29;
    static constexpr uint16_t CODE_CTRL_R    = 97;
    static constexpr uint16_t CODE_ALT_L     = 56;
    static constexpr uint16_t CODE_ALT_R     = 100;

    // Common navigation / buttons
    static constexpr uint16_t CODE_MENU      = 139;
    static constexpr uint16_t CODE_FW_LEFT   = 105;
    static constexpr uint16_t CODE_FW_RIGHT  = 106;

    // Volume and power
    static constexpr uint16_t CODE_VOL_DOWN  = 114;
    static constexpr uint16_t CODE_VOL_UP    = 115;
    static constexpr uint16_t CODE_POWER     = 116;
    static constexpr uint16_t CODE_SLEEP     = 142;
    static constexpr uint16_t CODE_SUSPEND   = 205;

    // Model-dependent overrides
    static constexpr uint16_t CODE_BACK_K3       = 158;
    static constexpr uint16_t CODE_BACK_DX       = 91;
    static constexpr uint16_t CODE_HOME_K3       = 102;
    static constexpr uint16_t CODE_HOME_DX       = 98;
    static constexpr uint16_t CODE_SELECT_K3     = 194;
    static constexpr uint16_t CODE_SELECT_DX     = 92;
    static constexpr uint16_t CODE_FW_UP_K3      = 103;
    static constexpr uint16_t CODE_FW_UP_DX      = 122;
    static constexpr uint16_t CODE_FW_DOWN_K3    = 108;
    static constexpr uint16_t CODE_FW_DOWN_DX    = 123;
    static constexpr uint16_t CODE_SYM_K3        = 126;
    static constexpr uint16_t CODE_SYM_DX        = 94;
    static constexpr uint16_t CODE_AA_CTRL_K3    = 190;
    static constexpr uint16_t CODE_AA_CTRL_DX    = 90;

    // Page turns
    static constexpr uint16_t CODE_PG_FWD_K3     = 191; // Right forward (Next)
    static constexpr uint16_t CODE_PG_BCK_K3     = 109; // Right backward (Prev)
    static constexpr uint16_t CODE_LPG_FWD_K3    = 104; // Left forward (Next)
    static constexpr uint16_t CODE_LPG_BCK_K3    = 193; // Left backward (Prev)

    static constexpr uint16_t CODE_PG_FWD_DX     = 124;
    static constexpr uint16_t CODE_PG_BCK_DX     = 109;
    static constexpr uint16_t CODE_LPG_FWD_DX    = 124;
    static constexpr uint16_t CODE_LPG_BCK_DX    = 104;

    static constexpr KeyCode map_scancode(uint16_t code, DeviceModel model = DeviceModel::Kindle3) noexcept {
        // Model-specific overrides first
        if (model == DeviceModel::KindleDX) {
            switch (code) {
                case CODE_BACK_DX:    return KeyCode::Back;
                case CODE_HOME_DX:    return KeyCode::Home;
                case CODE_SELECT_DX:  return KeyCode::Select;
                case CODE_FW_UP_DX:   return KeyCode::Up;
                case CODE_FW_DOWN_DX: return KeyCode::Down;
                case CODE_SYM_DX:     return KeyCode::Sym;
                case CODE_AA_CTRL_DX: return KeyCode::Ctrl;
                case CODE_PG_FWD_DX:  return KeyCode::PageDown;
                case CODE_PG_BCK_DX:  return KeyCode::PageUp;
                default: break;
            }
        } else {
            // Kindle 3 (or Auto default)
            switch (code) {
                case CODE_BACK_K3:    return KeyCode::Back;
                case CODE_HOME_K3:    return KeyCode::Home;
                case CODE_SELECT_K3:  return KeyCode::Select;
                case CODE_FW_UP_K3:   return KeyCode::Up;
                case CODE_FW_DOWN_K3: return KeyCode::Down;
                case CODE_SYM_K3:     return KeyCode::Sym;
                case CODE_AA_CTRL_K3: return KeyCode::Ctrl;
                case CODE_PG_FWD_K3:  return KeyCode::PageDown;
                case CODE_PG_BCK_K3:  return KeyCode::PageUp;
                case CODE_LPG_FWD_K3: return KeyCode::PageDown;
                case CODE_LPG_BCK_K3: return KeyCode::PageUp;
                default: break;
            }
        }

        // Shared / standard Linux scancodes
        switch (code) {
            case CODE_KEY_1: return KeyCode::Num1;
            case CODE_KEY_2: return KeyCode::Num2;
            case CODE_KEY_3: return KeyCode::Num3;
            case CODE_KEY_4: return KeyCode::Num4;
            case CODE_KEY_5: return KeyCode::Num5;
            case CODE_KEY_6: return KeyCode::Num6;
            case CODE_KEY_7: return KeyCode::Num7;
            case CODE_KEY_8: return KeyCode::Num8;
            case CODE_KEY_9: return KeyCode::Num9;
            case CODE_KEY_0: return KeyCode::Num0;

            case CODE_KEY_Q: return KeyCode::Q;
            case CODE_KEY_W: return KeyCode::W;
            case CODE_KEY_E: return KeyCode::E;
            case CODE_KEY_R: return KeyCode::R;
            case CODE_KEY_T: return KeyCode::T;
            case CODE_KEY_Y: return KeyCode::Y;
            case CODE_KEY_U: return KeyCode::U;
            case CODE_KEY_I: return KeyCode::I;
            case CODE_KEY_O: return KeyCode::O;
            case CODE_KEY_P: return KeyCode::P;

            case CODE_KEY_A: return KeyCode::A;
            case CODE_KEY_S: return KeyCode::S;
            case CODE_KEY_D: return KeyCode::D;
            case CODE_KEY_F: return KeyCode::F;
            case CODE_KEY_G: return KeyCode::G;
            case CODE_KEY_H: return KeyCode::H;
            case CODE_KEY_J: return KeyCode::J;
            case CODE_KEY_K: return KeyCode::K;
            case CODE_KEY_L: return KeyCode::L;

            case CODE_KEY_Z: return KeyCode::Z;
            case CODE_KEY_X: return KeyCode::X;
            case CODE_KEY_C: return KeyCode::C;
            case CODE_KEY_V: return KeyCode::V;
            case CODE_KEY_B: return KeyCode::B;
            case CODE_KEY_N: return KeyCode::N;
            case CODE_KEY_M: return KeyCode::M;

            case CODE_BACKSPACE:    return KeyCode::Backspace;
            case CODE_ENTER:        return KeyCode::Enter;
            case CODE_SPACE:        return KeyCode::Space;
            case CODE_DOT:          return KeyCode::Dot;
            case CODE_SLASH:        return KeyCode::Slash;
            case CODE_COMMA:        return KeyCode::Comma;
            case CODE_SEMICOLON:    return KeyCode::Semicolon;
            case CODE_APOSTROPHE:   return KeyCode::Apostrophe;
            case CODE_MINUS:        return KeyCode::Minus;
            case CODE_EQUAL:        return KeyCode::Equal;
            case CODE_LEFTBRACKET:  return KeyCode::LeftBracket;
            case CODE_RIGHTBRACKET: return KeyCode::RightBracket;
            case CODE_BACKSLASH:    return KeyCode::Backslash;

            case CODE_SHIFT_L:
            case CODE_SHIFT_R:
                return KeyCode::Shift;

            case CODE_CTRL_L:
            case CODE_CTRL_R:
                return KeyCode::Ctrl;

            case CODE_ALT_L:
            case CODE_ALT_R:
                return KeyCode::Alt;

            case CODE_MENU:
                return KeyCode::Menu;

            case CODE_FW_LEFT:
                return KeyCode::Left;

            case CODE_FW_RIGHT:
                return KeyCode::Right;

            case CODE_VOL_DOWN:
                return KeyCode::VolumeDown;

            case CODE_VOL_UP:
                return KeyCode::VolumeUp;

            case CODE_POWER:
            case CODE_SLEEP:
            case CODE_SUSPEND:
                return KeyCode::Power;

            default:
                return KeyCode::Unknown;
        }
    }
};

} // namespace kindle
