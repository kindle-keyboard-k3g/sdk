# Amazon 3G Mobile Data (Whispernet Proxy) Support

## Context

The Kindle SDK currently has zero networking code. Kindle 3G/Keyboard (K3G) devices ship with
Whispernet — Amazon's 3G network — accessible through a device-local HTTP proxy. Applications
signed with the `dn` developer role already have the permission gate in place, but the SDK
provides no client API for either kindlets or native C++ processes to reach the network.

This plan adds a dual-layer implementation: independent Java and C++ HTTP/socket clients that both
route through the Whispernet proxy, plus an optional IPC relay shim and a desktop emulator fake
proxy. The goal is to let any SDK app — pure kindlet, kindlet+C++ daemon, or standalone native
binary — make network requests without depending on external libraries.

**Approved approach:** Dual-layer independent implementations (Approach C). Java and C++ each
carry their own implementation; neither depends on the other. An IPC relay is provided as an
optional convenience for kindlet-cpp apps that want HTTP without writing native socket code.

---

## Key Constraints

- Java compiles with `source="1.4" target="1.4"` (Ant, CDC 1.1 / PBP 1.1 target)
- C++ uses C++17, static linking (`-static-libgcc -static-libstdc++`), ARMv6 cross-compilation
- No libcurl, no JNI, no hardcoded proxy addresses
- `java.net.Proxy` is Java 5 — a CDC CVM compatibility smoke-test is required before physical
  device release; a fallback adapter must be ready if CVM lacks it
- POSIX `CONNECT` establishes a tunnel but cannot encrypt — HTTPS requires an explicit optional
  TLS backend (OpenSSL static); until verified, native HTTPS returns a clear error, never silent
  plaintext
- Existing IPC wire values must not change: `Ping=0x01 Pong=0x02 Command=0x10 Response=0x11
  Notification=0x20 Shutdown=0xFF`; new values `HttpRequest=0x06 HttpResponse=0x07` fit the
  unassigned range without renumbering

---

## Architecture

```
Kindlet Java application
        │
        ├── WhispernetHttpClient   (java.net.Proxy + URLConnection)
        ├── WhispernetSocketClient (Socket + HTTP CONNECT tunnel)
        └── NetworkRelayHandler    (optional — sends KIND IPC HttpRequest frames)
                                            │
Native C++ daemon                           │
        │                                   │
        ├── kindle::network::HttpClient     ◄┘  (relay target)
        │       └── POSIX socket HTTP over proxy
        └── kindle::network::TcpConnection
                └── POSIX CONNECT tunnel

Desktop emulator
        └── FakeWhispernetProxy (localhost, deterministic, no real network)
```

Java and C++ reach the proxy independently. The relay is additive — removing it leaves both sides
fully functional.

---

## 1. Native C++ Layer

### New files

#### `native/include/kindle/network.hpp`
Public header, namespace `kindle::network`.

```cpp
namespace kindle::network {

struct ProxyConfig {
    std::string host;
    uint16_t    port = 0;

    bool is_configured() const;
    static ProxyConfig from_environment();  // reads KINDLE_WHISPERNET_PROXY_{HOST,PORT}
};

using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

struct HttpRequest {
    std::string  method;   // "GET", "POST", …
    std::string  url;
    HttpHeaders  headers;
    std::vector<uint8_t> body;
};

struct HttpResponse {
    int          status_code = 0;  // 0 = transport error
    std::string  reason;
    HttpHeaders  headers;
    std::vector<uint8_t> body;
    std::string  error;

    bool ok() const { return status_code >= 200 && status_code < 300; }
};

class TcpConnection {  // RAII POSIX socket
public:
    explicit TcpConnection(const ProxyConfig& proxy);
    ~TcpConnection();
    bool connect(const std::string& host, uint16_t port);
    bool connect_tunnel(const std::string& host, uint16_t port);  // HTTP CONNECT
    bool send_all(const uint8_t* data, size_t size);
    int64_t receive(uint8_t* data, size_t size);
    bool is_open() const;
    void close();
};

class HttpClient {
public:
    explicit HttpClient(const ProxyConfig& proxy);
    HttpResponse execute(const HttpRequest& request);
    HttpResponse get(const std::string& url, const HttpHeaders& headers = {});
    HttpResponse post(const std::string& url, const HttpHeaders& headers,
                      const std::vector<uint8_t>& body);
};

}  // namespace kindle::network
```

