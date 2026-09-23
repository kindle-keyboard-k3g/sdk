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
- **Message Type (8-bit):** See complete table below.
- **Flags (16-bit):** Reserved for future options (compression, encryption).
- **Request ID (32-bit):** Monotonically increasing identifier matching requests to responses.
- **Payload Length (32-bit):** Unsigned integer specifying bytes in payload (Max: 1 MiB).
- **Payload Data:** UTF-8 JSON payload or raw binary bytes (see per-type payload format).

### Message Type Values

All deployed wire values — do not renumber:

| Value  | Name          | Direction      | Description                                  |
|--------|---------------|----------------|----------------------------------------------|
| `0x01` | `Ping`        | Java → Native  | Health check ping from supervisor            |
| `0x02` | `Pong`        | Native → Java  | Response to Ping                             |
| `0x06` | `HttpRequest` | Java → Native  | Whispernet proxy HTTP request (relay)        |
| `0x07` | `HttpResponse`| Native → Java  | Whispernet proxy HTTP response (relay)       |
| `0x10` | `Command`     | Java → Native  | Command request carrying action payload      |
| `0x11` | `Response`    | Native → Java  | Reply to Command with status and results     |
| `0x20` | `Notification`| Native → Java  | Asynchronous unsolicited event notification  |
| `0xFF` | `Shutdown`    | Java → Native  | Clean termination signal                     |

Values `0x03`–`0x05` and `0x08`–`0x0F` are unassigned and reserved.

---

## HTTP Relay Payload Format (`HttpRequest` / `HttpResponse`)

`HttpRequest` (0x06) and `HttpResponse` (0x07) frames carry a versioned binary payload
inside the outer KIND frame. This is distinct from — and independent of — the outer
protocol version byte. All integer fields are **unsigned big-endian**.

### HTTP Request Payload

```
Offset  Size  Field
------  ----  -----
0       u8    schema_version   (always 1 for this format)
1       u8    method_length    (length of HTTP method string, e.g. 3 for "GET")
2       u16   url_length       (length of URL string in bytes)
4       u16   header_count     (number of headers)
6       u32   body_length      (length of request body in bytes)
--- variable-length fields follow ---
        bytes method           (method_length bytes, e.g. "GET")
        bytes url              (url_length bytes, e.g. "http://example.com/path")
        [repeated header_count times]
          u16 name_length      (header name length)
          u16 value_length     (header value length)
          bytes name           (name_length bytes)
          bytes value          (value_length bytes)
        bytes body             (body_length bytes)
```

### HTTP Response Payload

```
Offset  Size  Field
------  ----  -----
0       u8    schema_version   (always 1 for this format)
1       u16   status_code      (HTTP status code; 0 = transport error)
3       u16   reason_length    (length of reason phrase string)
5       u16   header_count     (number of response headers)
7       u32   body_length      (length of response body in bytes)
--- variable-length fields follow ---
        bytes reason           (reason_length bytes, e.g. "OK")
        [repeated header_count times]
          u16 name_length      (header name length)
          u16 value_length     (header value length)
          bytes name           (name_length bytes)
          bytes value          (value_length bytes)
        bytes body             (body_length bytes)
```

### Transport Error Convention

When a transport failure prevents completing the HTTP request, the daemon returns an
`HttpResponse` frame with:
- `status_code = 0`
- `reason` containing a bounded diagnostic message (ASCII, max 255 bytes)
- `body` empty

### Encoding Rules

1. All multi-byte integer fields are **unsigned big-endian** (network byte order).
2. Validate all length fields before allocating buffers — reject malformed frames early.
3. The total payload (fixed header + all variable fields) must not exceed `MAX_PAYLOAD_SIZE` (1 MiB).
4. Binary request and response bodies are preserved without encoding; callers must not assume UTF-8.
5. `schema_version` is independent of the outer IPC protocol `Version` byte and allows the
   HTTP payload format to evolve without bumping the outer protocol version.
6. The `Request ID` in the outer KIND frame header is used to correlate `HttpRequest` frames
   with their `HttpResponse` replies; the daemon echoes the same `request_id`.
7. No diagnostic output may be written to stdout by the native daemon; all diagnostics go to stderr.
