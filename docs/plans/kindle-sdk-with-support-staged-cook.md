# Kindlet C++ Showcase Application Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a production-grade example application in `examples/kindlet-cpp-showcase/` that wraps a modern C++ engine inside an official Amazon Kindlet (Java CDC 1.1 / PBP 1.1), demonstrating all Kindlet platform capabilities.

**Architecture:**
- **Host Kindlet App (Java PBP 1.1):** Implements canonical lifecycle (`create`, `start`, `stop`, `destroy`), lightweight high-contrast E-Ink widgets (`KComponent`, `KLabel`, `KProgressIndicator`, `KMenu`, `KMenuItem`), custom 16-level grayscale AWT drawing canvas, keyboard/D-pad event handling, orientation toggling (`context.setOrientation()`), and isolated persistent file I/O (`context.getHomeDirectory()`).
- **Native Process Engine (C++17/20):** Supervised via `NativeProcessSupervisor` and `RuntimeProcessLauncher` (CDC 1.1 compliant), communicating over length-prefixed stdin/stdout IPC streams (`NativeMessage` / `IpcChannel`). Computes E-Ink grayscale pixel buffers, queries SoC hardware (i.MX353 on K3 vs i.MX31 on DX) and memory info (`MemoryInfo`), and serializes JSON telemetry.
- **Packaging & Security:** Bundles the cross-compiled ARMv6 native binary into JAR resources at `/bin/armv6/showcase_daemon`, builds with Java 1.4 bytecode emission (`-source 1.4 -target 1.4`), signs with triple developer keys (`dk`, `di`, `dn`), and packages into `.azw2`.

**Tech Stack:**
- Java 8 toolchain targeting Java 1.4 bytecode (major version 48) with Apache Ant
- Modern C++17/20 with GCC `arm-linux-gnueabi` cross-compiler and host compiler
- CMake build system
- Python 3 `kindle-sdk` CLI for keystore generation, signing, and packaging
- Kindle Desktop E-Ink Simulator (`kindle-sdk emulate`)

---

## Proposed File Structure

```text
examples/kindlet-cpp-showcase/
├── README.md
├── build.xml
├── CMakeLists.txt
├── src/
│   └── main/
│       └── java/
│           └── com/
│               └── amazon/
│                   └── kindle/
│                       └── showcase/
│                           ├── ShowcaseKindlet.java          # Canonical lifecycle, UI layout, supervisor, keys
│                           ├── ShowcaseDashboardCanvas.java   # Custom 16-level grayscale E-Ink AWT canvas
│                           ├── ShowcaseSettingsManager.java  # Persistent I/O in context.getHomeDirectory()
│                           └── TelemetryData.java            # Hardware and benchmark telemetry model
├── native/
│   ├── CMakeLists.txt
│   └── src/
│       ├── showcase_daemon.cpp                              # Native daemon reading IPC stream and responding
│       └── system_telemetry.cpp                             # Probes SoC, RAM, and generates E-Ink bitmap data
└── scripts/
    └── build-and-run.sh                                     # Automated cross-compile, sign, package, and emulate
tests/
└── integration/
    └── test_showcase_app.py                                 # Automated integration test validating showcase build & emulation
```

---

### Task 1: Native C++ Showcase Daemon & Telemetry Engine

**Files:**
- Create: `examples/kindlet-cpp-showcase/native/src/system_telemetry.hpp`
- Create: `examples/kindlet-cpp-showcase/native/src/system_telemetry.cpp`
- Create: `examples/kindlet-cpp-showcase/native/src/showcase_daemon.cpp`
- Create: `examples/kindlet-cpp-showcase/native/CMakeLists.txt`
- Create: `examples/kindlet-cpp-showcase/CMakeLists.txt`

**Interfaces:**
- Consumes: `native/include/kindle/ipc.hpp`, `native/include/kindle/hardware.hpp`, `native/include/kindle/target.hpp`, `native/include/kindle/eink.hpp`.
- Produces: `showcase_daemon` executable capable of handling `Ping`, `Command` ("get_telemetry", "generate_pattern"), and `Shutdown`.

- [ ] **Step 1: Implement `system_telemetry.hpp` and `system_telemetry.cpp`**
  - Query CPU model, SoC type, and RAM metrics (`MemoryInfo`).
  - Generate a 16-level grayscale dithered test pattern buffer (e.g. 200x120 grayscale array) representing E-Ink graphical computation in C++.

- [ ] **Step 2: Implement `showcase_daemon.cpp`**
  - Loop on `IpcChannel::read_message(std::cin, in_msg)`.
  - Handle `MessageType::Ping` -> return `MessageType::Pong`.
  - Handle `MessageType::Command` -> parse JSON action (`get_telemetry`, `generate_pattern`, `echo`) and return `MessageType::Response` with JSON / binary payload.
  - Handle `MessageType::Shutdown` -> clean exit.

- [ ] **Step 3: Create CMake configuration files**
  - Build `showcase_daemon` supporting both host native build (for desktop simulation/testing) and ARMv6 cross-compilation (`KindleArmv6.cmake`).

- [ ] **Step 4: Verify C++ build and test execution**
  - Compile `showcase_daemon` and run a quick verification with piped IPC messages.

---

### Task 2: Java Showcase Kindlet Application & UI Architecture

**Files:**
- Create: `examples/kindlet-cpp-showcase/src/main/java/com/amazon/kindle/showcase/TelemetryData.java`
- Create: `examples/kindlet-cpp-showcase/src/main/java/com/amazon/kindle/showcase/ShowcaseSettingsManager.java`
- Create: `examples/kindlet-cpp-showcase/src/main/java/com/amazon/kindle/showcase/ShowcaseDashboardCanvas.java`
- Create: `examples/kindlet-cpp-showcase/src/main/java/com/amazon/kindle/showcase/ShowcaseKindlet.java`

