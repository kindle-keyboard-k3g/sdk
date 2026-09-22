# Kindle SDK with Modern C++ & Kindlet (J2ME CDC/PBP) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a production-grade Kindle SDK with official Amazon Kindlet (J2ME CDC 1.1 / PBP 1.1) support, modern C++ (C++17/20) cross-compilation for Kindle 3G / Keyboard (K3) and Kindle DX/DXG, Dockerized build toolchains, verified E-Ink and SoC hardware abstractions, triple-key code signing (`dk`, `di`, `dn`), native-in-Kindlet IPC process supervisor & optional JNI bridge, desktop E-ink emulator, and end-to-end TDD testing.

**Architecture:** 
- **Kindlet Engine:** Java CDC 1.1 / Personal Basis Profile (PBP 1.1) API stubs matching Amazon's official runtime (`com.amazon.kindle.kindlet.*`), strictly implementing lifecycle methods `create(KindletContext)`, `start()`, `stop()`, `destroy()`, compiled targeting Java 1.4 bytecode (major version 48).
- **Modern C++ Core:** ARMv6 (`-march=armv6j -mtune=arm1136jf-s -mfloat-abi=softfp`) cross-compiled C++17/20 static binaries targeting Linux 2.6 kernels on Freescale i.MX353 (K3) and Freescale i.MX31 (DX/DXG) SoCs, with direct E-Ink framebuffer (`/dev/fb0`, `FBIO_EINK_UPDATE_DISPLAY`, `/proc/eink_fb/update_display`) and evdev input abstractions.
- **Kindlet-to-C++ Bridge:** Supervised native process execution using `Runtime.getRuntime().exec` (CDC 1.1 compliant) and optional JNI bridge bundled inside `.azw2` packages with framed, length-prefixed stdin/stdout IPC streams.
- **Security & Packaging:** Triple-certificate signing engine supporting historical alias roles (`dk*`, `di*`, `dn*`), generating ephemeral keystores for tests (never committing private keys), supporting RSA-2048 with SHA256withRSA (K3 3.4.3+) and legacy algorithms, producing validated `.azw2` packages.
- **Emulation & Tooling:** Desktop AWT/Swing Kindle simulator with E-Ink 4-bit/8-bit grayscale quantization and refresh flashing, orchestrated via a unified `kindle-sdk` CLI.

**Tech Stack:**
- Java 8 toolchain targeting Java 1.4 bytecode (major 48) with Ant
- C++17 / C++20 with GCC `arm-linux-gnueabi` cross-compiler
- Docker & Docker Compose
- Python 3.10+ (CLI tooling & packaging)
- QEMU user mode (`qemu-arm-static`) for cross-architecture TDD
- JUnit 4 / Catch2 for unit testing

**Spec:** `docs/sot/j2me-kindlet.md`

## Global Constraints

- **Device targets:**
  - Kindle Keyboard (K3 / K3G, Freescale i.MX353 ARMv6 @ 532MHz, 256MB RAM, 600x800 Pearl, Linux 2.6.26-rt-lab126, glibc 2.5).
  - Kindle DX / DXG (Freescale i.MX31 / MCIMX31L ARMv6 @ 400MHz, 128MB RAM, 824x1200 Vizplex/Pearl, Linux 2.6.22, glibc 2.5).
- **Native ABI:** ARMv6 softfp baseline (`-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp`), statically linked (`-static`, `-static-libgcc`, `-static-libstdc++`) with glibc 2.5 compatibility.
- **Kindlet bytecode:** Java 1.4 bytecode (major 48) targeting Sun CVM / CDC 1.1 / Personal Basis Profile 1.1.
- **Kindlet Lifecycle:** `create(KindletContext)`, `start()`, `stop()`, `destroy()` (never Android-style `onCreate`/`onStart`).
- **Process execution:** CDC 1.1 compliant `Runtime.getRuntime().exec` (Java 5 `ProcessBuilder` does not exist in CDC 1.1).
- **Security & Signing:**
  - Support `dk*`, `di*`, `dn*` alias roles with RSA-2048 / SHA256withRSA for K3 and legacy algorithms.
  - **Zero credential commitment:** Never commit `.keystore`, `.p12`, private keys, or passwords. Generate ephemeral keystores in tests.
  - Add `*.keystore`, `*.p12`, `*.azw2`, `build/` to `.gitignore`.
