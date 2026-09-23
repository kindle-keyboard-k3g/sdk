package com.amazon.kindle.bridge.network;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.URL;
import java.util.Vector;

/**
 * HTTP client that routes requests through the Whispernet HTTP proxy.
 * Compatible with Java 1.4 / CDC 1.1.
 */
public class WhispernetHttpClient {

    private static final int MAX_BODY_BYTES = 1024 * 1024;
    private static final int SOCKET_TIMEOUT_MS = 30000;
    private static final int MAX_REDIRECTS = 5;

    private final WhispernetProxy proxy;

    public WhispernetHttpClient(WhispernetProxy proxy) {
        if (proxy == null) {
            throw new IllegalArgumentException("WhispernetProxy must not be null");
        }
        this.proxy = proxy;
    }

    public HttpResponse get(String url) {
        return execute(new HttpRequest("GET", url));
    }

    public HttpResponse post(String url, byte[] body) {
        return post(url, new Vector(), body);
    }

    /** Executes a POST with caller-supplied ordered header pairs. */
    public HttpResponse post(String url, Vector headers, byte[] body) {
        Vector requestHeaders = headers != null ? headers : new Vector();
        byte[] requestBody = body != null ? body : new byte[0];
        boolean hasContentLength = false;
        for (int index = 0; index < requestHeaders.size(); index++) {
            String[] pair = (String[]) requestHeaders.elementAt(index);
            if ("content-length".equalsIgnoreCase(pair[0])) {
                hasContentLength = true;
            }
        }
        if (!hasContentLength) {
            requestHeaders = new Vector(requestHeaders);
            requestHeaders.addElement(new String[]{"Content-Length", String.valueOf(requestBody.length)});
        }
        return execute(new HttpRequest("POST", url, requestHeaders, requestBody));
    }

    /** Executes a request and follows at most five safe redirects. */
    public HttpResponse execute(HttpRequest request) {
        if (request == null) {
            return new HttpResponse("HTTP request must not be null");
        }

        String method = request.getMethod();
        String currentUrl = request.getUrl();
        Vector headers = request.getHeaders();
        byte[] body = request.getBody();
        int redirectsRemaining = MAX_REDIRECTS;

        while (true) {
            try {
                validateRequest(method, currentUrl);
                HttpRequest current = new HttpRequest(method, currentUrl, headers, body);
                HttpResponse response = executeOnce(current);
                if (!isRedirect(response) || redirectsRemaining == 0) {
                    return response;
                }

                String location = response.getHeader("Location");
                if (location == null || location.trim().length() == 0) {
                    return response;
                }
                String nextUrl = resolveUrl(currentUrl, location.trim());
                rejectHttpsDowngrade(currentUrl, nextUrl);
                if (response.getStatusCode() == 303) {
                    method = "GET";
                    body = new byte[0];
                    headers = withoutEntityHeaders(headers);
                }
                currentUrl = nextUrl;
                redirectsRemaining--;
            } catch (IOException error) {
                return new HttpResponse("Transport error: " + error.getMessage());
            } catch (RuntimeException error) {
                return new HttpResponse("Transport error: " + error.getMessage());
            }
        }
    }

    private HttpResponse executeOnce(HttpRequest request) throws IOException {
        String scheme = schemeOf(request.getUrl());
        if ("https".equals(scheme)) {
            throw new IOException("TLS not supported over Whispernet proxy");
        }

        Socket socket = null;
        try {
            socket = new Socket(proxy.getHost(), proxy.getPort());
            socket.setSoTimeout(SOCKET_TIMEOUT_MS);
            sendRequest(socket.getOutputStream(), request);
            return readResponse(socket.getInputStream());
        } finally {
            if (socket != null) {
                try {
                    socket.close();
                } catch (IOException ignored) {
                }
            }
        }
    }

    private static void validateRequest(String method, String url) throws IOException {
        if (method == null || method.length() == 0) {
            throw new IOException("HTTP method must not be empty");
        }
        if (url == null || url.length() == 0) {
            throw new IOException("HTTP URL must not be empty");
        }
        rejectIfCrlfOrNul(method, "method");
        rejectIfCrlfOrNul(url, "URL");
        String scheme = schemeOf(url);
        if (!"http".equals(scheme) && !"https".equals(scheme)) {
            throw new IOException("Unsupported URL scheme: " + scheme);
        }
    }