**Interfaces:**
- Consumes: `java/kindlet-api` (`Kindlet`, `AbstractKindlet`, `KindletContext`, `KLabel`, `KProgressIndicator`, `KMenu`, `KMenuItem`) and `java/kindlet-bridge` (`NativeProcessSupervisor`, `RuntimeProcessLauncher`, `NativeMessage`, `NativeBridgeListener`).
- Produces: Complete Kindlet demonstrating all platform capabilities.

- [ ] **Step 1: Implement `TelemetryData.java` and `ShowcaseSettingsManager.java`**
  - `TelemetryData`: Data holder for SoC, CPU architecture, total/free RAM, IPC latency, and render time.
  - `ShowcaseSettingsManager`: Reads and writes properties file to `context.getHomeDirectory() / showcase.properties`, preserving run count and user preferences.

- [ ] **Step 2: Implement `ShowcaseDashboardCanvas.java`**
  - Custom AWT lightweight component drawing 16-level grayscale patterns, battery status bar, telemetry stats, and lifecycle event history.
  - High-contrast typography and E-Ink friendly borders.

- [ ] **Step 3: Implement `ShowcaseKindlet.java`**
  - Implement `Kindlet`, `KeyListener`, and `NativeBridgeListener`.
  - `create(KindletContext)`:
    - Initialize UI layout: Top title `KLabel`, center `ShowcaseDashboardCanvas`, bottom `KProgressIndicator` and status `KLabel`.
    - Setup `KMenu`: "Query C++ Engine", "Cycle E-Ink Pattern", "Toggle Orientation", "Save Settings".
    - Initialize `NativeProcessSupervisor` with `RuntimeProcessLauncher` and extract embedded daemon.
  - `start()`:
    - Launch native daemon via `supervisor.start()`.
    - Send initial `PING` and telemetry query `COMMAND`.
  - Key handling:
    - 5-way D-Pad (Up, Down, Left, Right, Select) to navigate options.
    - PageUp/PageDown to toggle orientation or refresh display.
  - `stop()` / `destroy()`:
    - Gracefully terminate native process and flush settings.

---

### Task 3: Ant Build, Resource Embedding & Packaging

**Files:**
- Create: `examples/kindlet-cpp-showcase/build.xml`
- Create: `examples/kindlet-cpp-showcase/scripts/build-and-run.sh`

**Interfaces:**
- Consumes: `java/kindlet-api/dist/kindlet-api.jar`, `java/kindlet-bridge/dist/kindlet-bridge.jar`, and compiled `showcase_daemon`.
- Produces: `dist/kindlet-cpp-showcase.jar` and `dist/kindlet-cpp-showcase.azw2`.

- [ ] **Step 1: Create `build.xml`**
  - Configure compilation targeting Java 1.4 bytecode (`-source 1.4 -target 1.4`).
  - Bundle `kindlet-bridge` classes into the target JAR.
  - Copy compiled `showcase_daemon` into `bin/armv6/showcase_daemon` inside the JAR.
  - Generate manifest with `Main-Class: com.amazon.kindle.showcase.ShowcaseKindlet` and Amazon headers.

- [ ] **Step 2: Create `scripts/build-and-run.sh`**
  - Compile C++ daemon.
  - Run Ant to compile Java and bundle JAR.
  - Generate developer keystore with `kindle-sdk sign` (or `python3 -m kindle_sdk.cli`).
  - Package `.azw2` container.
  - Launch inside desktop simulator (`kindle-sdk emulate`).

---

### Task 4: Documentation & Automated Integration Testing

**Files:**
- Create: `examples/kindlet-cpp-showcase/README.md`
- Create: `tests/integration/test_showcase_app.py`
- Modify: `docs/README.md`

**Interfaces:**
- Consumes: The complete showcase application.
- Produces: End-to-end automated test in CI and comprehensive guide for users.

- [ ] **Step 1: Write integration test `tests/integration/test_showcase_app.py`**
  - Test building the showcase C++ daemon.
  - Test compiling the Java Kindlet via Ant.
  - Test packaging and signing the `.azw2` archive.
  - Test headless simulator execution verifying `create -> start -> stop -> destroy`.

- [ ] **Step 2: Create `examples/kindlet-cpp-showcase/README.md`**
  - Detailed architecture walkthrough.
  - Step-by-step instructions for compiling, signing, testing on desktop simulator, and deploying to physical Kindle 3G / DX over USB.

- [ ] **Step 3: Update `docs/README.md`**
  - Add pointer to `examples/kindlet-cpp-showcase/` in the documentation index.

- [ ] **Step 4: Execute test suite and verify**
  - Run `python3 -m unittest tests/integration/test_showcase_app.py`.
  - Run `./scripts/run-tests.sh`.

---

## Verification Plan

1. **Native C++ Daemon Verification:**
   - Execute `showcase_daemon` with echo/ping messages via stdin/stdout pipe; verify valid JSON response.
2. **Ant Java Compilation Verification:**
   - Execute `ant -f examples/kindlet-cpp-showcase/build.xml jar` with OpenJDK 8; verify class files are major version 48 (Java 1.4 bytecode).
3. **Triple Signing & AZW2 Verification:**
   - Sign with `dk`, `di`, `dn` aliases; inspect `META-INF/` signature entries; ensure `jarsigner -verify` passes.
4. **Simulator Headless Verification:**
   - Run `kindle-sdk emulate examples/kindlet-cpp-showcase/dist/kindlet-cpp-showcase.azw2 --headless`; ensure all lifecycle phases execute cleanly without unhandled exceptions.
5. **Full Regression Test Suite:**
   - Run `./scripts/run-tests.sh` and `./scripts/verify-sdk.sh`.