- **E-Ink IOCTLs:** Support legacy ioctls `FBIO_EINK_UPDATE_DISPLAY` (0x46db), `FBIO_EINK_UPDATE_DISPLAY_AREA` (0x46dd), `FBIO_EINK_CLEAR_SCREEN` (0x46e1), and `/proc/eink_fb/update_display`.
- **Repository Paths:** All paths in the plan are repository-relative (e.g. `docs/...`, `native/...`, `java/...`, `python/...`).

---

### Task 1: Repository Foundation, Target Profiles & Documentation

**Files:**
- Create: `pyproject.toml`
- Create: `profiles/schema.json`
- Create: `profiles/k3.json`
- Create: `profiles/dx.json`
- Create: `docs/architecture.md`
- Create: `docs/kindlet-guide.md`
- Create: `docs/modern-cpp-guide.md`
- Create: `docs/signing-and-deployment.md`
- Modify: `.gitignore`
- Modify: `README.md`
- Test: `python/tests/test_profiles.py`

**Interfaces:**
- Consumes: Specifications from `docs/sot/j2me-kindlet.md`.
- Produces: Definitive hardware reference, target profiles (`k3`, `dx`), gitignore security protections, and validated profile parser.

- [ ] **Step 1: Write failing profile schema validation test `python/tests/test_profiles.py`**
  - Verify loading profiles for `k3` (i.MX353, 256MB, 600x800, ARMv6) and `dx` (i.MX31, 128MB, 824x1200, ARMv6).
  - Verify rejection of invalid dimensions, unknown SoCs, and negative memory values.

- [ ] **Step 2: Update `.gitignore` with strict security rules**
  - Add `*.keystore`, `*.p12`, `*.key`, `*.azw2`, `artifacts/`, `build/`, `.venv/`, `__pycache__/`.

- [ ] **Step 3: Create `profiles/schema.json`, `profiles/k3.json`, and `profiles/dx.json`**
  - Define CPU, SoC, RAM limit, framebuffer dimensions, pixel formats (`gray4`, `gray8`, `gray16`), and legacy update ioctls.

- [ ] **Step 4: Create `pyproject.toml` and implement profile loader in `python/src/kindle_sdk/profiles.py`**
  - Run pytest to verify tests pass.

- [ ] **Step 5: Write architectural documentation in `docs/` and update `README.md`**
  - Document SoC matrix (i.MX353 vs i.MX31), display formats (4bpp/8bpp/16bpp), CVM profile, signing aliases (`dk`/`di`/`dn`), and SDK roadmap.

- [ ] **Step 6: Commit**
```bash
git add pyproject.toml profiles/ docs/ README.md .gitignore python/
git commit -m "feat(foundation): initialize target profiles, security gitignore, and architectural documentation"
```

---

### Task 2: Docker Build Environments for J2ME and Modern C++

**Files:**
- Create: `docker/j2me/Dockerfile`
- Create: `docker/j2me/entrypoint.sh`
- Create: `docker/j2me/verify-toolchain.sh`
- Create: `docker/native/Dockerfile`
- Create: `docker/native/entrypoint.sh`
- Create: `docker/native/verify-toolchain.sh`
- Create: `docker/compose.yaml`
- Test: `tests/docker/test_docker_builds.sh`

**Interfaces:**
- Consumes: Source code trees mounted into containers.
- Produces: Hermetic build images `kindle-j2me-builder` and `kindle-cpp-builder`.

- [ ] **Step 1: Create `docker/j2me/Dockerfile`**
  - Base: `eclipse-temurin:8-jdk-focal`.
  - Install Ant 1.9+, Python 3, OpenSSL, Java keytool/jarsigner.
  - Configure ECJ / Ant flags targeting Java 1.4 bytecode emission (`-source 1.4 -target 1.4`).