#### `native/src/network.cpp`
POSIX implementation covering:
- `getaddrinfo` + retry-per-address connection
- Absolute-form requests through HTTP proxy; origin-form for direct
- `Content-Length` and `Connection: close` on all requests
- Response parser: status line, headers (case-insensitive), `Content-Length`,
  `Transfer-Encoding: chunked`, close-delimited body
- 1 MiB body limit (configurable, matches IPC max payload)
- HTTPS: `CONNECT host:443`, 200-gate, expose tunnel stream to optional TLS backend;
  return explicit `error` field when no TLS backend is linked — never send plaintext
- `SIGPIPE` prevention via `MSG_NOSIGNAL` or `SO_NOSIGPIPE`
- All failures return structured `HttpResponse{status_code=0, error=…}` — no daemon crash

**Injectable test seam:** `kindle::network::detail::SocketBackend` (internal) — allows
scripted fake responses, fragmented reads, EOF simulation in unit tests.

#### `native/include/kindle/network_ipc.hpp` + `native/src/network_ipc.cpp`
Binary codec for the IPC relay payload (separate from the outer KIND frame).

Request wire format (all fields big-endian):
```
u8   schema_version
u8   method_length
u16  url_length
u16  header_count
u32  body_length
bytes method
bytes url
per-header: u16 name_length, u16 value_length, bytes name, bytes value
bytes body
```

Response wire format:
```
u8   schema_version
u16  status_code       (0 = transport error)
u16  reason_length
u16  header_count
u32  body_length
bytes reason
per-header: u16 name_length, u16 value_length, bytes name, bytes value
bytes body
```

Constraints: validate all lengths before allocation; reject frames exceeding
`MAX_PAYLOAD_SIZE`; binary bodies preserved without modification.

#### `native/tests/test_network.cpp`
Test categories:
- `ProxyConfig`: valid config, missing env, no hardcoded fallback
- HTTP request formatting: absolute-form vs origin-form, Host/Content-Length headers
- Response parsing: content-length, chunked, close-delimited, fragmented reads,
  mixed-case headers
- Error handling: bad status line, invalid chunk size, truncated body, size limit exceeded
- CONNECT: correct request, header draining after 200, rejection of 407/403/malformed
- IPC codec: round-trip request/response, binary body with zero bytes, length rejection
- HTTPS guard: verify CONNECT is issued; verify no plaintext is sent without TLS backend

#### `native/platform/fake/fake_network.cpp` *(optional)*
Reusable fake socket backend for the test seam (scripted responses, fragmented reads).

### Modified files

#### `native/include/kindle/ipc.hpp`
Add without renumbering existing values:
```cpp
HttpRequest  = 0x06,
HttpResponse = 0x07,
```

#### `native/src/kindle_daemon_main.cpp`
- Accept `--proxy-host HOST --proxy-port PORT` (env fallback:
  `KINDLE_WHISPERNET_PROXY_{HOST,PORT}`)
- Dispatch `HttpRequest` frames: decode binary payload → `HttpClient::execute` →
  encode `HttpResponse` with matching request ID
- Malformed payload → return `HttpResponse` error frame, never crash
- All daemon diagnostics go to **stderr** only (stdout is the framed IPC stream)

#### `native/CMakeLists.txt`
```cmake
# Add to kindle_native sources:
native/src/network.cpp
native/src/network_ipc.cpp

# New test executable:
add_executable(test_network native/tests/test_network.cpp)
target_link_libraries(test_network kindle_native)

# Optional TLS (disabled by default):
option(KINDLE_NETWORK_WITH_OPENSSL "Enable OpenSSL TLS backend" OFF)
```

---

## 2. Java Layer

### New files under `java/kindlet-bridge/.../bridge/network/`

Package: `com.amazon.kindle.bridge.network`

All code must compile with `source="1.4" target="1.4"`. No generics, no enhanced `for`,
no autoboxing, no try-with-resources, no `StandardCharsets`. Use raw `Hashtable`/`Vector`,
string charset names (`"UTF-8"`).

#### `WhispernetProxy.java`
```java
public final class WhispernetProxy {
    public static final String HOST_PROPERTY = "kindle.whispernet.proxy.host";
    public static final String PORT_PROPERTY = "kindle.whispernet.proxy.port";

    public WhispernetProxy(String host, int port);
    public static WhispernetProxy fromSystemProperties();  // throws if absent/malformed
    public String getHost();
    public int getPort();
    public boolean isConfigured();
}
```

`fromSystemProperties()` never silently falls back to a hardcoded address.

**CDC compatibility gate:** if `java.net.Proxy` is unavailable on the target CVM,
implement a private `CompatProxy` adapter using the supported connection mechanism.
The public API is unchanged; only the internal mechanism switches.

