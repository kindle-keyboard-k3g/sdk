package com.amazon.kindle.bridge.network;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.Vector;

/**
 * HTTP client that routes requests through the Whispernet 3G proxy.
 * Uses raw TCP sockets (no java.net.Proxy — Java 5+); sends absolute-form
 * GET/POST to the proxy host directly.
 * Compatible with Java 1.4 / CDC 1.1.
 */
public class WhispernetHttpClient {

    private static final int MAX_BODY_BYTES = 1024 * 1024; // 1 MiB
    private static final int SOCKET_TIMEOUT_MS = 30000;    // 30 s

    private final WhispernetProxy proxy;

    /**
     * Constructs a client bound to the given proxy configuration.
     *
     * @param proxy configured Whispernet proxy; must not be null
     * @throws IllegalArgumentException if proxy is null
     */
    public WhispernetHttpClient(WhispernetProxy proxy) {
        if (proxy == null) {
            throw new IllegalArgumentException("WhispernetProxy must not be null");
        }
        this.proxy = proxy;
    }

    /**
     * Executes an HTTP GET via the Whispernet proxy.
     *
     * @param url absolute HTTP URL to fetch
     * @return parsed HttpResponse; status_code == 0 on transport error
     */
    public HttpResponse get(String url) {
        HttpRequest req = new HttpRequest("GET", url);
        return execute(req);
    }

    /**
     * Executes an HTTP POST with a byte-array body via the Whispernet proxy.
     *
     * @param url  absolute HTTP URL to post to
     * @param body request body bytes
     * @return parsed HttpResponse; status_code == 0 on transport error
     */
    public HttpResponse post(String url, byte[] body) {
        Vector headers = new Vector();
        headers.addElement(new String[]{"Content-Length", String.valueOf(body.length)});
        HttpRequest req = new HttpRequest("POST", url, headers, body);
        return execute(req);
    }

    /**
     * Executes any HTTP request through the Whispernet proxy.
     *
     * @param request request to send
     * @return HttpResponse; status_code == 0 on transport error
     */
    public HttpResponse execute(HttpRequest request) {
        if (request.getUrl().startsWith("https://")) {
            return new HttpResponse("TLS not supported over Whispernet proxy");
        }
        Socket socket = null;
        try {
            socket = new Socket(proxy.getHost(), proxy.getPort());
            socket.setSoTimeout(SOCKET_TIMEOUT_MS);
            sendRequest(socket.getOutputStream(), request);
            return readResponse(socket.getInputStream());
        } catch (IOException e) {
            return new HttpResponse("Transport error: " + e.getMessage());
        } finally {
            if (socket != null) {
                try { socket.close(); } catch (IOException ignored) {}
            }
        }
    }

    private void sendRequest(OutputStream out, HttpRequest req) throws IOException {
        // Validate headers for CRLF injection before writing anything
        Vector headers = req.getHeaders();
        for (int i = 0; i < headers.size(); i++) {
            String[] pair = (String[]) headers.elementAt(i);
            rejectIfCrlf(pair[0], "header name");
            rejectIfCrlf(pair[1], "header value");
        }

        DataOutputStream dos = new DataOutputStream(out);
        // Absolute-form request line for proxy
        dos.writeBytes(req.getMethod() + " " + req.getUrl() + " HTTP/1.0\r\n");
        dos.writeBytes("Connection: close\r\n");

        boolean hasContentLength = false;
        for (int i = 0; i < headers.size(); i++) {
            String[] pair = (String[]) headers.elementAt(i);
            dos.writeBytes(pair[0] + ": " + pair[1] + "\r\n");
            if ("content-length".equalsIgnoreCase(pair[0])) {
                hasContentLength = true;
            }
        }
        byte[] body = req.getBody();
        if (body.length > 0 && !hasContentLength) {
            dos.writeBytes("Content-Length: " + body.length + "\r\n");
        }
        dos.writeBytes("\r\n");
        if (body.length > 0) {
            dos.write(body);
        }
        dos.flush();
    }

    private HttpResponse readResponse(InputStream in) throws IOException {
        // Read headers one byte at a time to avoid BufferedReader consuming body bytes
        String statusLine = readRawLine(in);
        if (statusLine == null || !statusLine.startsWith("HTTP/")) {
            throw new IOException("Invalid status line: " + statusLine);
        }
        int firstSpace = statusLine.indexOf(' ');
        if (firstSpace < 0) {
            throw new IOException("Malformed status line: " + statusLine);
        }
        String rest = statusLine.substring(firstSpace + 1).trim();
        int secondSpace = rest.indexOf(' ');
        int statusCode;
        String reason;
        if (secondSpace < 0) {
            statusCode = Integer.parseInt(rest.trim());
            reason = "";
        } else {
            statusCode = Integer.parseInt(rest.substring(0, secondSpace).trim());
            reason = rest.substring(secondSpace + 1).trim();
        }

        Vector headers = new Vector();
        String line;
        int contentLength = -1;
        while ((line = readRawLine(in)) != null && line.length() > 0) {
            int colon = line.indexOf(':');
            if (colon > 0) {
                String name  = line.substring(0, colon).trim();
                String value = line.substring(colon + 1).trim();
                headers.addElement(new String[]{name, value});
                if ("content-length".equalsIgnoreCase(name)) {
                    contentLength = Integer.parseInt(value.trim());
                    if (contentLength < 0 || contentLength > MAX_BODY_BYTES) {
                        throw new IOException("Content-Length out of range: " + contentLength);
                    }
                }
            }
        }

        byte[] body = readBody(in, contentLength);
        return new HttpResponse(statusCode, reason, headers, body);
    }

    /** Reads one CRLF-terminated line from the stream, one byte at a time. */
    private static String readRawLine(InputStream in) throws IOException {
        StringBuffer sb = new StringBuffer();
        int b;
        while ((b = in.read()) != -1) {
            if (b == '\n') {
                int len = sb.length();
                if (len > 0 && sb.charAt(len - 1) == '\r') {
                    sb.deleteCharAt(len - 1);
                }
                return sb.toString();
            }
            sb.append((char) b);
        }
        return sb.length() > 0 ? sb.toString() : null;
    }

    private byte[] readBody(InputStream in, int contentLength) throws IOException {
        if (contentLength == 0) {
            return new byte[0];
        }
        byte[] buf = new byte[4096];
        java.io.ByteArrayOutputStream baos = new java.io.ByteArrayOutputStream();
        int remaining = (contentLength > 0) ? contentLength : MAX_BODY_BYTES;
        int read;
        while (remaining > 0 && (read = in.read(buf, 0, Math.min(buf.length, remaining))) != -1) {
            if (baos.size() + read > MAX_BODY_BYTES) {
                throw new IOException("Response body exceeds 1 MiB limit");
            }
            baos.write(buf, 0, read);
            if (contentLength > 0) {
                remaining -= read;
            }
        }
        return baos.toByteArray();
    }

    private static void rejectIfCrlf(String value, String field) throws IOException {
        if (value.indexOf('\r') >= 0 || value.indexOf('\n') >= 0) {
            throw new IOException("CRLF injection attempt in " + field + ": " + value);
        }
    }
}
