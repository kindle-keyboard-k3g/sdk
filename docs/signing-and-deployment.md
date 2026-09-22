# Code Signing & `.azw2` Packaging Specification

This document defines the security architecture, keystore requirements, code signing procedures, and packaging format for Kindle active content (`.azw2`).

## Security Architecture & Alias Roles

Kindlets run inside the Sun CVM on Kindle devices and are validated against keystores stored in `/var/local/java/keystore/developer.keystore`. 

To grant active content appropriate system permissions on jailbroken/developer devices, the JAR file must be sequentially signed with three certificate alias roles:
1. `dk*` (General Kindlet Developer signature): Basic application sandbox privileges.
2. `di*` (Device Interaction signature): Grants permission to interact with low-level device components, orientation, screen refresh, and audio.
3. `dn*` (Developer Network signature): Grants network connectivity (WiFi / 3G Whispernet). **Required for any kindlet that uses `WhispernetHttpClient`, `WhispernetSocketClient`, or the `kindle::network` C++ API.** Without this signature the CVM denies all socket connections on the device.

> **Note for Whispernet / 3G kindlets:** After signing, set the `KINDLE_WHISPERNET_PROXY_HOST` and `KINDLE_WHISPERNET_PROXY_PORT` environment variables (or pass them through the launcher) so the SDK clients can locate the device-local proxy. These are never hardcoded. See [`docs/network-guide.md`](network-guide.md) for full configuration details.

### Signature Algorithms:
- **Kindle Keyboard (Firmware 3.4.3+):** RSA-2048 with `SHA256withRSA` signature algorithm (`.RSA` signature block).
- **Legacy Devices (Kindle 2 / DX / Early K3):** RSA-1024 / DSA with SHA-1 signatures (`.DSA` / `.RSA`).

## Security Best Practices
- **Never commit private keys, keystores, or credentials to git repository.**
- Test environments must generate ephemeral keystores on-the-fly.
- `.gitignore` explicitly filters `*.keystore`, `*.p12`, `*.key`, and `*.azw2`.

## `.azw2` Container Specification

An `.azw2` file is a triple-signed Java Archive containing:
1. Compiled Java class files targeting Java 1.4 bytecode.
2. `META-INF/MANIFEST.MF` conforming to Amazon Kindlet requirements:
   ```manifest
   Manifest-Version: 1.0
   Main-Class: com.example.MyKindlet
   Implementation-Title: My App
   Implementation-Version: 1.0.0
   Extension-List: SDK
   SDK-Extension-Name: com.amazon.kindle.kindlet
   SDK-Specification-Version: 2.1
   Toolbar-Mode: persistent
   Amazon-Cover-Image: cover.png
   ```
   *Note: Manifest files must end with an empty newline and contain no trailing whitespace.*
3. Optional native binaries under `/native/armv6/` or application assets.
4. Triple signature files in `META-INF/` produced by `jarsigner`.
