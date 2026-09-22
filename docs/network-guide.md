# Whispernet 3G Network Guide

Amazon Kindle 3G devices connect via the Whispernet MVNO network. The device exposes a local HTTP proxy through which all 3G traffic flows. This guide explains how to use the network APIs from kindlets and native C++ daemons.

## Architecture Overview

```
Kindlet (Java)                  C++ daemon
    |                               |
    | WhispernetHttpClient          | kindle::network::HttpClient
    | WhispernetSocketClient        | kindle::network::TcpConnection
    |        (direct proxy)         |        (direct proxy)
    |                               |
    |  --- optional IPC relay ---   |
    | NetworkRelayHandler ------>  | kindle_daemon_main (HTTP dispatch)
    |         TYPE_HTTP_REQUEST    |
    |         TYPE_HTTP_RESPONSE   |
    |                               |
    +----------+--------------------+
               |
    Device-local Whispernet proxy (host:port from env vars)
               |
           Amazon 3G network
```

Both layers speak directly to the Whispernet proxy. The IPC relay (`NetworkRelayHandler`) is optional and is used when a kindlet wants to delegate HTTP to the C++ daemon instead of opening its own socket.

## Environment Variables

The proxy address is **never hardcoded**. Both the Java and C++ layers read it from:

| Variable | Description |
|---|---|
| `KINDLE_WHISPERNET_PROXY_HOST` | Proxy hostname or IP (e.g. `127.0.0.1`) |
| `KINDLE_WHISPERNET_PROXY_PORT` | Proxy TCP port (1–65535) |

If `KINDLE_WHISPERNET_PROXY_HOST` is absent, no proxy is configured and direct connections are attempted (useful for WiFi-only testing).

## Java API

### Dependencies

```java
import com.amazon.kindle.bridge.network.WhispernetProxy;
import com.amazon.kindle.bridge.network.WhispernetHttpClient;
import com.amazon.kindle.bridge.network.WhispernetSocketClient;
import com.amazon.kindle.bridge.network.HttpRequest;
import com.amazon.kindle.bridge.network.HttpResponse;
```

### HTTP Client

```java
WhispernetProxy proxy = WhispernetProxy.fromEnvironment();
// returns null if unconfigured; throws IllegalArgumentException if HOST set but PORT invalid
if (proxy == null) {
    // WiFi-only path or no network available
    return;
}
WhispernetHttpClient client = new WhispernetHttpClient(proxy);

// GET
HttpResponse resp = client.get("http://example.com/api/data");
if (resp.isOk()) {
    String body = new String(resp.getBody());
}

// POST
HttpResponse post = client.post("http://example.com/api/submit", "data".getBytes());

// Custom request
HttpRequest req = new HttpRequest("PUT", "http://example.com/resource");
req = req.withHeader("Content-Type", "application/json");
req = req.withBody("{\"key\":\"value\"}".getBytes());
HttpResponse result = client.execute(req);
if (result.isTransportError()) {
    System.err.println("Network error: " + result.getError());
}
```

### Raw TCP via CONNECT Tunnel

```java
WhispernetSocketClient sockClient = new WhispernetSocketClient(proxy);
Socket socket = sockClient.connect("mqtt.example.com", 1883);
// socket is now a TCP stream to mqtt.example.com:1883 through the proxy
// use socket.getInputStream() / getOutputStream() normally
// caller is responsible for closing
socket.close();
```

### IPC Relay (optional)

When your kindlet controls a C++ daemon and you want HTTP to be dispatched by the daemon instead:

```java
// In the kindlet: wire the relay handler into the supervisor listener
NativeProcessSupervisor supervisor = ...;
OutputStream daemonStdin = supervisor.getOutputStream();
NetworkRelayHandler relay = new NetworkRelayHandler(
    new WhispernetHttpClient(proxy),
    daemonStdin,
    new HttpResponseListener() {
        public void onHttpResponse(int requestId, HttpResponse response) {
            // called on completion with the response
        }
    }
);

// In the supervisor message callback:
supervisor.setListener(new NativeBridgeListener() {
    public void onMessageReceived(NativeMessage msg) {
        relay.handleMessage(msg);
    }
    public void onProcessTerminated(int exitCode) {}
});
```

## C++ API

### Dependencies

```cpp
#include "kindle/network.hpp"
#include "kindle/network_ipc.hpp"   // only if using IPC relay
```

### Proxy Configuration

```cpp
// Load from environment; returns unconfigured if vars absent
kindle::network::ProxyConfig proxy = kindle::network::ProxyConfig::from_environment();
if (!proxy.is_configured()) {
    // No Whispernet proxy — use WiFi or fail
}
```

### HTTP Client

```cpp
kindle::network::HttpClient client(proxy);

kindle::network::HttpRequest req;
req.method  = "GET";
req.url     = "http://example.com/api/data";
req.headers = {{"Accept", "application/json"}};

kindle::network::HttpResponse resp = client.execute(req);
if (resp.ok()) {
    std::string body(resp.body.begin(), resp.body.end());
} else {
    // resp.status_code == 0 → transport error, check resp.error
    std::cerr << "error: " << resp.error << "\n";
}

// Convenience methods
auto get  = client.get("http://example.com/");
auto post = client.post("http://example.com/submit", payload_bytes);
```

### TCP Connection (CONNECT tunnel)

```cpp
kindle::network::TcpConnection conn(proxy);
conn.connect("mqtt.example.com", 1883);
// conn.send_all(data, size);
// conn.receive(buf, size);
conn.close();
```

### HTTPS

HTTPS is explicitly **not supported** over Whispernet (TLS backend unavailable). Both clients return a transport error (`status_code == 0`, `error = "TLS backend not available"`) for `https://` URLs without opening a socket. This prevents accidental plaintext CONNECT tunnels.

### IPC Relay (daemon side)

The daemon automatically handles `TYPE_HTTP_REQUEST` frames when built with the current `kindle_daemon_main.cpp`. No extra setup is required — load `ProxyConfig::from_environment()` at startup and the dispatch loop handles the rest.

## CRLF Injection Protection

Both Java and C++ clients reject header names or values containing `\r` or `\n` before any bytes are sent. They return a transport error without making a network connection.

## Body Size Limit

Responses exceeding 1 MiB are rejected with a transport error. This applies to both Content-Length and chunked transfer-encoding.

## Desktop Development (FakeWhispernetProxy)

During development on a desktop machine without a physical Kindle, use `FakeWhispernetProxy` (Java emulator module):

```java
FakeWhispernetProxy fakeProxy = new FakeWhispernetProxy();
fakeProxy.start();
// KINDLE_WHISPERNET_PROXY_HOST/PORT system properties now point to 127.0.0.1:N
// ... run your kindlet ...
fakeProxy.stop();
```

Or via the command line:

```sh
java -cp ... com.amazon.kindle.emulator.EmulatorLauncher my.azw2 --fake-proxy --headless
```

The fake proxy forwards requests to the real internet via `java.net.URL`.

## Signing Requirements

Whispernet (3G) access requires the **`dn` signing role** (Developer Network). See [`docs/signing-and-deployment.md`](signing-and-deployment.md) for the triple-signing procedure (`dk` → `di` → `dn`).

## Wire Format Reference

See [`docs/sot/bridge-protocol.md`](sot/bridge-protocol.md) for the full binary payload specification of `TYPE_HTTP_REQUEST` (0x06) and `TYPE_HTTP_RESPONSE` (0x07) IPC frames.