    private static String schemeOf(String url) throws IOException {
        int separator = url.indexOf("://");
        if (separator <= 0) {
            throw new IOException("URL is missing a scheme");
        }
        return url.substring(0, separator).toLowerCase();
    }

    private static void sendRequest(OutputStream out, HttpRequest request)
            throws IOException {
        Vector headers = request.getHeaders();
        for (int index = 0; index < headers.size(); index++) {
            String[] pair = (String[]) headers.elementAt(index);
            rejectIfCrlfOrNul(pair[0], "header name");
            rejectIfCrlfOrNul(pair[1], "header value");
        }

        DataOutputStream output = new DataOutputStream(out);
        output.writeBytes(request.getMethod() + " " + request.getUrl() + " HTTP/1.0\r\n");
        output.writeBytes("Connection: close\r\n");

        boolean hasContentLength = false;
        for (int index = 0; index < headers.size(); index++) {
            String[] pair = (String[]) headers.elementAt(index);
            output.writeBytes(pair[0] + ": " + pair[1] + "\r\n");
            if ("content-length".equalsIgnoreCase(pair[0])) {
                hasContentLength = true;
            }
        }
        byte[] body = request.getBody();
        if (body.length > 0 && !hasContentLength) {
            output.writeBytes("Content-Length: " + body.length + "\r\n");
        }
        output.writeBytes("\r\n");
        output.write(body);
        output.flush();
    }

    private HttpResponse readResponse(InputStream input) throws IOException {
        String statusLine = readRawLine(input);
        if (statusLine == null || !statusLine.startsWith("HTTP/")) {
            throw new IOException("Invalid status line: " + statusLine);
        }

        int firstSpace = statusLine.indexOf(' ');
        if (firstSpace < 0) {
            throw new IOException("Malformed status line: " + statusLine);
        }
        String remainder = statusLine.substring(firstSpace + 1).trim();
        int secondSpace = remainder.indexOf(' ');
        String statusToken = secondSpace < 0
            ? remainder : remainder.substring(0, secondSpace).trim();
        int statusCode;
        try {
            statusCode = Integer.parseInt(statusToken);
        } catch (NumberFormatException error) {
            throw new IOException("Invalid status code: " + statusToken);
        }
        if (statusCode < 100 || statusCode > 999) {
            throw new IOException("Invalid status code: " + statusToken);
        }
        String reason = secondSpace < 0
            ? "" : remainder.substring(secondSpace + 1).trim();

        Vector headers = new Vector();
        int contentLength = -1;
        boolean chunked = false;
        String line;
        while ((line = readRawLine(input)) != null && line.length() > 0) {
            int colon = line.indexOf(':');
            if (colon <= 0) {
                continue;
            }
            String name = line.substring(0, colon).trim();
            String value = line.substring(colon + 1).trim();
            headers.addElement(new String[]{name, value});
            if ("content-length".equalsIgnoreCase(name)) {
                try {
                    contentLength = Integer.parseInt(value);
                } catch (NumberFormatException error) {
                    throw new IOException("Invalid Content-Length: " + value);
                }
                if (contentLength < 0 || contentLength > MAX_BODY_BYTES) {
                    throw new IOException("Content-Length out of range: " + contentLength);
                }
            }
            if ("transfer-encoding".equalsIgnoreCase(name) && containsToken(value, "chunked")) {
                chunked = true;
            }
        }
        if (line == null) {
            throw new IOException("Unexpected EOF while reading response headers");
        }

        byte[] body;
        if (statusCode / 100 == 1 || statusCode == 204 || statusCode == 304) {
            body = new byte[0];
        } else if (chunked) {
            body = readChunkedBody(input);
        } else if (contentLength >= 0) {
            body = readFixedBody(input, contentLength);
        } else {
            body = readCloseDelimitedBody(input);
        }
        return new HttpResponse(statusCode, reason, headers, body);
    }

    private static byte[] readFixedBody(InputStream input, int length) throws IOException {
        byte[] body = new byte[length];
        int offset = 0;
        while (offset < length) {
            int count = input.read(body, offset, length - offset);
            if (count < 0) {
                throw new IOException("Unexpected EOF while reading response body");
            }
            offset += count;
        }
        return body;
    }

