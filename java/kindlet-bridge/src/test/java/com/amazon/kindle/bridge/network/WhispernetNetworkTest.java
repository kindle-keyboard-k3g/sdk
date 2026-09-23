package com.amazon.kindle.bridge.network;

import com.amazon.kindle.bridge.NativeMessage;
import com.amazon.kindle.bridge.NativeProcessSupervisor;
import com.amazon.kindle.bridge.ProcessLauncher;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Vector;

/**
 * Unit tests for WhispernetProxy, WhispernetHttpClient, WhispernetSocketClient,
 * HttpRequest, and HttpResponse.
 * Runs a minimal in-process fake server for network tests.
 * Java 1.4 compatible (no generics, no enhanced for, no autoboxing).
 */
public class WhispernetNetworkTest {

    private static int passed = 0;
    private static int total  = 0;

    private static void check(String name, boolean condition) {
        total++;
        if (condition) {
            passed++;
            System.out.println("PASS: " + name);
        } else {
            System.out.println("FAIL: " + name);
        }
    }

    // -------------------------------------------------------------------------
    // WhispernetProxy value type tests
    // -------------------------------------------------------------------------

    static void testProxyValueType() {
        System.out.println("\n-- WhispernetProxy --");

        WhispernetProxy p = new WhispernetProxy("10.0.0.1", 3128);
        check("host accessor", "10.0.0.1".equals(p.getHost()));
        check("port accessor", p.getPort() == 3128);
        check("toString format", "10.0.0.1:3128".equals(p.toString()));

        try {
            new WhispernetProxy(null, 3128);
            check("null host rejected", false);
        } catch (IllegalArgumentException e) {
            check("null host rejected", true);
        }

        try {
            new WhispernetProxy("host", 0);
            check("port 0 rejected", false);
        } catch (IllegalArgumentException e) {
            check("port 0 rejected", true);
        }

        try {
            new WhispernetProxy("host", 65536);
            check("port 65536 rejected", false);
        } catch (IllegalArgumentException e) {
            check("port 65536 rejected", true);
        }
    }

    // -------------------------------------------------------------------------
    // HttpRequest value type tests
    // -------------------------------------------------------------------------

    static void testHttpRequestValueType() {
        System.out.println("\n-- HttpRequest --");

        HttpRequest req = new HttpRequest("GET", "http://example.com/");
        check("method accessor", "GET".equals(req.getMethod()));
        check("url accessor",    "http://example.com/".equals(req.getUrl()));
        check("headers empty",   req.getHeaders().size() == 0);
        check("body empty",      req.getBody().length == 0);

        HttpRequest req2 = req.withHeader("Accept", "text/html");
        check("withHeader returns new instance", req2 != req);
        check("withHeader name",  ((String[]) req2.getHeaders().elementAt(0))[0].equals("Accept"));
        check("withHeader value", ((String[]) req2.getHeaders().elementAt(0))[1].equals("text/html"));
        check("original unchanged", req.getHeaders().size() == 0);

        byte[] body = "hello".getBytes();
        HttpRequest req3 = req.withBody(body);
        check("withBody sets body", req3.getBody().length == 5);
        check("original body unchanged", req.getBody().length == 0);

        try {
            new HttpRequest(null, "http://x.com/");
            check("null method rejected", false);
        } catch (IllegalArgumentException e) {
            check("null method rejected", true);
        }

        try {
            new HttpRequest("GET", "");
            check("empty url rejected", false);
        } catch (IllegalArgumentException e) {
            check("empty url rejected", true);
        }
    }

    // -------------------------------------------------------------------------
    // HttpResponse value type tests
    // -------------------------------------------------------------------------

