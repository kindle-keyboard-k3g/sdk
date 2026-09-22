# Kindle SDK: Kindlet C++ Showcase Application

A reference application demonstrating how to embed and supervise a modern C++17 native engine inside an official Amazon Kindlet (Java J2ME CDC 1.1 / Personal Basis Profile 1.1).

Designed specifically for the Kindle Keyboard (K3 / K3G / K3W) and Kindle DX / DX Graphite.

---

## Architecture Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                 Amazon Kindlet UI (Java)                   │
│                                                             │
│  - AbstractKindlet Lifecycle (create, start, stop, destroy) │
│  - Custom 16-level Grayscale E-Ink AWT Dashboard Canvas     │
│  - Keyboard / 5-way D-pad Navigation                        │
│  - Screen Orientation Control (Portrait / Landscape)        │
│  - Isolated Persistent Storage in context.getHomeDirectory()│
└──────────────────────────────┬──────────────────────────────┘
                               │
               Length-prefixed binary IPC stream
                  (KIND magic, stdio pipes)
                               │
┌──────────────────────────────▼──────────────────────────────┐
│             Native C++ Engine (showcase_daemon)             │
│                                                             │
│  - Supervised via RuntimeProcessLauncher                    │
│  - Dynamic SoC & Hardware Detection (/proc probe)           │
│  - RAM Capacity & Available Memory Gauge                    │
│  - 16-Level Grayscale E-Ink Test Pattern Computation        │
│  - Bidirectional Command / Response Dispatcher              │
└─────────────────────────────────────────────────────────────┘
```

---

## Capabilities Demonstrated

1. **Kindlet Lifecycle Compliance:**
   Strictly adheres to Amazon's `create(KindletContext)`, `start()`, `stop()`, and `destroy()` flow.
2. **Bytecode Portability:**
   Compiled targeting Java 1.4 bytecode (`-source 1.4 -target 1.4`, major version 48) using Java 8 toolchains, running smoothly on the Amazon Kindle JVM.
3. **C++ Native Process Supervision:**
   Extracts `showcase_daemon` from the JAR to the Kindlet's private home directory at runtime, sets executable permissions, and streams messages over standard I/O.
4. **Binary Wire Framing:**
   Packets formatted with `KIND` magic header, message type (`PING`, `PONG`, `COMMAND`, `RESPONSE`, `SHUTDOWN`), 32-bit request correlation IDs, and length prefixes.
5. **E-Ink Grayscale Rendering:**
   C++ calculates test patterns (stepped horizontal bars, radial gradients, dithered checkerboards) which the Java Kindlet draws on a custom high-contrast canvas.
6. **Triple Developer Signing:**
   Signs packages with Amazon's required triple-key developer aliases (`dkDeveloper`, `diDeveloper`, `dnDeveloper`) into production `.azw2` containers.

---

## Directory Structure

```text
examples/kindlet-cpp-showcase/
├── CMakeLists.txt                 # C++ root build definition
├── README.md                      # Architecture and instructions
├── build.xml                      # Apache Ant configuration (Java 1.4 targets)
├── native/
│   ├── CMakeLists.txt             # Native daemon executable targets
│   └── src/
│       ├── showcase_daemon.cpp    # IPC loop and request dispatcher
│       ├── system_telemetry.cpp   # Hardware, RAM, and pattern engine
│       └── system_telemetry.hpp   # Telemetry headers
├── scripts/
│   └── build-and-run.sh           # Automated end-to-end build & simulation
└── src/
    └── main/
        └── java/
            └── com/
                └── amazon/
                    └── kindle/
                        └── showcase/
                            ├── ShowcaseDashboardCanvas.java  # Custom E-Ink canvas
                            ├── ShowcaseKindlet.java          # Canonical Kindlet
                            ├── ShowcaseSettingsManager.java  # Persistent I/O
                            └── TelemetryData.java            # Telemetry model
```

---

## Building and Running

### 1. Automated Build & Emulation
To compile C++, build Java bytecode, bundle the JAR, sign, package into `.azw2`, and run the headless smoke test:

```bash
./scripts/build-and-run.sh
```

To run with the desktop interactive simulator window:

```bash
./scripts/build-and-run.sh --gui
```

### 2. Manual Step-by-Step Build

#### Step A: Compile C++ Native Daemon
```bash
mkdir -p build && cd build
cmake ..
make showcase_daemon
cd ..
```

#### Step B: Compile Java Kindlet and Assemble JAR
```bash
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH
ant jar
```

#### Step C: Triple Code-Sign and Package AZW2
```bash
python3 -m kindle_sdk.cli sign dist/kindlet-cpp-showcase.jar --keystore build/developer.keystore --password password123
cp dist/kindlet-cpp-showcase.jar dist/kindlet-cpp-showcase.azw2
```

#### Step D: Run in Kindle Simulator
```bash
python3 -m kindle_sdk.cli emulate dist/kindlet-cpp-showcase.azw2 --headless
```

---

## Deployment to Physical Kindle Device

1. Connect your jailbroken Kindle 3G / DX to your computer via USB.
2. Ensure the `developer.keystore` certificates or jailbreak developer keys (`developer.cer`) are installed in the Kindle's Java security keystore (`/var/local/java/keystore/developer.keystore`).
3. Copy `dist/kindlet-cpp-showcase.azw2` into the `documents/` directory on the Kindle USB storage:
   ```bash
   cp dist/kindlet-cpp-showcase.azw2 /media/kindle/documents/
   ```
4. Safely unmount/eject the Kindle.
5. The application will appear in the Kindle Home screen as **"Kindlet Modern C++ Showcase"**.