#### `HttpRequest.java`
Java 1.4 value type: method (`String`), url (`String`), headers (`Hashtable`),
body (`byte[]`). Defensive copies on construction. Validates method, rejects CRLF
injection in header names/values.

#### `HttpResponse.java`
Java 1.4 value type: statusCode (`int`), reasonPhrase (`String`),
headers (`Hashtable`), body (`byte[]`). 1 MiB body limit enforced on read.
Case-insensitive header lookup. `isSuccess()` predicate.

#### `WhispernetHttpClient.java`
```java
public HttpResponse execute(HttpRequest request) throws IOException;
public HttpResponse get(String url) throws IOException;
public HttpResponse post(String url, Hashtable headers, byte[] body) throws IOException;
```

Implementation:
1. Validate URL scheme (`http`/`https` only)
2. `new URL(url).openConnection(proxy)` where proxy is `Proxy.Type.HTTP`
3. Set method, headers, follow-redirects disabled
4. Write body if present
5. Read status, headers, body (bounded)
6. Manual redirect following: max 5, resolve relative URLs, reject loops,
   reject HTTPS→HTTP downgrade by default

#### `WhispernetSocketClient.java`
```java
public Socket openTunnel(String targetHost, int targetPort) throws IOException;
```

Sends `CONNECT target:port HTTP/1.1\r\nHost: target:port\r\n\r\n`, requires 200,
drains proxy response headers, returns connected `Socket`. Closes socket on
non-200 or malformed response.

#### `HttpIpcCodec.java`
Java implementation of the binary relay payload format (mirrors `network_ipc.cpp`).
Encodes `HttpRequest` → `byte[]`; decodes `byte[]` → `HttpResponse`.

#### `HttpResponseListener.java`
```java
public interface HttpResponseListener {
    void onHttpResponse(int requestId, HttpResponse response);
    void onHttpError(int requestId, String message);
}
```

#### `NetworkRelayHandler.java`
Optional wrapper around an existing `NativeProcessSupervisor`. Implements
`NativeBridgeListener` — forwards non-HTTP messages to the application's own listener,
matches `TYPE_HTTP_RESPONSE` frames by request ID, dispatches to `HttpResponseListener`.
Uses raw `Hashtable` keyed by `Integer` for pending requests; synchronized access.
Notifies outstanding requests on native process termination.

### Modified Java bridge files

#### `NativeMessage.java`
```java
public static final byte TYPE_HTTP_REQUEST  = 0x06;
public static final byte TYPE_HTTP_RESPONSE = 0x07;
```

#### `NativeProcessSupervisor.java`
- Add `start(File exec, NativeBridgeListener listener, String[] extraArguments)` overload
  (existing two-arg delegates to it)
- Synchronize `sendMessage` writes (multiple threads may use relay concurrently)

#### `java/kindlet-bridge/build.xml`
Network classes compile automatically via existing `srcdir`; add network test classes to
the test target.

---

## 3. Emulator Fake Proxy

### New file: `java/emulator/.../emulator/FakeWhispernetProxy.java`

Binds to `127.0.0.1` on an ephemeral port. Runs an accept loop on a daemon thread.
Routes:
- `GET /health` → `200 OK` + `{"status":"ok"}`
- `GET /echo` → echoes request headers as JSON body
- `POST /post` → echoes request body
- `CONNECT host:port` → `200 Connection established`, then byte-echo (or scripted tunnel)

Enforces bounded request-line and header sizes. Default: no real Internet forwarding.

```java
public void start() throws IOException;
public void stop();
public String getHost();
public int getPort();
public void installSystemProperties();   // saves previous values
public void restoreSystemProperties();  // restores previous values
```

Property keys set: `kindle.whispernet.proxy.host`, `kindle.whispernet.proxy.port`.

### Modified emulator files

#### `EmulatorLauncher.java`
Start `FakeWhispernetProxy` before Kindlet lifecycle; `installSystemProperties()`;
`restoreSystemProperties()` + `stop()` in finally block. CLI flags:
`--no-fake-whispernet`, `--whispernet-proxy HOST:PORT`.

#### `KindleSimulatorWindow.java`
Own fake proxy lifecycle in interactive mode; stop on window close.

#### `SimulatorKindletContext.java`
Store proxy host/port; do not change public `KindletContext` interface.

---

## 4. Tests