    static void testHttpResponseValueType() {
        System.out.println("\n-- HttpResponse --");

        Vector headers = new Vector();
        headers.addElement(new String[]{"Content-Type", "text/plain"});
        HttpResponse resp = new HttpResponse(200, "OK", headers, "body".getBytes());
        check("status code",      resp.getStatusCode() == 200);
        check("reason",           "OK".equals(resp.getReason()));
        check("error empty",      resp.getError().length() == 0);
        check("isOk",             resp.isOk());
        check("isTransportError false", !resp.isTransportError());
        check("getHeader found", "text/plain".equals(resp.getHeader("content-type")));
        check("getHeader missing", resp.getHeader("X-Missing") == null);
        check("body",             resp.getBody().length == 4);

        HttpResponse err = new HttpResponse("connection refused");
        check("error status 0",    err.getStatusCode() == 0);
        check("error message",     "connection refused".equals(err.getError()));
        check("isOk false",        !err.isOk());
        check("isTransportError",  err.isTransportError());
        check("error reason empty", err.getReason().length() == 0);
    }

    // -------------------------------------------------------------------------
    // WhispernetHttpClient tests with a fake proxy server
    // -------------------------------------------------------------------------

    static void testHttpClient() throws Exception {
        System.out.println("\n-- WhispernetHttpClient --");

        // --- HTTPS guard: no socket connection attempted ---
        {
            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", findFreePort());
            WhispernetHttpClient client = new WhispernetHttpClient(proxy);
            HttpResponse resp = client.get("https://example.com/secure");
            check("HTTPS returns transport error", resp.isTransportError());
            check("HTTPS error mentions TLS", resp.getError().indexOf("TLS") >= 0
                                           || resp.getError().indexOf("tls") >= 0
                                           || resp.getError().indexOf("not supported") >= 0);
        }

        // --- null proxy rejected ---
        {
            try {
                new WhispernetHttpClient(null);
                check("null proxy rejected", false);
            } catch (IllegalArgumentException e) {
                check("null proxy rejected", true);
            }
        }

        // --- GET round-trip via fake proxy ---
        {
            String fakeResponse = "HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\nhello";
            FakeProxyServer server = new FakeProxyServer(fakeResponse.getBytes("US-ASCII"));
            server.start();

            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            WhispernetHttpClient client = new WhispernetHttpClient(proxy);
            HttpResponse resp = client.get("http://example.com/test");

            server.stop();
            check("GET status 200", resp.getStatusCode() == 200);
            check("GET reason OK",  "OK".equals(resp.getReason()));
            check("GET body hello", new String(resp.getBody()).equals("hello"));
            check("GET request contains absolute URL",
                  server.getCapturedRequest().indexOf("GET http://example.com/test") >= 0);
            check("GET request HTTP/1.0",
                  server.getCapturedRequest().indexOf("HTTP/1.0") >= 0);
        }

        // --- POST round-trip ---
        {
            String fakeResponse = "HTTP/1.0 201 Created\r\nContent-Length: 0\r\n\r\n";
            FakeProxyServer server = new FakeProxyServer(fakeResponse.getBytes("US-ASCII"));
            server.start();

            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            WhispernetHttpClient client = new WhispernetHttpClient(proxy);
            HttpResponse resp = client.post("http://example.com/api", "data".getBytes("US-ASCII"));

            server.stop();
            check("POST status 201", resp.getStatusCode() == 201);
            check("POST sends Content-Length",
                  server.getCapturedRequest().indexOf("Content-Length: 4") >= 0);
        }

        // --- CRLF injection rejected ---
        {
            FakeProxyServer server = new FakeProxyServer(new byte[0]);
            server.start();

            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            WhispernetHttpClient client = new WhispernetHttpClient(proxy);
            Vector headers = new Vector();
            headers.addElement(new String[]{"X-Evil", "value\r\nInjected: header"});
            HttpRequest req = new HttpRequest("GET", "http://example.com/", headers, new byte[0]);
            HttpResponse resp = client.execute(req);
            server.stop();
            check("CRLF injection returns transport error", resp.isTransportError());
        }

        // --- connect failure ---
        {
            WhispernetProxy deadProxy = new WhispernetProxy("127.0.0.1", findFreePort());
            WhispernetHttpClient client = new WhispernetHttpClient(deadProxy);
            HttpResponse resp = client.get("http://example.com/");
            check("connect failure returns transport error", resp.isTransportError());
        }
    }

