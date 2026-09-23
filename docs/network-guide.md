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

The proxy address is **never hardcoded**. The native C++ daemon reads:

| Environment variable | Description |
|---|---|
| `KINDLE_WHISPERNET_PROXY_HOST` | Proxy hostname or IP (e.g. `127.0.0.1`) |
| `KINDLE_WHISPERNET_PROXY_PORT` | Proxy TCP port (1–65535) |

The Java CDC client reads the equivalent system properties:

| System property | Description |
|---|---|
| `kindle.whispernet.proxy.host` | Proxy hostname or IP |
| `kindle.whispernet.proxy.port` | Proxy TCP port |

`System.getenv()` is a Java 5 API and is not available on CDC/Java 1.4. Use
`WhispernetProxy.fromSystemProperties()` instead. If the C++ environment variables
are absent, the daemon starts without a proxy.

## Java API

### Dependencies

```java
import com.amazon.kindle.bridge.network.WhispernetProxy;
import com.amazon.kindle.bridge.network.WhispernetHttpClient;
import com.amazon.kindle.bridge.network.WhispernetSocketClient;
import com.amazon.kindle.bridge.network.HttpRequest;
import com.amazon.kindle.bridge.network.HttpResponse;
import java.util.Vector;
```

### HTTP Client

```java
WhispernetProxy proxy = WhispernetProxy.fromSystemProperties();
// returns null if unconfigured; throws IllegalArgumentException if HOST is set but PORT is invalid
if (proxy == null) {
    // No proxy configured or no network available
    return;
}
WhispernetHttpClient client = new WhispernetHttpClient(proxy);

// GET
HttpResponse resp = client.get("http://example.com/api/data");
if (resp.isOk()) {
    String body = new String(resp.getBody());
}

// POST (url, headers, body)
Vector headers = new Vector();
headers.addElement(new String[]{"Content-Type", "text/plain"});
HttpResponse post = client.post("http://example.com/api/submit", headers,
                               "data".getBytes("UTF-8"));

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

When your kindlet controls a C++ daemon and wants the daemon to perform HTTP:

```java
NativeProcessSupervisor supervisor = ...;
NetworkRelayHandler relay = new NetworkRelayHandler(supervisor,
    new NativeBridgeListener() {
        public void onMessageReceived(NativeMessage message) {
            // Handle non-network frames here.
        }
        public void onProcessTerminated(int exitCode) {}
    });
supervisor.start(daemonExecutable, relay);

HttpRequest request = new HttpRequest("GET", "http://example.com/status");
relay.sendHttpRequest(request, new HttpResponseListener() {
    public void onHttpResponse(int requestId, HttpResponse response) {
        // Called when the native daemon returns TYPE_HTTP_RESPONSE.
    }
    public void onHttpError(int requestId, String message) {
        // Called if the daemon terminates or the frame cannot be decoded.
    }
});
```

`NetworkRelayHandler` sends `TYPE_HTTP_REQUEST` frames Java → native and
correlates native `TYPE_HTTP_RESPONSE` frames by request ID. It does not execute
HTTP requests in Java.

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
auto post = client.post("http://example.com/submit", {}, payload_bytes);
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
try {
    fakeProxy.start();
    fakeProxy.installSystemProperties();
    // ... run your kindlet against /health, /echo, or /post ...
} finally {
    fakeProxy.restoreSystemProperties();
    fakeProxy.stop();
}
```

Or via the command line:

```sh
java -cp ... com.amazon.kindle.emulator.EmulatorLauncher my.azw2 --fake-proxy --headless
```

The fake proxy is deterministic and local-only. It serves canned `/health`, `/echo`, and `/post` routes and provides a byte-echo `CONNECT` tunnel; it never forwards to the public Internet.

## Signing Requirements

Whispernet (3G) access requires the **`dn` signing role** (Developer Network). See [`docs/signing-and-deployment.md`](signing-and-deployment.md) for the triple-signing procedure (`dk` → `di` → `dn`).

## Wire Format Reference

See [`docs/sot/bridge-protocol.md`](sot/bridge-protocol.md) for the full binary payload specification of `TYPE_HTTP_REQUEST` (0x06) and `TYPE_HTTP_RESPONSE` (0x07) IPC frames.
