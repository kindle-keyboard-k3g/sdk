# Kindle SDK Documentation Index

Comprehensive guides, specifications, architectural references, and hardware profiles for the Kindle SDK.

## Guides & References

| Document | Description |
| :--- | :--- |
| [`docs/architecture.md`](architecture.md) | Kindle Keyboard (K3) and Kindle DX hardware profiles, SoCs, RAM limits, display ioctls, and CVM runtime constraints |
| [`docs/kindlet-guide.md`](kindlet-guide.md) | Official Amazon Kindlet J2ME CDC 1.1 / Personal Basis Profile 1.1 specification, lifecycle methods, UI components, and bytecode requirements |
| [`docs/modern-cpp-guide.md`](modern-cpp-guide.md) | Modern C++ (C++17/20) cross-compilation for ARMv6 softfp, glibc 2.5 compatibility, and native-in-Kindlet integration |
| [`docs/signing-and-deployment.md`](signing-and-deployment.md) | Code signing security, developer keystore management, `dk`/`di`/`dn` sequential triple-signing, and `.azw2` packaging |
| [`docs/cli.md`](cli.md) | `kindle-sdk` command-line reference (`doctor`, `init`, `build`, `sign`, `package`, `emulate`) |
| [`docs/emulator.md`](emulator.md) | Desktop E-Ink simulator architecture, 16-level grayscale quantization, refresh simulation, and headless testing |
| [`docs/sot/bridge-protocol.md`](sot/bridge-protocol.md) | Wire protocol framing and specification for Java-to-C++ IPC communication |
| [`docs/sot/j2me-kindlet.md`](sot/j2me-kindlet.md) | Source-of-truth technical reference on Kindlet environment, security constraints, and jailbreak prerequisites |
| [`examples/kindlet-cpp-showcase/README.md`](../examples/kindlet-cpp-showcase/README.md) | Production-grade reference app embedding modern C++17 inside an official Kindlet (lifecycle, IPC, 16-level grayscale canvas, signing, emulation) |

