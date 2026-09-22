# Kindle SDK

Production-grade development kit for building modern C++ and Kindlet (J2ME CDC/PBP) applications for Amazon Kindle devices, specifically targeting the Kindle Keyboard (K3/K3G) and Kindle DX/DXG.

## Highlights

- **Official Kindlet API Stubs:** Compliant with Amazon's CDC 1.1 / Personal Basis Profile (PBP 1.1) Sun CVM runtime (`create`, `start`, `stop`, `destroy`).
- **Modern C++ (C++17/20) Support:** ARMv6 cross-compilation toolchains with static linking and low-overhead IPC supervisor.
- **Hardware & E-Ink Abstractions:** Direct `/dev/fb0` framebuffer drawing, legacy ioctls (`FBIO_EINK_UPDATE_DISPLAY`), 4bpp/8bpp/16bpp formats, and evdev input parsing.
- **Hermetic Docker Build Toolchains:** Containerized J2ME and GCC cross-compilation environments for reproducible builds.
- **Triple Code Signing:** Automated `dk`, `di`, `dn` role signing and `.azw2` package validation.
- **Desktop Simulator:** Headless and GUI AWT/Swing simulator mimicking Kindle E-Ink displays and 5-way D-Pad controls.
- **Full TDD Testing:** Unit tests for Python, Java, and C++ with QEMU ARM user-mode verification.

## Architecture & Documentation

- [Architecture & Hardware Specs](docs/architecture.md)
- [Kindlet Development Guide](docs/kindlet-guide.md)
- [Modern C++ Guide](docs/modern-cpp-guide.md)
- [Signing & Deployment Guide](docs/signing-and-deployment.md)

## Quick Start

```bash
# Check toolchain prerequisites
python3 -m kindle_sdk.cli doctor

# Create a new project from a template
python3 -m kindle_sdk.cli init --template pure-kindlet my-kindlet-app

# Build and package
python3 -m kindle_sdk.cli build --project my-kindlet-app
python3 -m kindle_sdk.cli sign --project my-kindlet-app --keystore my.keystore
python3 -m kindle_sdk.cli package --project my-kindlet-app --output dist/my-app.azw2
```