- [ ] **Step 2: Create `docker/native/Dockerfile`**
  - Base: `ubuntu:22.04`.
  - Install `gcc-arm-linux-gnueabi`, `g++-arm-linux-gnueabi`, CMake, Ninja, Make, `qemu-user-static`.
  - Install host `g++`, Catch2, and testing tools.

- [ ] **Step 3: Create `docker/compose.yaml` and entrypoints**
  - Define services `j2me-builder` and `native-builder` with `TZ=America/Sao_Paulo`.

- [ ] **Step 4: Create verification script `tests/docker/test_docker_builds.sh`**
  - Verify that `javac`, `ant`, `jarsigner`, `arm-linux-gnueabi-g++`, `cmake`, and `qemu-arm-static` are accessible.

- [ ] **Step 5: Commit**
```bash
git add docker/ tests/docker/
git commit -m "feat(docker): add hermetic j2me and modern cpp docker build containers"
```

---

### Task 3: Official Amazon Kindlet API Stubs & Framework

**Files:**
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/Kindlet.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/AbstractKindlet.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/KindletContext.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/KindletException.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/ui/KComponent.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/ui/KLabel.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/ui/KMenu.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/ui/KMenuItem.java`
- Create: `java/kindlet-api/src/main/java/com/amazon/kindle/kindlet/ui/KProgressIndicator.java`
- Create: `java/kindlet-api/build.xml`
- Test: `java/kindlet-api/src/test/java/com/amazon/kindle/kindlet/KindletLifecycleTest.java`

**Interfaces:**
- Consumes: Java PBP 1.1 / CDC 1.1 AWT subset (`java.awt.Container`, `java.awt.Graphics`).
- Produces: `kindlet-api.jar` with Java 1.4 bytecode compatibility (major version 48).

- [ ] **Step 1: Write failing lifecycle unit test `KindletLifecycleTest.java`**
  - Test invocation sequence: `create(KindletContext)` -> `start()` -> `stop()` -> `destroy()`.
  - Validate state transitions and context persistence.

- [ ] **Step 2: Implement core Kindlet interfaces and classes**
  - `Kindlet.java`: `void create(KindletContext context)`, `void start()`, `void stop()`, `void destroy()`.
  - `AbstractKindlet.java`: default no-op implementations.
  - `KindletContext.java`: `Container getRootContainer()`, `File getHomeDirectory()`, `int getOrientation()`.
  - `KindletException.java`.

- [ ] **Step 3: Implement Kindlet UI components**
  - `KComponent.java`, `KLabel.java`, `KMenu.java`, `KMenuItem.java`, `KProgressIndicator.java`.

- [ ] **Step 4: Create Ant build script `java/kindlet-api/build.xml`**
  - Target `-source 1.4 -target 1.4` bytecode emission, package into `dist/kindlet-api.jar`.

- [ ] **Step 5: Run tests & Commit**
```bash
git add java/kindlet-api/
git commit -m "feat(kindlet): add official kindlet pbp 1.1 api stubs and lifecycle test suite"
```

---

### Task 4: Security, Code Signing Engine & `.azw2` Packaging

**Files:**
- Create: `python/src/kindle_sdk/signing/keystore.py`
- Create: `python/src/kindle_sdk/signing/jarsigner.py`
- Create: `python/src/kindle_sdk/signing/verification.py`
- Create: `python/src/kindle_sdk/packaging/manifest.py`
- Create: `python/src/kindle_sdk/packaging/azw2.py`
- Test: `python/tests/test_signing.py`
- Test: `python/tests/test_manifest.py`
- Test: `python/tests/test_packaging.py`

**Interfaces:**
- Consumes: JAR archives, application metadata, signing configuration.
- Produces: Triple-signed `.azw2` packages validated with `jarsigner -verify`.

- [ ] **Step 1: Write failing manifest and signing tests**
  - Test `test_manifest.py`: verify required headers (`Main-Class`, `Extension-List: SDK`, `SDK-Extension-Name: com.amazon.kindle.kindlet`, `SDK-Specification-Version: 2.1`, trailing newline, no line-wrap errors).
  - Test `test_signing.py`: generate ephemeral keystore with `dk`, `di`, `dn` aliases; sign JAR; verify with `jarsigner -verify`.
  - Test `test_packaging.py`: build `.azw2` archive and ensure binary integrity.

- [ ] **Step 2: Implement `keystore.py`**
  - Support ephemeral RSA-2048 with SHA256withRSA key generation (K3 3.4.3+) and legacy key generation.
  - Create alias roles (`dk*`, `di*`, `dn*`).
  - Never save default credentials to tracked files.

- [ ] **Step 3: Implement `jarsigner.py` and `verification.py`**
  - Sequentially sign JAR with `dk`, `di`, `dn` aliases.
  - Verify signatures and return detailed diagnostic reports.

- [ ] **Step 4: Implement `manifest.py` and `azw2.py`**
  - Construct valid manifests and package signed JAR into `.azw2`.

- [ ] **Step 5: Run tests & Commit**
```bash
git add python/src/kindle_sdk/signing/ python/src/kindle_sdk/packaging/ python/tests/
git commit -m "feat(security): implement dk di dn triple code signing and azw2 packaging engine"
```

---

### Task 5: Modern C++ Hardware, E-Ink & Input Abstraction Library

**Files:**
- Create: `native/include/kindle/target.hpp`
- Create: `native/include/kindle/hardware.hpp`
- Create: `native/include/kindle/eink.hpp`
- Create: `native/include/kindle/input.hpp`
- Create: `native/src/hardware.cpp`
- Create: `native/src/eink.cpp`
- Create: `native/src/input.cpp`
- Create: `native/platform/linux/procfs.cpp`
- Create: `native/platform/linux/framebuffer_linux.cpp`
- Create: `native/platform/fake/fake_procfs.cpp`
- Create: `native/platform/fake/fake_eink.cpp`
- Create: `native/CMakeLists.txt`
- Create: `native/cmake/KindleArmv6.cmake`
- Test: `native/tests/test_hardware.cpp`
- Test: `native/tests/test_eink.cpp`
- Test: `native/tests/test_input.cpp`

**Interfaces:**
- Consumes: Target profiles and Linux `/dev/fb0`, `/proc/eink_fb/update_display`, `/dev/input/event0` (or fakes in tests).
- Produces: `libkindle_native.a` modern C++17/20 library.

- [ ] **Step 1: Write failing C++ tests for hardware detection and memory**
  - Test identifying i.MX353 (K3) and i.MX31 (DX) from fake `/proc/cpuinfo`.
  - Test RAM thresholds (256MB on K3, 128MB on DX).

- [ ] **Step 2: Implement `hardware.hpp` and `hardware.cpp`**
  - Model detection enum (`KindleKeyboard`, `KindleDX`, `Unknown`), memory monitoring, and CPU info queries.

- [ ] **Step 3: Write failing C++ tests for E-Ink pixel formats and updates**
  - Test 4bpp (16 grayscale levels), 8bpp, and 16bpp pixel buffers.
  - Test coordinate translation for 600x800 (K3) and 824x1200 (DX).
  - Test update modes (`PARTIAL`, `FULL`, `FLASH`) and ioctl calls (`FBIO_EINK_UPDATE_DISPLAY`).

- [ ] **Step 4: Implement `eink.hpp`, `eink.cpp`, and platform backends**
  - Framebuffer mmap and ioctls with fake backend for desktop testing.

- [ ] **Step 5: Write failing C++ tests for input and implement `input.hpp`/`input.cpp`**
  - Decode evdev events: 5-way D-Pad (Up, Down, Left, Right, Select), Page Turn, and Keyboard buttons.

- [ ] **Step 6: Run C++ test suite & Commit**
```bash
git add native/
git commit -m "feat(native): implement modern cpp hardware, eink, and input library with tdd"
```

---

### Task 6: Native-in-Kindlet Process Supervisor & IPC Protocol Bridge

**Files:**
- Create: `docs/sot/bridge-protocol.md`
- Create: `native/include/kindle/ipc.hpp`
- Create: `native/src/ipc.cpp`
- Create: `native/include/kindle/jni_bridge.hpp`
- Create: `native/src/jni_bridge.cpp`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/ProcessLauncher.java`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/RuntimeProcessLauncher.java`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/NativeProcessSupervisor.java`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/NativeMessage.java`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/NativeBridgeListener.java`
- Create: `java/kindlet-bridge/src/main/java/com/amazon/kindle/bridge/NativeJniBridge.java`
- Create: `java/kindlet-bridge/build.xml`
- Test: `native/tests/test_ipc.cpp`
- Test: `java/kindlet-bridge/src/test/java/com/amazon/kindle/bridge/NativeProcessSupervisorTest.java`

