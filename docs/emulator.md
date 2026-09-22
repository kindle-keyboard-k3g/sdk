# Kindle Desktop E-Ink Simulator Guide

The Kindle SDK includes a desktop AWT/Swing simulator designed to reproduce the visual and hardware interaction characteristics of physical Kindle devices (Kindle Keyboard K3 and Kindle DX).

## Architecture

The simulator consists of four main architectural layers:

```
┌──────────────────────────────────────────────────┐
│           KindleSimulatorWindow (Frame)          │
├──────────────────────────────────────────────────┤
│                                                  │
│   ┌──────────────────────────────────────────┐   │
│   │        EinkScreenPanel (Display)         │   │
│   │  - 600x800 (K3) or 824x1200 (DX)         │   │
│   │  - EinkQuantizer (16-level grayscale)   │   │
│   │  - Refresh flash simulation              │   │
│   │  - Embeds Kindlet rootContainer          │   │
│   └──────────────────────────────────────────┘   │
│                                                  │
│   ┌──────────────────────────────────────────┐   │
│   │          KeypadPanel (Hardware Keys)     │   │
│   │  [PgUp]   [ ▲ ]   [PgDn]                 │   │
│   │  [ ◄  ]  [SEL]   [ ►  ]                 │   │
│   │  [BACK]   [ ▼ ]   [MENU]                 │   │
│   └──────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
```

### 1. Grayscale Quantization (`EinkQuantizer`)

Real Kindle E-Ink controllers display images using 16 distinct grayscale levels (4 bits per pixel):
- Computes standard NTSC luminance: $Y = 0.299R + 0.587G + 0.114B$.
- Maps luminance values $[0, 255]$ into discrete 16-level quantization steps: $\text{level} = \lfloor\frac{Y \times 15 + 127}{255}\rfloor$.
- Accurately renders contrast and visual readability before deploying to physical hardware.

### 2. Screen Refresh Simulation (`EinkScreenPanel`)

E-Ink screens require physical pigment inversion cycles to clear ghosting particles. The simulator accurately mimics this behavior:
- **Full Refresh / Flash:** Momentarily renders screen inversion (black/white transition) on startup and major page turns.
- **Partial Refresh:** Fast regional repaints without full inversion.

### 3. Keypad & Input Navigation (`KeypadPanel`)

Mimics the physical hardware controls found on the Kindle Keyboard and Kindle DX:
- **5-way D-Pad:** Directional navigation (`▲`, `▼`, `◄`, `►`) and Center `Select`.
- **Page Turn Buttons:** `PgUp` and `PgDn` controls.
- **System Buttons:** Dedicated `Back` and `Menu` keys.

### 4. Simulator Kindlet Context (`SimulatorKindletContext`)

Implements the official `com.amazon.kindle.kindlet.KindletContext` interface:
- Returns an isolated, double-buffered `Container` for Kindlet UI rendering.
- Manages an isolated temporary directory simulating `/var/local/mesquite/data/<app-id>/`.
- Supports screen orientation configuration and reporting.

---

## Running the Simulator

### Interactive Mode

```bash
# Launch via CLI
kindle-sdk emulate path/to/my-app.azw2 --target k3

# Launch directly with Java
java -cp java/emulator/build/emulator.jar:java/kindlet-api/build/kindlet-api.jar \
    com.amazon.kindle.emulator.EmulatorLauncher path/to/my-app.azw2 --width 600 --height 800
```

### Headless CI Automation Mode

For headless test pipelines, the simulator validates Kindlet loading, instantiation, and clean lifecycle destruction without requiring an X11/Wayland display server:

```bash
kindle-sdk emulate path/to/my-app.azw2 --headless
```

Output:
```text
Simulator loading Kindlet: com.amazon.kindle.demo.DemoKindlet (600x800)
Simulator headless execution verified: Kindlet started successfully!
Simulator headless execution verified: Kindlet stopped and destroyed cleanly!
```