| Layer | File | Notes |
|---|---|---|
| C++ unit | `native/tests/test_network.cpp` | Fake socket seam; covers parser edge cases |
| Java unit | `.../bridge/network/WhispernetNetworkTest.java` | Local `ServerSocket`; GET/POST/redirect/CONNECT |
| Java unit | `.../bridge/network/HttpIpcCodecTest.java` | Binary round-trip; wire-format interop with C++ |
| Java unit | `.../bridge/network/NetworkRelayHandlerTest.java` | Mock IPC stream; correlation; process exit |
| Emulator unit | `.../emulator/FakeWhispernetProxyTest.java` | Lifecycle, property save/restore, no thread leak |
| Integration | `tests/integration/test_network_relay.py` | Real daemon build; Python fake proxy; end-to-end KIND frames |

Integration test flow:
1. Build native daemon
2. Start Python fake HTTP proxy on ephemeral port
3. Launch daemon with proxy env vars
4. Send `KIND HttpRequest` frame; assert `KIND HttpResponse` with matching request ID
5. Test chunked response, binary body, send `Shutdown`, assert clean exit

---

## 5. Documentation

| File | Change |
|---|---|
| `docs/architecture.md` | Add network subsystem section with layer diagram |
| `docs/signing-and-deployment.md` | Note `dn` role required; emulator ≠ physical device permission |
| `docs/sot/bridge-protocol.md` | Add `HttpRequest=0x06`, `HttpResponse=0x07`; document binary payload schemas |
| `docs/README.md` | Add `network-guide.md` entry |
| `docs/network-guide.md` | **New** — Java config/examples, C++ examples, relay usage, TLS limits, security notes |
| `templates/native-process-kindlet/README.md` | **New** — how to enable networking, proxy config, dn signing |

Example snippet for `network-guide.md`:
```java
System.setProperty("kindle.whispernet.proxy.host", proxyHost);
System.setProperty("kindle.whispernet.proxy.port", String.valueOf(proxyPort));
WhispernetProxy proxy = WhispernetProxy.fromSystemProperties();
WhispernetHttpClient client = new WhispernetHttpClient(proxy);
HttpResponse resp = client.get("http://example.com/status");
```

---

## 6. Implementation Sequence

- [ ] Reconcile IPC enum values in source vs docs; document discrepancy
- [ ] Define and document binary HTTP IPC payload format (update `bridge-protocol.md`)
- [ ] `network.hpp` public API + `network.cpp` POSIX implementation
- [ ] `test_network.cpp` with fake socket seam; verify host compilation
- [ ] `network_ipc.hpp` + `network_ipc.cpp` binary codec + codec tests
- [ ] CMake additions (`kindle_native` sources, `test_network`, TLS option)
- [ ] ARMv6 cross-compilation verification
- [ ] Java value types: `WhispernetProxy`, `HttpRequest`, `HttpResponse`
- [ ] `WhispernetHttpClient` + `WhispernetSocketClient` + Java unit tests
- [ ] CDC/CVM `java.net.Proxy` compatibility smoke-test; implement adapter if needed
- [ ] `HttpIpcCodec.java` + interop test against C++ codec
- [ ] `NativeMessage` constants, `NativeProcessSupervisor` overload + sync
- [ ] `NetworkRelayHandler` + relay handler tests
- [ ] Daemon dispatch: `HttpRequest` frame handling in `kindle_daemon_main.cpp`
- [ ] `FakeWhispernetProxy` + emulator integration (`EmulatorLauncher`, simulator window)
- [ ] `tests/integration/test_network_relay.py`
- [ ] All documentation files
- [ ] Full test suite: CTest + Ant + integration; verify no existing tests regress
- [ ] Physical device: confirm `dn`-signed package gets network permission on K3G

---

## 7. Acceptance Criteria

1. Java sources compile with `source="1.4" target="1.4"`
2. Java clients perform GET and POST through a configured HTTP proxy
3. Java clients follow bounded redirects (≤5), preserve binary bodies
4. Java `WhispernetSocketClient` establishes a CONNECT tunnel and returns a usable `Socket`
5. C++ builds with host compiler and ARMv6 toolchain; no libcurl dependency
6. C++ parses content-length, chunked, and close-delimited responses
7. C++ proxy config has no hardcoded device address
8. Native IPC relay uses framed messages; 1 MiB limit enforced
9. Daemon returns correlated `HttpResponse` frames for `HttpRequest` frames
10. Emulator automatically provides a deterministic fake proxy
11. All tests pass: `test_network`, Java bridge tests, `FakeWhispernetProxyTest`, integration
12. HTTPS never sends plaintext through CONNECT when no TLS backend is linked
13. Existing IPC wire values (`Ping=0x01 … Shutdown=0xFF`) remain unchanged
14. `docs/signing-and-deployment.md` documents `dn` permission requirement
