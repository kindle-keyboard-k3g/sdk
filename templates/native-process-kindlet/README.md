# Native-Process Kindlet Networking

This template can use the device-local Whispernet proxy from either Java or the
native process. Networking is optional; the native daemon can run without proxy
configuration for local or Wi-Fi testing.

## Enable the proxy

Set the proxy system properties before constructing the Java client:

```java
System.setProperty("kindle.whispernet.proxy.host", "127.0.0.1");
System.setProperty("kindle.whispernet.proxy.port", "3128");

WhispernetProxy proxy = WhispernetProxy.fromSystemProperties();
if (proxy != null) {
    WhispernetHttpClient client = new WhispernetHttpClient(proxy);
    HttpResponse response = client.get("http://example.com/health");
}
```

`System.getenv()` is a Java 5 API and is not available on the CDC/Java 1.4
runtime. Use `WhispernetProxy.fromSystemProperties()` and the lowercase system
properties above. The Java HTTP client enforces a 1 MiB response limit and does
not send plaintext for HTTPS URLs.

A Java kindlet that delegates HTTP to its supervised native process can use
`NetworkRelayHandler`. The handler sends `TYPE_HTTP_REQUEST` frames to the
native process and delivers correlated `TYPE_HTTP_RESPONSE` frames to the
request listener.

## Native C++ client

The native daemon reads the proxy endpoint from environment variables:

```sh
export KINDLE_WHISPERNET_PROXY_HOST=127.0.0.1
export KINDLE_WHISPERNET_PROXY_PORT=3128
```

Use the native client with `ProxyConfig::from_environment()`:

```cpp
kindle::network::ProxyConfig proxy =
    kindle::network::ProxyConfig::from_environment();
kindle::network::HttpClient client(proxy);

kindle::network::HttpResponse response =
    client.get("http://example.com/", {});
```

The daemon also accepts `--proxy-host HOST --proxy-port PORT`. Invalid proxy
configuration is reported to stderr and does not corrupt the framed IPC stream.

## Device signing

Whispernet/3G access on a physical device requires the `dn` signing role in
addition to the normal `dk` and `di` signatures. See
[`docs/signing-and-deployment.md`](../../docs/signing-and-deployment.md) for the
triple-signing workflow.

## Emulator development

For deterministic local development, start `FakeWhispernetProxy` from the
emulator module and call `installSystemProperties()`. It serves `/health`,
`/echo`, and `/post`, and provides a byte-echo `CONNECT` tunnel. It never
forwards requests to the real Internet. Always call `restoreSystemProperties()`
and `stop()` in a `finally` block.