**Interfaces:**
- Consumes: Bundled native binary inside JAR resources.
- Produces: Length-prefixed binary/JSON IPC channel between CDC 1.1 Kindlet and C++ binary.

- [ ] **Step 1: Write failing C++ and Java framing tests**
  - Test length-prefixed packet framing: magic header (`0x4B494E44`), message type, request ID, payload length, payload.
  - Test malformed packet rejection and oversized payload protection.

- [ ] **Step 2: Implement C++ IPC channel in `native/src/ipc.cpp`**
  - Thread-safe stdin reader and stdout message writer.

- [ ] **Step 3: Implement Java `ProcessLauncher` and `RuntimeProcessLauncher`**
  - Use `Runtime.getRuntime().exec(String[])` to comply with Java CDC 1.1 / PBP 1.1 baseline.

- [ ] **Step 4: Implement `NativeProcessSupervisor.java`**
  - Extract embedded native binary from JAR resource to private app folder.
  - Validate permissions and launch child process via `ProcessLauncher`.
  - Handle crash restarts and graceful shutdown on `Kindlet.stop()` / `destroy()`.

- [ ] **Step 5: Implement `NativeJniBridge.java` and `jni_bridge.cpp`**
  - Capability-detected optional JNI bridge with byte-array transaction API.

- [ ] **Step 6: Run tests & Commit**
```bash
git add docs/sot/bridge-protocol.md native/src/ipc.cpp native/include/kindle/ipc.hpp native/include/kindle/jni_bridge.hpp native/src/jni_bridge.cpp java/kindlet-bridge/
git commit -m "feat(bridge): implement native-in-kindlet process supervisor and ipc bridge"
```

