# Kindle Native-to-Kindlet IPC Protocol Specification

## Overview

The Native IPC protocol provides a robust, framed, bidirectional stream communication channel between the Java Kindlet running inside Sun CVM (CDC 1.1 / Personal Basis Profile 1.1) and a supervised native Modern C++ daemon executing as a subprocess.

## Framing Format

Every message exchanged across standard I/O pipes (`stdin` / `stdout`) adheres to a binary frame structure:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Magic: 'K' 'I' 'N' 'D'                    | (0x4B494E44)
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Version    |  Message Type |             Flags             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Request ID                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Payload Length                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Payload Data...                        |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Fields:
- **Magic (32-bit):** Fixed literal `0x4B494E44` (`KIND` in ASCII).
- **Version (8-bit):** `0x01` for Protocol v1.
- **Message Type (8-bit):**
  - `0x01`: Handshake / Ping
  - `0x02`: HandshakeAck / Pong
  - `0x10`: Command Request
  - `0x11`: Command Response
  - `0x20`: Redraw / State Notification
  - `0xFF`: Shutdown / Terminate
- **Flags (16-bit):** Reserved for future options (compression, encryption).
- **Request ID (32-bit):** Monotonically increasing identifier matching requests to responses.
- **Payload Length (32-bit):** Unsigned integer specifying bytes in payload (Max: 1 MiB).
- **Payload Data:** UTF-8 JSON payload or raw binary bytes.