    // -------------------------------------------------------------------------
    // WhispernetSocketClient tests
    // -------------------------------------------------------------------------

    static void testSocketClient() throws Exception {
        System.out.println("\n-- WhispernetSocketClient --");

        // null proxy rejected
        try {
            new WhispernetSocketClient(null);
            check("null proxy rejected by socket client", false);
        } catch (IllegalArgumentException e) {
            check("null proxy rejected by socket client", true);
        }

        // CONNECT tunnel success
        {
            String tunnelResponse = "HTTP/1.0 200 Connection established\r\n\r\n";
            FakeProxyServer server = new FakeProxyServer(tunnelResponse.getBytes("US-ASCII"));
            server.start();

            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            WhispernetSocketClient client = new WhispernetSocketClient(proxy);
            Socket tunneled = client.connect("target.example.com", 443);
            server.stop();
            check("CONNECT returns socket", tunneled != null);
            check("CONNECT request format",
                  server.getCapturedRequest().indexOf("CONNECT target.example.com:443") >= 0);
            if (tunneled != null) {
                try { tunneled.close(); } catch (IOException ignored) {}
            }
        }

        // CONNECT tunnel rejected (407)
        {
            String tunnelResponse = "HTTP/1.0 407 Proxy Authentication Required\r\n\r\n";
            FakeProxyServer server = new FakeProxyServer(tunnelResponse.getBytes("US-ASCII"));
            server.start();

            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            WhispernetSocketClient client = new WhispernetSocketClient(proxy);
            boolean threw = false;
            try {
                client.connect("target.example.com", 443);
            } catch (IOException e) {
                threw = true;
            }
            server.stop();
            check("CONNECT 407 throws IOException", threw);
        }

        // Connect failure
        {
            WhispernetProxy deadProxy = new WhispernetProxy("127.0.0.1", findFreePort());
            WhispernetSocketClient client = new WhispernetSocketClient(deadProxy);
            boolean threw = false;
            try {
                client.connect("example.com", 80);
            } catch (IOException e) {
                threw = true;
            }
            check("connect failure throws IOException", threw);
        }
    }

    // -------------------------------------------------------------------------
    // Minimal fake proxy server (in-process, single connection)
    // -------------------------------------------------------------------------

    static class FakeProxyServer {
        private final byte[] responseBytes;
        private ServerSocket serverSocket;
        private Thread thread;
        private final StringBuffer captured = new StringBuffer();
        private volatile boolean running = false;

        FakeProxyServer(byte[] responseBytes) {
            this.responseBytes = responseBytes;
        }

        void start() throws IOException {
            serverSocket = new ServerSocket(0);
            running = true;
            thread = new Thread(new Runnable() {
                public void run() {
                    try {
                        Socket client = serverSocket.accept();
                        client.setSoTimeout(3000);
                        InputStream in = client.getInputStream();
                        byte[] buf = new byte[4096];
                        int read;
                        try {
                            while ((read = in.read(buf)) > 0) {
                                captured.append(new String(buf, 0, read, "US-ASCII"));
                                // Stop reading headers once we see double-CRLF
                                String s = captured.toString();
                                if (s.indexOf("\r\n\r\n") >= 0) break;
                            }
                        } catch (IOException ignored) {}
                        OutputStream out = client.getOutputStream();
                        if (responseBytes.length > 0) {
                            out.write(responseBytes);
                            out.flush();
                        }
                        client.close();
                    } catch (IOException e) {
                        // Server stopped or no connection — expected in failure tests
                    }
                }
            });
            thread.setDaemon(true);
            thread.start();
        }

        void stop() {
            running = false;
            try { if (serverSocket != null) serverSocket.close(); } catch (IOException ignored) {}
            try { if (thread != null) thread.join(1000); } catch (InterruptedException ignored) {}
        }

        int getPort() {
            return serverSocket.getLocalPort();
        }