---

### Task 7: Desktop Kindle Simulator & E-Ink Emulation Window

**Files:**
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/KindleSimulatorWindow.java`
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/SimulatorKindletContext.java`
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/EinkScreenPanel.java`
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/KeypadPanel.java`
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/EinkQuantizer.java`
- Create: `java/emulator/src/main/java/com/amazon/kindle/emulator/EmulatorLauncher.java`
- Create: `java/emulator/build.xml`
- Test: `java/emulator/src/test/java/com/amazon/kindle/emulator/EinkQuantizerTest.java`

**Interfaces:**
- Consumes: Target Kindlet JAR and profiles (`k3`, `dx`).
- Produces: Interactive desktop window and headless test runner with 4bpp/8bpp grayscale quantization and refresh flashing.

- [ ] **Step 1: Write failing unit test `EinkQuantizerTest.java`**
  - Verify 24-bit RGB conversion into 16-level grayscale palette (`gray4`) and `gray8`.
  - Test full-refresh flash inversion cycles.

- [ ] **Step 2: Implement `EinkQuantizer.java` and `EinkScreenPanel.java`**
  - Image dithering filter and flash animation for simulated screen refreshes.

- [ ] **Step 3: Implement `SimulatorKindletContext.java`**
  - Provide simulator container, home directory, and orientation properties.

- [ ] **Step 4: Implement `KindleSimulatorWindow.java`, `KeypadPanel.java`, and `EmulatorLauncher.java`**
  - Swing/AWT desktop window with Kindle bezels, 5-way D-Pad, keyboard, and headless CLI launch mode.

- [ ] **Step 5: Run tests & Commit**
```bash
git add java/emulator/
git commit -m "feat(emulator): implement desktop e-ink simulator with grayscale quantization and keypad"
```

---

### Task 8: Unified CLI Tool (`kindle-sdk`) & Starter Templates

**Files:**
- Create: `python/src/kindle_sdk/cli.py`
- Create: `python/src/kindle_sdk/config.py`
- Create: `python/src/kindle_sdk/project/init.py`
- Create: `python/src/kindle_sdk/toolchains.py`
- Create: `templates/pure-kindlet/`
- Create: `templates/native-process-kindlet/`
- Create: `templates/native-jni-kindlet/`
- Create: `templates/standalone-native/`
- Test: `python/tests/test_cli.py`
- Test: `tests/integration/test_end_to_end.py`

**Interfaces:**
- Consumes: CLI commands (`init`, `doctor`, `build`, `test`, `sign`, `package`, `emulate`).
- Produces: Project initialization, builds, and packaging workflows.

- [ ] **Step 1: Write failing CLI tests `test_cli.py`**
  - Test CLI subcommands: `doctor`, `init --template pure-kindlet`, `build`, `sign`, `package`.

- [ ] **Step 2: Implement CLI commands in `cli.py`, `config.py`, and `init.py`**
  - Handle safe project scaffolding without overwriting non-empty directories.
  - Implement toolchain doctor checking Java, Ant, CMake, cross-compiler, and QEMU.

- [ ] **Step 3: Implement starter templates**
  - `pure-kindlet`: Pure Java PBP 1.1 Kindlet application with custom Kindle UI widgets.
  - `native-process-kindlet`: Kindlet wrapping modern C++ engine via IPC pipe.
  - `native-jni-kindlet`: Kindlet interacting with C++ via JNI.
  - `standalone-native`: Pure C++ daemon writing directly to `/dev/fb0`.

- [ ] **Step 4: Run CLI tests & Commit**
```bash
git add python/src/kindle_sdk/ templates/ tests/integration/
git commit -m "feat(cli): implement kindle-sdk cli and starter project templates"
```

---

### Task 9: CI, End-to-End Verification Suite & Security Scanners

**Files:**
- Create: `scripts/run-tests.sh`
- Create: `scripts/verify-sdk.sh`
- Create: `scripts/check-no-private-keys.sh`
- Modify: `README.md`

**Interfaces:**
- Consumes: Complete SDK codebase, templates, and build toolchains.
- Produces: Automated verification report and security gate preventing private key leaks.

- [ ] **Step 1: Implement security check `scripts/check-no-private-keys.sh`**
  - Scans working tree for committed private keys, `.keystore`, `.p12`, or hard-coded secrets.

- [ ] **Step 2: Implement test runner `scripts/run-tests.sh`**
  - Runs Python tests (`pytest`), Java tests (`ant test`), and C++ tests (`ctest`).

- [ ] **Step 3: Implement end-to-end verification `scripts/verify-sdk.sh`**
  - Scaffolds a new project from `native-process-kindlet`.
  - Builds C++ binary with cross-toolchain.
  - Compiles Kindlet with Java 1.4 bytecode.
  - Generates ephemeral test keystore, signs with `dk`, `di`, `dn`, and packages `.azw2`.
  - Runs headlessly in the desktop simulator.

- [ ] **Step 4: Execute verification scripts & Commit**
```bash
git add scripts/ README.md
git commit -m "test(sdk): add comprehensive end-to-end test suite and secret verification scanner"
```

---

## Verification Plan

### Automated Tests
1. **Python Unit Tests:**
   - Command: `python3 -m pytest python/tests/ -v`
   - Validates: Profile loading, manifest generation, keystore generator, `jarsigner` wrapper, CLI subcommands.
2. **Java Unit Tests:**
   - Command: `ant -f java/kindlet-api/build.xml test` and `ant -f java/emulator/build.xml test`
   - Validates: Kindlet lifecycle, event routing, grayscale quantizer algorithm.
3. **C++ Unit Tests:**
   - Command: `cd native/build && ctest --output-on-failure`
   - Validates: SoC detection, RAM thresholds, E-ink update ioctls, evdev input parsing, IPC framing.
4. **Security & Cleanliness Check:**
   - Command: `./scripts/check-no-private-keys.sh`
   - Validates: Zero committed private keys, certificates, or `.keystore` files.
5. **End-to-End Integration:**
   - Command: `./scripts/verify-sdk.sh`
   - Validates: Complete build-sign-package-emulate lifecycle for a sample project.
