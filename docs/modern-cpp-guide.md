# Modern C++ Development on Kindle

This document covers cross-compiling Modern C++ (C++17 / C++20) applications for legacy Kindle devices (Kindle Keyboard K3 and Kindle DX/DXG).

## Cross-Compilation Toolchain

Modern C++ standards (C++17 and C++20) require modern compiler frontends (such as GCC 10+), while target Kindle firmware runs legacy Linux 2.6 kernels and GNU libc 2.5.

To prevent runtime loader errors and missing symbols:
1. Target architecture flags:
   ```bash
   -march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp
   ```
2. Link static runtimes:
   ```bash
   -static-libgcc -static-libstdc++
   ```
   Or statically link the entire application (`-static`) when no dynamic plugins are required.
3. Keep system calls conservative (avoid newer Linux syscalls like `pipe2`, `accept4`, or `epoll_create1` which do not exist in Linux 2.6.26/2.6.22).

## Architectural Integration Modes

The Kindle SDK supports two primary modes for running modern C++ code on Kindle:

### Mode 1: Kindlet Process Supervisor (Recommended for App Integration)
- A signed Java Kindlet acts as the application launcher and lifecycle manager.
- The compiled native C++ ARM binary is packaged inside the Kindlet JAR.
- On `create()`/`start()`, the Kindlet extracts the binary to its local storage, makes it executable (`chmod 755`), and spawns it using `Runtime.getRuntime().exec()`.
- Communication occurs across framed, length-prefixed stdin/stdout IPC streams.
- The Kindlet terminates the child process cleanly on `stop()`/`destroy()`.

### Mode 2: Standalone Native Daemon
- Standalone C++ binary launched via SSH, USBNetwork, or KUAL (Kindle Unified Application Launcher).
- Draws directly to `/dev/fb0` and controls E-Ink flashing via `FBIO_EINK_UPDATE_DISPLAY` ioctls.
- Reads input directly from `/dev/input/event0` (5-way D-Pad, page turn buttons, keyboard).
