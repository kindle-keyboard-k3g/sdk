# Kindle SDK CLI Reference (`kindle-sdk`)

The `kindle-sdk` command-line interface provides unified commands for environment verification, project scaffolding, compiling, signing, packaging, and simulation.

## Installation & Setup

Ensure the Python package is installed or accessible in your virtual environment:

```bash
pip install -e python/
kindle-sdk --help
```

## Global Command Syntax

```bash
kindle-sdk [OPTIONS] COMMAND [ARGS]...
```

### Available Subcommands

| Command | Purpose |
| :--- | :--- |
| `doctor` | Probes host system for required Java, Ant, CMake, cross-compilers, QEMU, and key tools |
| `init` | Scaffolds a new project from preconfigured templates |
| `build` | Compiles Java Kindlet (Java 1.4 bytecode) and/or cross-compiles native C++ |
| `sign` | Sequentially signs JAR with developer keys (`dk`, `di`, `dn`) |
| `package` | Packages signed JAR into Kindle Active Content `.azw2` format |
| `emulate` | Launches application in the desktop E-Ink simulator |

---

## Detailed Subcommand Usage

### 1. `kindle-sdk doctor`

Inspects host environment and reports status of essential toolchain components.

```bash
kindle-sdk doctor
```

**Checks Performed:**
- Java Development Kit (JDK 8 recommended)
- Apache Ant (`ant`)
- Native Cross-Compilers (`arm-linux-gnueabi-gcc`, `arm-linux-gnueabi-g++`)
- Build Generators (`cmake`, `ninja`, `make`)
- Security Tools (`keytool`, `jarsigner`, `openssl`)
- Architecture Emulators (`qemu-arm`, `qemu-arm-static`)

---

### 2. `kindle-sdk init`

Scaffolds a new project structure in the target directory.

```bash
kindle-sdk init <project-name> [--template <template-id>] [--target <k3|dx>]
```

**Options:**
- `--template`: Project template type:
  - `pure-kindlet`: Pure Java CDC 1.1 / PBP 1.1 Kindlet with high-contrast UI
  - `native-process-kindlet` (default): Kindlet supervising cross-compiled C++ ARMv6 daemon via framed IPC
  - `native-jni-kindlet`: In-process native library binding via JNI
  - `standalone-native`: Pure C++ daemon directly driving `/dev/fb0` and `/dev/input/event0`
- `--target`: Target hardware profile (`k3` for Kindle Keyboard, `dx` for Kindle DX).

---

### 3. `kindle-sdk build`

Compiles project artifacts targeting legacy Kindle hardware constraints.

```bash
kindle-sdk build [--project-dir <path>] [--target <k3|dx>]
```

**Actions:**
- Compiles Java sources using Ant with `-source 1.4 -target 1.4` (bytecode major 48).
- Cross-compiles C++ sources with CMake using flags:
  `-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -static-libgcc -static-libstdc++`.

---

### 4. `kindle-sdk sign`

Performs triple sequential code-signing using `dk`, `di`, and `dn` certificate roles.

```bash
kindle-sdk sign <archive-path> --keystore <path> --password <pass> [--sigalg SHA256withRSA]
```

**Options:**
- `<archive-path>`: Path to input JAR file.
- `--keystore`: Path to developer JKS or PKCS12 keystore.
- `--password`: Keystore unlock password.
- `--sigalg`: Signature algorithm (defaults to `SHA256withRSA` for firmware 3.4.3+).

---

### 5. `kindle-sdk package`

Bundles signed binaries, resources, and Amazon-compliant `MANIFEST.MF` into a deployable `.azw2` container.

```bash
kindle-sdk package <source-jar> -o <output.azw2>
```

---

### 6. `kindle-sdk emulate`

Executes the `.azw2` or JAR package inside the desktop Kindle simulator.

```bash
kindle-sdk emulate <app.azw2> [--headless] [--target <k3|dx>]
```

**Options:**
- `--headless`: Executes the Kindlet lifecycle (`create`, `start`, `stop`, `destroy`) headlessly for CI test automation.
- `--target`: Configures display resolution (600×800 for K3, 824×1200 for DX).
