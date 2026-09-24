#include "kindle/keyboard_translator.hpp"
#include <cassert>
#include <iostream>
#include <linux/input.h>

int main() {
    std::cout << "Testing KeyboardTranslator...\n";
    kindle::KeyboardTranslator translator(kindle::DeviceModel::Kindle3);

    // 1. Lowercase letter
    struct input_event ev_a{ {}, EV_KEY, 30, 1 }; // Key A press
    auto s1 = translator.process_event(ev_a);
    assert(s1 == "a");

    // Release A produces no string
    ev_a.value = 0;
    assert(translator.process_event(ev_a).empty());

    // 2. Shift modifier + Letter -> Uppercase
    struct input_event ev_shift_press{ {}, EV_KEY, 42, 1 }; // Shift press
    assert(translator.process_event(ev_shift_press).empty());
    assert(translator.modifiers().shift);

    ev_a.value = 1;
    auto s2 = translator.process_event(ev_a);
    assert(s2 == "A");

    // Release Shift
    struct input_event ev_shift_release{ {}, EV_KEY, 42, 0 };
    assert(translator.process_event(ev_shift_release).empty());
    assert(!translator.modifiers().shift);

    // 3. Shift + Numeric Row -> Symbols
    // Scancode 2 = '1' -> Shift+'1' = '!'
    ev_shift_press.value = 1;
    translator.process_event(ev_shift_press);

    struct input_event ev_num1{ {}, EV_KEY, 2, 1 };
    assert(translator.process_event(ev_num1) == "!");

    struct input_event ev_num2{ {}, EV_KEY, 3, 1 };
    assert(translator.process_event(ev_num2) == "@");

    struct input_event ev_num3{ {}, EV_KEY, 4, 1 };
    assert(translator.process_event(ev_num3) == "#");

    struct input_event ev_num4{ {}, EV_KEY, 5, 1 };
    assert(translator.process_event(ev_num4) == "$");

    struct input_event ev_num5{ {}, EV_KEY, 6, 1 };
    assert(translator.process_event(ev_num5) == "%");

    struct input_event ev_num6{ {}, EV_KEY, 7, 1 };
    assert(translator.process_event(ev_num6) == "^");

    struct input_event ev_num7{ {}, EV_KEY, 8, 1 };
    assert(translator.process_event(ev_num7) == "&");

    struct input_event ev_num8{ {}, EV_KEY, 9, 1 };
    assert(translator.process_event(ev_num8) == "*");

    struct input_event ev_num9{ {}, EV_KEY, 10, 1 };
    assert(translator.process_event(ev_num9) == "(");

    struct input_event ev_num0{ {}, EV_KEY, 11, 1 };
    assert(translator.process_event(ev_num0) == ")");

    translator.process_event(ev_shift_release);
    // Unshifted '1'
    assert(translator.process_event(ev_num1) == "1");

    // 4. Ctrl + Letter (Aa key on K3 = scancode 190)
    struct input_event ev_aa_press{ {}, EV_KEY, 190, 1 };
    translator.process_event(ev_aa_press);
    assert(translator.modifiers().ctrl);

    // Ctrl+C -> \x03
    struct input_event ev_c{ {}, EV_KEY, 46, 1 };
    auto ctrl_c = translator.process_event(ev_c);
    assert(ctrl_c.size() == 1 && ctrl_c[0] == '\x03');

    // Ctrl+A -> \x01
    auto ctrl_a = translator.process_event(ev_a);
    assert(ctrl_a.size() == 1 && ctrl_a[0] == '\x01');

    // Ctrl+Z -> \x1A
    struct input_event ev_z{ {}, EV_KEY, 44, 1 };
    auto ctrl_z = translator.process_event(ev_z);
    assert(ctrl_z.size() == 1 && ctrl_z[0] == '\x1A');

    struct input_event ev_aa_release{ {}, EV_KEY, 190, 0 };
    translator.process_event(ev_aa_release);
    assert(!translator.modifiers().ctrl);

    // 5. Arrow Keys -> ANSI Escape Sequences
    struct input_event ev_up{ {}, EV_KEY, 103, 1 }; // 5-way Up K3
    assert(translator.process_event(ev_up) == "\033[A");

    struct input_event ev_down{ {}, EV_KEY, 108, 1 }; // 5-way Down K3
    assert(translator.process_event(ev_down) == "\033[B");

    struct input_event ev_right{ {}, EV_KEY, 106, 1 }; // 5-way Right
    assert(translator.process_event(ev_right) == "\033[C");

    struct input_event ev_left{ {}, EV_KEY, 105, 1 }; // 5-way Left
    assert(translator.process_event(ev_left) == "\033[D");

    // Shift + Up/Down -> PageUp / PageDown
    translator.process_event(ev_shift_press);
    assert(translator.process_event(ev_up) == "\033[5~");
    assert(translator.process_event(ev_down) == "\033[6~");
    translator.process_event(ev_shift_release);

    // 6. Enter, Space, Del, Dot, Slash
    struct input_event ev_enter{ {}, EV_KEY, 28, 1 };
    assert(translator.process_event(ev_enter) == "\r");

    struct input_event ev_space{ {}, EV_KEY, 57, 1 };
    assert(translator.process_event(ev_space) == " ");

    struct input_event ev_del{ {}, EV_KEY, 14, 1 };
    assert(translator.process_event(ev_del) == "\x7f");

    struct input_event ev_dot{ {}, EV_KEY, 52, 1 };
    assert(translator.process_event(ev_dot) == ".");

    struct input_event ev_slash{ {}, EV_KEY, 53, 1 };
    assert(translator.process_event(ev_slash) == "/");

    // Shift + Slash -> ?
    translator.process_event(ev_shift_press);
    assert(translator.process_event(ev_slash) == "?");
    translator.process_event(ev_shift_release);

    // 7. Alt Chords (Numbers, Symbols, Brackets, Comma)
    struct input_event ev_alt_press{ {}, EV_KEY, 56, 1 }; // Alt press
    struct input_event ev_alt_release{ {}, EV_KEY, 56, 0 }; // Alt release

    translator.process_event(ev_alt_press);
    assert(translator.modifiers().alt);

    struct input_event ev_q{ {}, EV_KEY, 16, 1 };
    assert(translator.process_event(ev_q) == "1");

    struct input_event ev_w{ {}, EV_KEY, 17, 1 };
    assert(translator.process_event(ev_w) == "2");

    struct input_event ev_p{ {}, EV_KEY, 25, 1 };
    assert(translator.process_event(ev_p) == "0");

    assert(translator.process_event(ev_a) == "~");

    struct input_event ev_s{ {}, EV_KEY, 31, 1 };
    assert(translator.process_event(ev_s) == "!");

    struct input_event ev_d{ {}, EV_KEY, 32, 1 };
    assert(translator.process_event(ev_d) == "@");

    struct input_event ev_g{ {}, EV_KEY, 34, 1 };
    assert(translator.process_event(ev_g) == "$");

    assert(translator.process_event(ev_z) == "(");
    assert(translator.process_event(ev_c) == "-");

    struct input_event ev_v{ {}, EV_KEY, 47, 1 };
    assert(translator.process_event(ev_v) == "+");

    struct input_event ev_b{ {}, EV_KEY, 48, 1 };
    assert(translator.process_event(ev_b) == "=");

    struct input_event ev_n{ {}, EV_KEY, 49, 1 };
    assert(translator.process_event(ev_n) == "[");

    struct input_event ev_m{ {}, EV_KEY, 50, 1 };
    assert(translator.process_event(ev_m) == "]");

    // Alt + Dot -> ','
    assert(translator.process_event(ev_dot) == ",");

    // Alt + Slash -> '\'
    assert(translator.process_event(ev_slash) == "\\");

    // Alt + Space -> '_'
    assert(translator.process_event(ev_space) == "_");

    translator.process_event(ev_alt_release);
    assert(!translator.modifiers().alt);

    // 8. Sym Chords
    struct input_event ev_sym_press{ {}, EV_KEY, 126, 1 }; // Sym K3
    struct input_event ev_sym_release{ {}, EV_KEY, 126, 0 };

    translator.process_event(ev_sym_press);
    assert(translator.modifiers().sym);
    assert(translator.process_event(ev_q) == "{");
    assert(translator.process_event(ev_w) == "}");

    struct input_event ev_e{ {}, EV_KEY, 18, 1 };
    assert(translator.process_event(ev_e) == "<");

    struct input_event ev_r{ {}, EV_KEY, 19, 1 };
    assert(translator.process_event(ev_r) == ">");

    struct input_event ev_t{ {}, EV_KEY, 20, 1 };
    assert(translator.process_event(ev_t) == ";");

    struct input_event ev_y{ {}, EV_KEY, 21, 1 };
    assert(translator.process_event(ev_y) == ":");

    struct input_event ev_u{ {}, EV_KEY, 22, 1 };
    assert(translator.process_event(ev_u) == "'");

    struct input_event ev_i{ {}, EV_KEY, 23, 1 };
    assert(translator.process_event(ev_i) == "\"");

    struct input_event ev_o{ {}, EV_KEY, 24, 1 };
    assert(translator.process_event(ev_o) == "`");

    assert(translator.process_event(ev_p) == "|");

    translator.process_event(ev_sym_release);
    assert(!translator.modifiers().sym);

    // 9. Direct punctuation KeyCodes
    kindle::ModifierState no_mods{};
    assert(translator.translate(kindle::KeyCode::Comma, no_mods) == ",");
    assert(translator.translate(kindle::KeyCode::Semicolon, no_mods) == ";");
    assert(translator.translate(kindle::KeyCode::Apostrophe, no_mods) == "'");
    assert(translator.translate(kindle::KeyCode::Minus, no_mods) == "-");
    assert(translator.translate(kindle::KeyCode::Equal, no_mods) == "=");
    assert(translator.translate(kindle::KeyCode::LeftBracket, no_mods) == "[");
    assert(translator.translate(kindle::KeyCode::RightBracket, no_mods) == "]");
    assert(translator.translate(kindle::KeyCode::Backslash, no_mods) == "\\");

    // 10. Kindle DX Model tests
    kindle::KeyboardTranslator dx_translator(kindle::DeviceModel::KindleDX);
    // Aa / Ctrl on DX is scancode 90
    struct input_event ev_dx_aa{ {}, EV_KEY, 90, 1 };
    dx_translator.process_event(ev_dx_aa);
    assert(dx_translator.modifiers().ctrl);
    auto dx_ctrl_c = dx_translator.process_event(ev_c);
    assert(dx_ctrl_c.size() == 1 && dx_ctrl_c[0] == '\x03');
    ev_dx_aa.value = 0;
    dx_translator.process_event(ev_dx_aa);
    assert(!dx_translator.modifiers().ctrl);

    // 5-way Up on DX is scancode 122
    struct input_event ev_dx_up{ {}, EV_KEY, 122, 1 };
    assert(dx_translator.process_event(ev_dx_up) == "\033[A");

    // 5-way Down on DX is scancode 123
    struct input_event ev_dx_down{ {}, EV_KEY, 123, 1 };
    assert(dx_translator.process_event(ev_dx_down) == "\033[B");

    std::cout << "PASS: test_keyboard_translator\n";
    return 0;
}
