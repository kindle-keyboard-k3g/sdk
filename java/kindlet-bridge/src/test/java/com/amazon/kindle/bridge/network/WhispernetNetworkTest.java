package com.amazon.kindle.bridge.network;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
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
                  && decoded.getReason() != null
                  && decoded.getReason().length() == 0);
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

        // null httpClient rejected
        try {
            new NetworkRelayHandler(null, new ByteArrayOutputStream(), null);
            check("null httpClient rejected", false);
        } catch (IllegalArgumentException e) {
            check("null httpClient rejected", true);
        }

        // null stdin rejected
        try {
            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", findFreePort());
            new NetworkRelayHandler(new WhispernetHttpClient(proxy), null, null);
            check("null stdin rejected", false);
        } catch (IllegalArgumentException e) {
            check("null stdin rejected", true);
        }

        // Non-HTTP_REQUEST type is silently ignored
        {
            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", findFreePort());
            NetworkRelayHandler handler = new NetworkRelayHandler(
                new WhispernetHttpClient(proxy), stdout, null);
            com.amazon.kindle.bridge.NativeMessage pingMsg = new com.amazon.kindle.bridge.NativeMessage(
                com.amazon.kindle.bridge.NativeMessage.TYPE_PING, 0, new byte[0]);
            handler.handleMessage(pingMsg);
            check("non-HTTP_REQUEST type ignored (no output)", stdout.size() == 0);
        }

        // Malformed payload → transport-error response written to stdout
        {
            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", findFreePort());
            NetworkRelayHandler handler = new NetworkRelayHandler(
                new WhispernetHttpClient(proxy), stdout, null);
            com.amazon.kindle.bridge.NativeMessage badMsg = new com.amazon.kindle.bridge.NativeMessage(
                com.amazon.kindle.bridge.NativeMessage.TYPE_HTTP_REQUEST, 42, new byte[]{0x00});
            handler.handleMessage(badMsg);
            check("malformed payload → response written", stdout.size() > 0);
            // Parse the written frame and verify it is a transport error
            com.amazon.kindle.bridge.NativeMessage reply =
                com.amazon.kindle.bridge.NativeMessage.readFrom(
                    new ByteArrayInputStream(stdout.toByteArray()));
            check("malformed payload → HTTP_RESPONSE type",
                  reply.getType() == com.amazon.kindle.bridge.NativeMessage.TYPE_HTTP_RESPONSE);
            check("malformed payload → request ID echoed", reply.getRequestId() == 42);
            HttpResponse decodedResp = HttpIpcCodec.decodeResponse(reply.getPayload());
            check("malformed payload → transport error response",
                  decodedResp != null && decodedResp.isTransportError());
        }

        // Successful relay via fake proxy: listener is called with response
        {
            String fakeReply = "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n\r\nhi";
            FakeProxyServer server = new FakeProxyServer(fakeReply.getBytes("US-ASCII"));
            server.start();

            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            WhispernetProxy proxy = new WhispernetProxy("127.0.0.1", server.getPort());
            final HttpResponse[] listenerResult = new HttpResponse[1];
            final int[] listenerRequestId = new int[]{-1};
            NetworkRelayHandler handler = new NetworkRelayHandler(
                new WhispernetHttpClient(proxy), stdout,
                new HttpResponseListener() {
                    public void onHttpResponse(int requestId, HttpResponse response) {
                        listenerRequestId[0] = requestId;
                        listenerResult[0] = response;
                    }
                });

            HttpRequest req = new HttpRequest("GET", "http://example.com/");
            byte[] encodedReq = HttpIpcCodec.encodeRequest(req);
            com.amazon.kindle.bridge.NativeMessage reqMsg = new com.amazon.kindle.bridge.NativeMessage(
                com.amazon.kindle.bridge.NativeMessage.TYPE_HTTP_REQUEST, 99, encodedReq);
            handler.handleMessage(reqMsg);
            server.stop();

            check("relay listener called", listenerResult[0] != null);
            check("relay listener request ID", listenerRequestId[0] == 99);
            check("relay response status 200",
                  listenerResult[0] != null && listenerResult[0].getStatusCode() == 200);
            // Verify frame written to stdout
            com.amazon.kindle.bridge.NativeMessage reply =
                com.amazon.kindle.bridge.NativeMessage.readFrom(
                    new ByteArrayInputStream(stdout.toByteArray()));
            check("relay frame type HTTP_RESPONSE",
                  reply.getType() == com.amazon.kindle.bridge.NativeMessage.TYPE_HTTP_RESPONSE);
            check("relay frame request ID echoed", reply.getRequestId() == 99);
        }
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