        String getCapturedRequest() {
            return captured.toString();
        }
    }

    static int findFreePort() throws IOException {
        ServerSocket s = new ServerSocket(0);
        int port = s.getLocalPort();
        s.close();
        return port;
    }

    // -------------------------------------------------------------------------
    // HttpIpcCodec tests
    // -------------------------------------------------------------------------

    static void testHttpIpcCodec() {
        System.out.println("\n-- HttpIpcCodec --");

        // Round-trip request: GET with headers and empty body
        {
            HttpRequest req = new HttpRequest("GET", "http://example.com/path");
            req = req.withHeader("Host", "example.com").withHeader("Accept", "text/html");
            byte[] payload = HttpIpcCodec.encodeRequest(req);
            check("encode request non-null", payload != null);
            HttpRequest decoded = HttpIpcCodec.decodeRequest(payload);
            check("decode request non-null", decoded != null);
            if (decoded != null) {
                check("request method round-trip", "GET".equals(decoded.getMethod()));
                check("request url round-trip",    "http://example.com/path".equals(decoded.getUrl()));
                check("request header count",      decoded.getHeaders().size() == 2);
                check("request body empty",        decoded.getBody().length == 0);
            }
        }

        // Round-trip request: POST with binary body
        {
            byte[] body = {0x00, 0x01, (byte) 0xFF};
            HttpRequest req = new HttpRequest("POST", "http://example.com/api",
                                              new java.util.Vector(), body);
            byte[] payload = HttpIpcCodec.encodeRequest(req);
            HttpRequest decoded = HttpIpcCodec.decodeRequest(payload);
            check("POST binary body round-trip", decoded != null
                  && decoded.getBody().length == 3
                  && decoded.getBody()[2] == (byte) 0xFF);
        }

        // Truncated payload → null
        {
            HttpRequest req = new HttpRequest("GET", "http://x.com/");
            byte[] payload = HttpIpcCodec.encodeRequest(req);
            byte[] truncated = new byte[payload.length / 2];
            System.arraycopy(payload, 0, truncated, 0, truncated.length);
            check("truncated request payload → null", HttpIpcCodec.decodeRequest(truncated) == null);
        }

        // Method > 255 bytes throws
        {
            StringBuffer sb = new StringBuffer();
            for (int i = 0; i < 256; i++) sb.append('X');
            try {
                HttpIpcCodec.encodeRequest(new HttpRequest(sb.toString(), "http://x.com/"));
                check("method > 255 throws", false);
            } catch (IllegalArgumentException e) {
                check("method > 255 throws", true);
            }
        }

        // Round-trip response: 200 OK with headers and body
        {
            java.util.Vector headers = new java.util.Vector();
            headers.addElement(new String[]{"Content-Type", "text/plain"});
            HttpResponse resp = new HttpResponse(200, "OK", headers, "hi".getBytes());
            byte[] payload = HttpIpcCodec.encodeResponse(resp);
            check("encode response non-null", payload != null);
            HttpResponse decoded = HttpIpcCodec.decodeResponse(payload);
            check("decode response non-null", decoded != null);
            if (decoded != null) {
                check("response status round-trip", decoded.getStatusCode() == 200);
                check("response reason round-trip", "OK".equals(decoded.getReason()));
                check("response header count",      decoded.getHeaders().size() == 1);
                check("response body round-trip",   decoded.getBody().length == 2);
            }
        }

        // Transport error response (status 0)
        {
            HttpResponse err = new HttpResponse("network timeout");
            byte[] payload = HttpIpcCodec.encodeResponse(err);
            HttpResponse decoded = HttpIpcCodec.decodeResponse(payload);
            check("transport error response round-trip", decoded != null
                  && decoded.getStatusCode() == 0
                  && "network timeout".equals(decoded.getReason()));
        }

        // Wrong schema version → null
        {
            java.util.Vector h = new java.util.Vector();
            HttpResponse resp = new HttpResponse(200, "OK", h, new byte[0]);
            byte[] payload = HttpIpcCodec.encodeResponse(resp);
            if (payload != null && payload.length > 0) {
                payload[0] = 0x02;
            }
            check("wrong schema version → null", HttpIpcCodec.decodeResponse(payload) == null);
        }

        // null payload → null
        check("null request payload → null", HttpIpcCodec.decodeRequest(null) == null);
        check("null response payload → null", HttpIpcCodec.decodeResponse(null) == null);
    }

