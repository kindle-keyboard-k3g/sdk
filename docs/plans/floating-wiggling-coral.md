# Plan: Improved Keyboard Management Subsystem for kindle-sdk

## 1. Context & Motivation

The Kindle Keyboard 3G (K3/K3G) and Kindle DX platforms present distinct physical and embedded constraints:
- **Missing Hardware Number Row on K3**: Unlike the Kindle DX, the K3 physical keyboard only has 4 rows (`QWERTY`, `ASDF`, `ZXCV`, and space/modifier row). Numbers (`1`..`0`) are generated either via `Alt + Q..P` chords or mapped directly by the kernel to Linux scancodes 2..11 (`KEY_1`..`KEY_0`).
- **Missing Punctuation Keys**: The physical keyboard lacks dedicated keys for comma (`,`), colon (`:`), semicolon (`;`), brackets (`[` `]`), braces (`{` `}`), quotes (`'` `"`), hyphen/plus (`-` `=`), and backslash (`\`). Users rely on `Alt + letter` chords and the `Sym` key.
- **Hardware Bouncing & Mechanical Age**: On aging Kindle hardware, physical switch contacts can chatter and bounce, generating spurious press/release events within 10–30 ms.
- **E-Ink Refresh Latency vs. Auto-Repeat Flooding**: Hardware/kernel auto-repeat (`ev.value == 2`) generates events at 20–30 Hz. When piped directly to an E-Ink display, this floods the event queue with partial updates, causing sluggish response and ghosting build-up.
- **Handheld Typing Ergonomics (Sticky/Latch Modifiers)**: When holding the Kindle with one or two hands, multi-key chording (e.g. holding `Shift` + `Z`, or `Alt` + `G`) can be clumsy. A sticky/latch modifier mode (tap `Shift` once to latch next character, double-tap to lock) provides superior ergonomics.

### Consumer Application Needs
1. **`dino` (Game)**: Requires instantaneous, debounced key press/release actions (`Jump`, `Duck`, `Quit`, `Restart`). Must bypass character translation latency, disable auto-repeat for jump commands, and support hardware volume keys.
2. **`papergram` (Messaging)**: Requires full alphanumeric and symbol typing (including commas via `Alt+Dot`, quotes, exclamation, brackets), cursor navigation via 5-way D-Pad, smooth debounced text input, and global `Alt+G` ("Ghostbuster") manual full E-Ink refresh.
3. **`kindle-myts` (Terminal Emulator)**: Requires deterministic zero-allocation ANSI escape sequence generation (`\033[A`..`\033[D`, `\033[5~`, `\033[6~`), ASCII control characters (`Ctrl+A` through `Ctrl+Z` via the `Aa` key), `Shift+Arrows` page scrolling, and chord interception for modal overlays.

---

## 2. Architecture & Component Decomposition

Following **SOLID** and **Object Calisthenics** (Single Responsibility, interfaces, classes ≤100 lines, functions ≤15 lines, no `else`, zero dynamic allocations in hot paths), the keyboard management subsystem is decomposed into five focused components:

```
Linux evdev (/dev/input/event0, event1, event2) or FakeInputDevice
                           |
                           v  InputEvent (raw scancode, press/release/repeat, timestamp_us)
               +-----------------------+
               |     InputDebouncer    |  Suppresses mechanical contact bounce (<20ms)
               +-----------------------+  and regulates auto-repeat rate
                           |
                           v  Debounced InputEvent
               +-----------------------+
               |    ModifierTracker    |  Tracks Shift, Alt, Ctrl (Aa), Sym
               +-----------------------+  Supports both active chording & sticky latch mode
                           |
                           v  (KeyCode, ModifierState)
               +-----------------------+
               |   ShortcutRegistry    |  Matches registered chords (e.g. Alt+G Ghostbuster,
               +-----------------------+  VolumeUp/Down) -> triggers action/callback
                           | (if not consumed by shortcut)
                           v
               +-----------------------+
               |   KeyboardTranslator  |  Decodes keycode + modifiers into ASCII/UTF-8
               +-----------------------+  or ANSI escape sequences (zero heap allocation)
                           |
                           v
               Rich KeyEvent (KeyCode, KeyEventType, ModifierState, text, is_shortcut)
```

---

## 3. Data Structures & Interfaces

### 3.1 Extended KeyCodes & InputEvent (`native/include/kindle/input.hpp`)
Extend `KeyCode` with missing punctuation and add `timestamp_us` to `InputEvent`:

```cpp
namespace kindle {

enum class KeyCode : uint16_t {
    // Alphanumeric keys
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Punctuation & Editing
    Enter,
    Space,
    Backspace,
    Dot,
    Slash,
    Comma,       // Added: Linux scancode 51 or Alt+Dot
    Semicolon,   // Added: Alt+M / Sym
    Apostrophe,  // Added: Sym
    Minus,       // Added: Alt+C
    Equal,       // Added: Alt+B
    LeftBracket, // Added: Alt+N
    RightBracket,// Added: Alt+M (DX) / Sym
    Backslash,   // Added: Alt+Slash

    // Modifiers
    Shift,
    Alt,
    Sym,
    Ctrl,

    // Navigation & Kindle Buttons
    Up,
    Down,
    Left,
    Right,
    Select,
    PageUp,
    PageDown,
    Back,
    Menu,
    Home,

    // Hardware functions
    VolumeUp,
    VolumeDown,
    Power,

    Unknown
};

struct InputEvent {
    KeyCode key{KeyCode::Unknown};
    KeyEventType type{KeyEventType::Press};
    uint16_t raw_code{0};
    uint64_t timestamp_us{0};
};

} // namespace kindle
```

### 3.2 Debounce & Key-Repeat Engine (`native/include/kindle/input_debouncer.hpp`)
Suppresses mechanical switch bounce and synthesizes/filters key repeat events:

```cpp
namespace kindle {

struct DebounceConfig {
    uint32_t debounce_ms{20};       // Ignore bounces within 20ms
    bool     repeat_enabled{true};  // If false, drops repeat events (e.g. for dino jumps)
    uint32_t repeat_delay_ms{400};  // Initial hold delay before repeat starts
    uint32_t repeat_interval_ms{80};// Interval between repeated events
};

class InputDebouncer {
public:
    explicit InputDebouncer(const DebounceConfig& config = {}) noexcept;

    /**
     * Filters raw input events. Returns true if event is valid and should be processed;
     * returns false if event is suppressed as bounce or filtered repeat.
     */
    bool filter(const InputEvent& in_event, uint64_t current_time_ms) noexcept;

    void reset() noexcept;

private:
    DebounceConfig config_;
    KeyCode        last_key_{KeyCode::Unknown};
    KeyEventType   last_type_{KeyEventType::Release};
    uint64_t       last_transition_ms_{0};
};

} // namespace kindle
```

### 3.3 Modifier Tracker with Sticky/Latch Mode (`native/include/kindle/modifier_tracker.hpp`)
Maintains modifier state for Shift, Alt, Ctrl (Aa key on K3/DX), and Sym. Supports both held chords and single-tap latching:

```cpp
namespace kindle {

enum class LatchMode : uint8_t {
    Disabled,   // Pure physical hold (default for games and terminal)
    StickyOnce, // Press & release modifier latches for next keypress only
    Lockable    // Double-tap locks modifier (Caps Lock / Alt Lock)
};

class ModifierTracker {
public:
    explicit ModifierTracker(LatchMode mode = LatchMode::Disabled) noexcept;

    void update(KeyCode key, KeyEventType type) noexcept;
    void reset() noexcept;

    [[nodiscard]] const ModifierState& state() const noexcept { return current_; }
    [[nodiscard]] bool is_latched() const noexcept { return latched_; }

    // Consumes single-shot latch after a non-modifier key is pressed
    void consume_latch() noexcept;

private:
    void handle_press(KeyCode key) noexcept;
    void handle_release(KeyCode key) noexcept;

    LatchMode     mode_;
    ModifierState current_{};
    bool          latched_{false};
    bool          locked_{false};
};

} // namespace kindle
```

### 3.4 Expanded Keyboard Translator (`native/include/kindle/keyboard_translator.hpp`)
Complete Kindle Keyboard (K3) and DX symbol mapping table:
- `Alt + Q..P` -> `'1'`, `'2'`, `'3'`, `'4'`, `'5'`, `'6'`, `'7'`, `'8'`, `'9'`, `'0'`
- `Alt + A..L` -> `'~'`, `'!'`, `'@'`, `'#'`, `'$'`, `'%'`, `'^'`, `'&'`, `'*'`
- `Alt + Z..M` -> `'('`, `')'`, `'-'`, `'+'`, `'='`, `'['`, `']'`
- `Alt + Dot` -> `','` (Comma on K3!)
- `Alt + Slash` -> `'\\'` (Backslash)
- `Alt + Space` -> `'_'` (Underscore)
- `Alt + Backspace` -> `'\x7f'`
- `Sym + letter` -> Extended symbol table (`'{'`, `'}'`, `'<'`, `'>'`, `';'`, `':'`, `'\'`, `'"'`, `` '`' ``, `'|'`)
- `Ctrl + A..Z` (via `Aa` key) -> `0x01`..`0x1A`
- Navigation keys -> ANSI escape sequences (`\033[A`, `\033[B`, `\033[C`, `\033[D`, `\033[5~`, `\033[6~`, `\033[H`, `\033`)
- `Shift + Up/Down` -> `\033[5~` / `\033[6~` (PageUp / PageDown)

```cpp
namespace kindle {

class KeyboardTranslator {
public:
    explicit KeyboardTranslator(DeviceModel model = DeviceModel::Kindle3) noexcept;

    /**
     * Translates KeyCode and ModifierState into ASCII char, symbol, or ANSI escape sequence.
     * Returns std::string_view backed by internal fixed buffer (zero heap allocation).
     */
    std::string_view translate(KeyCode key, const ModifierState& mods) noexcept;

private:
    std::string_view translate_letters(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_alt_symbols(KeyCode key) noexcept;
    std::string_view translate_sym_symbols(KeyCode key) noexcept;
    std::string_view translate_numbers(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_punctuation(KeyCode key, const ModifierState& mods) noexcept;
    std::string_view translate_navigation(KeyCode key, const ModifierState& mods) noexcept;

    std::string_view emit(const char* str) noexcept;
    std::string_view emit_char(char c) noexcept;

    DeviceModel model_;
    char        buffer_[16]{};
};

} // namespace kindle
```

### 3.5 Shortcut & Chord Registry (`native/include/kindle/shortcut_registry.hpp`)
Lightweight, fixed-capacity callback/action registry for global and custom chords:
- Pre-bound: `Alt + G` -> `ShortcutAction::Ghostbuster` (manual E-Ink full refresh)
- Pre-bound: `VolumeUp` / `VolumeDown` -> `ShortcutAction::VolumeUp` / `VolumeDown`
- Custom callback registration with zero dynamic memory allocation:

```cpp
namespace kindle {

enum class ShortcutAction : uint8_t {
    None,
    Ghostbuster,   // Alt+G: Full E-Ink refresh
    VolumeUp,      // VolumeUp key
    VolumeDown,    // VolumeDown key
    Home,          // Home / Alt+H
    Back,          // Back / Alt+B
    Menu           // Menu / Alt+M
};

using ShortcutHandler = void (*)(void* user_data);

struct ShortcutBinding {
    uint8_t         mod_mask{0};
    KeyCode         key{KeyCode::Unknown};
    ShortcutAction  action{ShortcutAction::None};
    ShortcutHandler handler{nullptr};
    void*           user_data{nullptr};
};

class ShortcutRegistry {
public:
    ShortcutRegistry() noexcept;

    bool bind_action(uint8_t mod_mask, KeyCode key, ShortcutAction action) noexcept;
    bool bind_callback(uint8_t mod_mask, KeyCode key, ShortcutHandler handler, void* user_data = nullptr) noexcept;
    void reset() noexcept;

    /**
     * Checks if event matches a registered chord.
     * If matched, triggers handler (if any) and returns action.
     */
    ShortcutAction check(KeyCode key, const ModifierState& mods) noexcept;

private:
    static constexpr size_t MAX_BINDINGS = 16;
    ShortcutBinding bindings_[MAX_BINDINGS]{};
    size_t          count_{0};
};

} // namespace kindle
```

### 3.6 Unified KeyboardManager Facade (`native/include/kindle/keyboard_manager.hpp`)
Coordinates `InputDevice`, `InputDebouncer`, `ModifierTracker`, `ShortcutRegistry`, and `KeyboardTranslator` into a clean high-level pipeline:

```cpp
namespace kindle {

struct KeyEvent {
    KeyCode          key{KeyCode::Unknown};
    KeyEventType     type{KeyEventType::Press};
    ModifierState    modifiers{};
    std::string_view text{};           // Decoded character or escape sequence
    ShortcutAction   shortcut_action{ShortcutAction::None};
    bool             is_shortcut{false};
};

class KeyboardManager {
public:
    explicit KeyboardManager(
        InputDevice& device,
        DeviceModel model = DeviceModel::Kindle3,
        const DebounceConfig& debounce = {},
        LatchMode latch = LatchMode::Disabled) noexcept;

    /**
     * Polls the next high-level KeyEvent. Returns false if no event available.
     * Zero heap allocation.
     */
    bool poll(KeyEvent& out_event) noexcept;

    ShortcutRegistry& shortcuts() noexcept { return shortcuts_; }
    const ModifierState& modifiers() const noexcept { return tracker_.state(); }
    void reset() noexcept;

private:
    InputDevice&       device_;
    InputDebouncer     debouncer_;
    ModifierTracker    tracker_;
    KeyboardTranslator translator_;
    ShortcutRegistry   shortcuts_;
};

} // namespace kindle
```

---

## 4. Files to Create & Modify

### Files to Create:
1. `native/include/kindle/input_debouncer.hpp` — Interface and config for debounce & repeat.
2. `native/src/input_debouncer.cpp` — Implementation of debounce filter and repeat limiter.
3. `native/include/kindle/modifier_tracker.hpp` — Modifier tracking and sticky latch engine.
4. `native/src/modifier_tracker.cpp` — Implementation of modifier transition state machine.
5. `native/include/kindle/shortcut_registry.hpp` — Chord table and callback dispatching.
6. `native/src/shortcut_registry.cpp` — Implementation of fixed-capacity shortcut registry.
7. `native/include/kindle/keyboard_manager.hpp` — High-level keyboard facade.
8. `native/src/keyboard_manager.cpp` — Integration pipeline implementation.
9. `native/tests/test_keyboard_manager.cpp` — Comprehensive unit test suite.

### Files to Modify:
1. `native/include/kindle/input.hpp`:
   - Add missing `KeyCode`s (`Comma`, `Semicolon`, `Apostrophe`, `Minus`, `Equal`, `LeftBracket`, `RightBracket`, `Backslash`).
   - Add `timestamp_us` to `InputEvent`.
2. `native/include/kindle/key_catalog.hpp`:
   - Add scancode 51 (`KEY_COMMA`) and extended hardware codes for K3 / DX.
3. `native/src/keyboard_translator.cpp`:
   - Expand `translate()` with complete Kindle 3 `Alt+letter` and `Sym` tables.
4. `native/CMakeLists.txt`:
   - Add `src/input_debouncer.cpp`, `src/modifier_tracker.cpp`, `src/shortcut_registry.cpp`, `src/keyboard_manager.cpp` to `kindle_native`.
   - Add `test_keyboard_manager` test target.

---

## 5. Consumer Integration Recipes

### 5.1 `dino` (Game)
- Instantiate `KeyboardManager` with `repeat_enabled = false` and `LatchMode::Disabled`.
- Listen directly to `event.key` and `event.type == KeyEventType::Press` for zero latency:
  ```cpp
  if (ev.key == KeyCode::Space || ev.key == KeyCode::Up) game.jump();
  else if (ev.key == KeyCode::Down || ev.key == KeyCode::D) game.duck(ev.type == KeyEventType::Press);
  ```

### 5.2 `papergram` (Messaging)
- Configure `LatchMode::StickyOnce` for comfortable thumb typing.
- Register `Alt+G` with `ShortcutRegistry` for manual E-Ink ghostbuster full refresh:
  ```cpp
  manager.shortcuts().bind_action(ModifierState::MOD_ALT, KeyCode::G, ShortcutAction::Ghostbuster);
  ```
- Use `ev.text` directly to append typed characters to the active input text box.

### 5.3 `kindle-myts` (Terminal)
- Feed `ev.text` directly into the PTY master descriptor when non-empty.
- Handle `ShortcutAction::Ghostbuster` to trigger `eink_display.update_display_full()`.
- Modifier states (Shift, Ctrl via Aa, Alt) are tracked automatically without custom app loops.

---

## 6. Verification & Test Plan

Follow strict **Test-Driven Development (TDD)** using `FakeInputDevice`:
1. **Debounce Tests**:
   - Inject bouncing transitions (<20ms); assert bounce events are dropped.
   - Inject repeat events with `repeat_enabled = false`; assert repeats are ignored.
   - Inject repeat events with `repeat_enabled = true`; assert repeats pass after delay.
2. **Modifier & Latch Tests**:
   - Verify standard held chords (`Shift` held + `A` -> uppercase `'A'`).
   - Verify `StickyOnce` mode: tap `Shift`, release `Shift`, press `A` -> uppercase `'A'`, next key `'b'` is lowercase.
   - Verify `Aa` key sets `ctrl` modifier; `Ctrl+C` produces `0x03`.
3. **Symbol Table Tests**:
   - Verify `Alt + Q..P` produces `'1'`..`'0'`.
   - Verify `Alt + Dot` produces `','`.
   - Verify `Alt + A..L` produces `~ ! @ # $ % ^ & *`.
   - Verify `Sym + letter` produces extended bracket/quote symbols.
4. **Shortcut Registry Tests**:
   - Verify `Alt + G` triggers `ShortcutAction::Ghostbuster`.
   - Verify `VolumeUp` / `VolumeDown` trigger volume shortcut actions.
   - Verify custom handler callback invocation with `user_data`.
5. **Full KeyboardManager Integration**:
   - Stream raw evdev sequence through `FakeInputDevice`; assert `KeyEvent`s emitted with correct text and modifier flags.
6. **Host & Target Verification**:
   - Host unit test run: `ctest --output-on-failure`.
   - ARMv6 cross-compilation check:
     `cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Kindle-ARMv6.cmake && make kindle_native`.

---

## Task 1: Extended KeyCodes, InputEvent Timestamps, and Scancode Catalog

### Description
Extend `KeyCode` with punctuation keys (`Comma`, `Semicolon`, `Apostrophe`, `Minus`, `Equal`, `LeftBracket`, `RightBracket`, `Backslash`). Add `timestamp_us` to `InputEvent`. Add corresponding scancodes and mapping in `KeyCatalog`.

### Steps
1. In `native/tests/test_input.cpp`, add tests verifying mapping and decoding of new keycodes and timestamp preservation.
2. Run test to verify RED state.
3. Update `native/include/kindle/input.hpp` and `native/include/kindle/key_catalog.hpp`.
4. Run test to verify GREEN state.
5. Commit changes.

---

## Task 2: Debounce & Key-Repeat Engine (InputDebouncer)

### Description
Implement `InputDebouncer` in `native/include/kindle/input_debouncer.hpp` and `native/src/input_debouncer.cpp`. Filter mechanical bounce transitions within `debounce_ms` and regulate or disable key auto-repeats.

### Steps
1. Add initial unit tests in `native/tests/test_keyboard_manager.cpp` covering bounce rejection and repeat suppression.
2. Run build to verify RED state (compiler or link error).
3. Implement `InputDebouncer` in `native/include/kindle/input_debouncer.hpp` and `native/src/input_debouncer.cpp`, and add to `native/CMakeLists.txt`.
4. Run tests to verify GREEN state.
5. Commit changes.

---

## Task 3: Modifier Tracker with Sticky/Latch Mode (ModifierTracker)

### Description
Implement `ModifierTracker` in `native/include/kindle/modifier_tracker.hpp` and `native/src/modifier_tracker.cpp`. Support `LatchMode::Disabled`, `LatchMode::StickyOnce`, and `LatchMode::Lockable`.

### Steps
1. Add unit tests for `ModifierTracker` in `native/tests/test_keyboard_manager.cpp` covering standard chords, sticky-once latching, and latch consumption.
2. Run build to verify RED state.
3. Implement `ModifierTracker` in `native/include/kindle/modifier_tracker.hpp` and `native/src/modifier_tracker.cpp`, and add to `native/CMakeLists.txt`.
4. Run tests to verify GREEN state.
5. Commit changes.

---

## Task 4: Expanded Keyboard Translator

### Description
Update `KeyboardTranslator` in `native/src/keyboard_translator.cpp` with complete Kindle 3 `Alt+letter` chords (`Alt+Q..P` numbers, `Alt+A..L` symbols, `Alt+Z..M` math/brackets, `Alt+Dot` comma, `Alt+Slash` backslash, `Alt+Space` underscore, `Sym+letter` extended symbols, and `Shift+Arrows` page scrolling).

### Steps
1. Add tests in `native/tests/test_keyboard_translator.cpp` asserting translation of new `Alt` and `Sym` chords.
2. Run test to verify RED state.
3. Implement translation tables in `native/src/keyboard_translator.cpp`.
4. Run tests to verify GREEN state.
5. Commit changes.

---

## Task 5: Shortcut & Chord Registry (ShortcutRegistry)

### Description
Implement `ShortcutRegistry` in `native/include/kindle/shortcut_registry.hpp` and `native/src/shortcut_registry.cpp`. Pre-bind `Alt+G` (Ghostbuster) and Volume keys. Allow binding custom callbacks with `user_data` without dynamic allocations.

### Steps
1. Add unit tests in `native/tests/test_keyboard_manager.cpp` verifying pre-bound shortcuts and custom callbacks.
2. Run build to verify RED state.
3. Implement `ShortcutRegistry` in `native/include/kindle/shortcut_registry.hpp` and `native/src/shortcut_registry.cpp`, and add to `native/CMakeLists.txt`.
4. Run tests to verify GREEN state.
5. Commit changes.

---

## Task 6: Unified KeyboardManager Facade (KeyboardManager)

### Description
Implement `KeyboardManager` in `native/include/kindle/keyboard_manager.hpp` and `native/src/keyboard_manager.cpp`. Wire `InputDevice`, `InputDebouncer`, `ModifierTracker`, `ShortcutRegistry`, and `KeyboardTranslator` into a high-level `poll(KeyEvent& out_event)` pipeline.

### Steps
1. Add integration tests in `native/tests/test_keyboard_manager.cpp` verifying streaming input through `FakeInputDevice`.
2. Run build to verify RED state.
3. Implement `KeyboardManager` in `native/include/kindle/keyboard_manager.hpp` and `native/src/keyboard_manager.cpp`, and add to `native/CMakeLists.txt`.
4. Run tests to verify GREEN state.
5. Commit changes.

---

## Task 7: Comprehensive Test Suite & Toolchain Verification

### Description
Finalize `native/tests/test_keyboard_manager.cpp` covering all consumer recipes (`dino`, `papergram`, `kindle-myts`), register test in `native/CMakeLists.txt`, verify host test pass, and verify ARMv6 cross-compilation.

### Steps
1. Review and complete all consumer recipe test scenarios in `native/tests/test_keyboard_manager.cpp`.
2. Run full test suite on host (`ctest --output-on-failure`).
3. Run ARMv6 cross-compilation check.
4. Commit changes.

