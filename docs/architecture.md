# Kindle Hardware & System Architecture

This document specifies the target hardware profiles, SoC architectures, memory constraints, display characteristics, and OS/runtime layers for legacy Amazon Kindle devices supported by the Kindle SDK.

## Target Hardware Profiles

| Feature | Kindle Keyboard (K3 / K3G) | Kindle DX / DX Graphite (DX / DXG) |
| :--- | :--- | :--- |
| **Model Code** | D00901, B006 (US/CA 3G), B00A (EU 3G), etc. | B004, B005, B009 |
| **SoC / Platform** | Freescale MCIMX353 (i.MX35 family) | Freescale MCIMX31L (i.MX31 family) |
| **CPU Architecture** | ARM1136JF-S (ARMv6) @ 532 MHz | ARM1136JF-S (ARMv6) @ 400 MHz |
| **FPU / Vector** | VFPv2 Vector Floating Point | VFPv2 Vector Floating Point |
| **System RAM** | 256 MiB SDRAM (133 MHz DDR) | 128 MiB Mobile SDRAM |
| **Internal Storage** | 4 GiB eMMC Flash | 4 GiB Flash |
| **E-Ink Display** | 6.0" E-Ink Pearl, 600×800 (167 ppi) | 9.7" E-Ink Vizplex (DX) / Pearl (DXG), 824×1200 (150 ppi) |
| **Grayscale Levels** | 16-level grayscale (4-bit per pixel) | 16-level grayscale (4-bit per pixel) |
| **Operating System** | Custom Linux 2.6.26-rt-lab126 | Custom Linux 2.6.22 / 2.6.22.19-lab126 |
| **Standard C Library**| GNU C Library (glibc) 2.5 | GNU C Library (glibc) 2.5-era |
| **Java Runtime** | Sun PhoneME Advanced / CVM (CDC 1.1 / PBP 1.1) | Sun PhoneME Advanced / CVM (CDC 1.1 / PBP 1.1) |

## Native ABI & Toolchain Recommendations

- **Architecture baseline:** `-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp`
- **Compiler target:** `arm-linux-gnueabi` or `arm-kindle-linux-gnueabi`
- **Static runtime linking:** When targeting native C++17/20, use `-static-libgcc -static-libstdc++` (or full static linking) to avoid ABI and C++ standard library incompatibilities with the legacy glibc 2.5 host runtime.
- **Sysroot verification:** Always verify exported glibc symbols (`readelf -s`) against the target firmware sysroot.

## E-Ink Framebuffer & Controller Interface

Legacy Kindle devices drive the display controller through the Linux framebuffer device (`/dev/fb0`) and custom ioctls or procfs triggers:
- Framebuffer node: `/dev/fb0` (memory-mapped)
- Legacy IOCTL: `FBIO_EINK_UPDATE_DISPLAY` (0x46db)
- Legacy Area IOCTL: `FBIO_EINK_UPDATE_DISPLAY_AREA` (0x46dd)
- Screen Clear IOCTL: `FBIO_EINK_CLEAR_SCREEN` (0x46e1)
- ProcFS update trigger: `/proc/eink_fb/update_display` (e.g., `echo 1 > /proc/eink_fb/update_display`)

## Network Subsystem

The networking stack keeps Java kindlets and native processes independent while
providing an optional IPC relay when a kindlet delegates HTTP work to its native
daemon.

```text
Kindlet Java application
        |
        +-- WhispernetHttpClient (HTTP through proxy)
        +-- WhispernetSocketClient (CONNECT tunnel)
        +-- NetworkRelayHandler (optional Java -> native IPC)
                                      |
Native C++ daemon                      |
        +-- kindle::network::HttpClient <+-- TYPE_HTTP_REQUEST/RESPONSE
        |       +-- POSIX HTTP over proxy
        +-- kindle::network::TcpConnection
                +-- POSIX CONNECT tunnel
                                      |
              Device-local Whispernet proxy
                                      |
                              Whispernet 3G network
```

- `WhispernetProxy` loads the Java proxy endpoint from system properties; the
  native `ProxyConfig` loads its endpoint from environment variables or daemon
  command-line arguments.
- `WhispernetHttpClient` parses bounded HTTP responses, including content-length,
  chunked, and close-delimited bodies, while rejecting unsupported HTTPS rather
  than sending plaintext.
- `WhispernetSocketClient` and `kindle::network::TcpConnection` establish raw
  TCP streams through an HTTP CONNECT proxy.
- `HttpIpcCodec` mirrors the binary request/response payload format on both
  platforms, and `kindle_daemon` dispatches native requests safely.
- `NetworkRelayHandler` is optional. It sends Java requests as framed
  `TYPE_HTTP_REQUEST` messages, correlates `TYPE_HTTP_RESPONSE` replies, and
  forwards unrelated native messages to the application listener.