    private static byte[] readCloseDelimitedBody(InputStream input) throws IOException {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int count;
        while ((count = input.read(buffer)) != -1) {
            if (count > MAX_BODY_BYTES - body.size()) {
                throw new IOException("Response body exceeds 1 MiB limit");
            }
            body.write(buffer, 0, count);
        }
        return body.toByteArray();
    }

    private static byte[] readChunkedBody(InputStream input) throws IOException {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        while (true) {
            String sizeLine = readRawLine(input);
            if (sizeLine == null) {
                throw new IOException("Unexpected EOF while reading chunk size");
            }
            int semicolon = sizeLine.indexOf(';');
            String token = (semicolon >= 0 ? sizeLine.substring(0, semicolon) : sizeLine).trim();
            if (token.length() == 0) {
                throw new IOException("Empty chunk size");
            }
            long chunkSize;
            try {
                chunkSize = Long.parseLong(token, 16);
            } catch (NumberFormatException error) {
                throw new IOException("Invalid chunk size: " + token);
            }
            if (chunkSize < 0 || chunkSize > MAX_BODY_BYTES - body.size()) {
                throw new IOException("Chunked response exceeds 1 MiB limit");
            }
            if (chunkSize == 0) {
                while (true) {
                    String trailer = readRawLine(input);
                    if (trailer == null) {
                        throw new IOException("Unexpected EOF while reading chunk trailers");
                    }
                    if (trailer.length() == 0) {
                        return body.toByteArray();
                    }
                }
            }

            byte[] chunk = readFixedBody(input, (int) chunkSize);
            body.write(chunk, 0, chunk.length);
            requireCrlf(input);
        }
    }

    private static void requireCrlf(InputStream input) throws IOException {
        int carriageReturn = input.read();
        int lineFeed = input.read();
        if (carriageReturn != '\r' || lineFeed != '\n') {
            throw new IOException("Invalid chunk terminator");
        }
    }

    private static String readRawLine(InputStream input) throws IOException {
        StringBuffer line = new StringBuffer();
        int value;
        while ((value = input.read()) != -1) {
            if (value == '\n') {
                int length = line.length();
                if (length > 0 && line.charAt(length - 1) == '\r') {
                    line.deleteCharAt(length - 1);
                }
                return line.toString();
            }
            line.append((char) value);
            if (line.length() > 8192) {
                throw new IOException("HTTP line exceeds 8192 bytes");
            }
        }
        return line.length() > 0 ? line.toString() : null;
    }

    private static boolean containsToken(String value, String token) {
        String[] parts = value.split(",");
        for (int index = 0; index < parts.length; index++) {
            if (token.equalsIgnoreCase(parts[index].trim())) {
                return true;
            }
        }
        return false;
    }

    private static boolean isRedirect(HttpResponse response) {
        int status = response.getStatusCode();
        return status == 301 || status == 302 || status == 303 ||
               status == 307 || status == 308;
    }

    private static String resolveUrl(String base, String location) throws IOException {
        try {
            return new URL(new URL(base), location).toExternalForm();
        } catch (IOException error) {
            throw new IOException("Invalid redirect Location: " + location);
        }
    }

    private static void rejectHttpsDowngrade(String previous, String next)
            throws IOException {
        String previousScheme = schemeOf(previous);
        String nextScheme = schemeOf(next);
        if ("https".equals(previousScheme) && "http".equals(nextScheme)) {
            throw new IOException("HTTPS to HTTP redirect is not allowed");
        }
    }

    private static Vector withoutEntityHeaders(Vector headers) {
        Vector result = new Vector();
        for (int index = 0; index < headers.size(); index++) {
            String[] pair = (String[]) headers.elementAt(index);
            if (!"content-length".equalsIgnoreCase(pair[0]) &&
                !"content-type".equalsIgnoreCase(pair[0])) {
                result.addElement(new String[]{pair[0], pair[1]});
            }
        }
        return result;
    }

    private static void rejectIfCrlfOrNul(String value, String field)
            throws IOException {
        if (value.indexOf('\r') >= 0 || value.indexOf('\n') >= 0 ||
            value.indexOf('\0') >= 0) {
            throw new IOException("Invalid " + field + ": contains CRLF or NUL");
        }
    }
}