    // -------------------------------------------------------------------------
    // NetworkRelayHandler tests
    // -------------------------------------------------------------------------

    static void testNetworkRelayHandler() throws Exception {
        System.out.println("\n-- NetworkRelayHandler --");

        final ByteArrayOutputStream nativeOutput = new ByteArrayOutputStream();
        ProcessLauncher launcher = new ProcessLauncher() {
            public Process launch(String[] command, File workingDirectory) {
                return new Process() {
                    public OutputStream getOutputStream() { return nativeOutput; }
                    public InputStream getInputStream() {
                        return new InputStream() {
                            public int read() throws IOException {
                                try { Thread.sleep(5000); } catch (InterruptedException e) {
                                    Thread.currentThread().interrupt();
                                }
                                return -1;
                            }
                        };
                    }
                    public InputStream getErrorStream() { return new ByteArrayInputStream(new byte[0]); }
                    public int waitFor() { return 0; }
                    public int exitValue() { return 0; }
                    public void destroy() {}
                };
            }
        };
        NativeProcessSupervisor supervisor = new NativeProcessSupervisor(launcher, new File("/tmp"));
        final HttpResponse[] responseHolder = new HttpResponse[1];
        final String[] errorHolder = new String[1];
        NetworkRelayHandler handler = new NetworkRelayHandler(supervisor, null);
        supervisor.start(new File("/tmp/native"), handler);

        int requestId = handler.sendHttpRequest(
            new HttpRequest("GET", "http://example.com/"),
            new HttpResponseListener() {
                public void onHttpResponse(int id, HttpResponse response) {
                    responseHolder[0] = response;
                }
                public void onHttpError(int id, String message) {
                    errorHolder[0] = message;
                }
            });
        NativeMessage requestFrame = NativeMessage.readFrom(
            new ByteArrayInputStream(nativeOutput.toByteArray()));
        check("relay sends HTTP_REQUEST frame",
              requestFrame.getType() == NativeMessage.TYPE_HTTP_REQUEST);
        check("relay allocates request ID", requestFrame.getRequestId() == requestId);

        HttpResponse nativeResponse = new HttpResponse(200, "OK", new Vector(),
                                                       "ok".getBytes("US-ASCII"));
        handler.onNativeMessage(new NativeMessage(
            NativeMessage.TYPE_HTTP_RESPONSE, requestId,
            HttpIpcCodec.encodeResponse(nativeResponse)));
        check("relay dispatches response", responseHolder[0] != null &&
              responseHolder[0].getStatusCode() == 200);
        check("relay has no response error", errorHolder[0] == null);

        final String[] terminationError = new String[1];
        handler.sendHttpRequest(new HttpRequest("GET", "http://example.com/"),
            new HttpResponseListener() {
                public void onHttpResponse(int id, HttpResponse response) {}
                public void onHttpError(int id, String message) {
                    terminationError[0] = message;
                }
            });
        handler.onNativeProcessTerminated(1);
        check("relay reports process termination", terminationError[0] != null);
        supervisor.stop();
    }

    // -------------------------------------------------------------------------
    // main
    // -------------------------------------------------------------------------

    public static void main(String[] args) throws Exception {
        System.out.println("Running WhispernetNetworkTest...");

        testProxyValueType();
        testHttpRequestValueType();
        testHttpResponseValueType();
        testHttpClient();
        testSocketClient();
        testHttpIpcCodec();
        testNetworkRelayHandler();

        System.out.println("\n" + passed + "/" + total + " tests passed");
        if (passed != total) {
            System.exit(1);
        }
    }
}
