package com.amazon.kindle.bridge.network;

import java.util.Vector;

/**
 * Immutable HTTP request value type for Whispernet proxy calls.
 * Compatible with Java 1.4 / CDC 1.1 (no generics, no enhanced for).
 */
public final class HttpRequest {

    private final String method;
    private final String url;
    private final Vector headers;
    private final byte[] body;

    public HttpRequest(String method, String url) {
        this(method, url, new Vector(), new byte[0]);
    }

    public HttpRequest(String method, String url, Vector headers, byte[] body) {
        if (method == null || method.trim().length() == 0) {
            throw new IllegalArgumentException("HTTP method must not be empty");
        }
        if (url == null || url.trim().length() == 0) {
            throw new IllegalArgumentException("HTTP url must not be empty");
        }
        this.method = method;
        this.url = url;
        this.headers = copyHeaders(headers);
        this.body = copyBytes(body);
    }

    public String getMethod() {
        return method;
    }

    public String getUrl() {
        return url;
    }

    /** Returns a defensive copy of the ordered header list. */
    public Vector getHeaders() {
        return copyHeaders(headers);
    }

    /** Returns a defensive copy of the request body. */
    public byte[] getBody() {
        return copyBytes(body);
    }

    public HttpRequest withHeader(String name, String value) {
        Vector newHeaders = copyHeaders(headers);
        newHeaders.addElement(new String[]{name, value});
        return new HttpRequest(method, url, newHeaders, body);
    }

    public HttpRequest withBody(byte[] newBody) {
        return new HttpRequest(method, url, headers, newBody);
    }

    private static Vector copyHeaders(Vector source) {
        Vector result = new Vector();
        if (source == null) {
            return result;
        }
        for (int index = 0; index < source.size(); index++) {
            Object value = source.elementAt(index);
            if (!(value instanceof String[])) {
                throw new IllegalArgumentException("Header must be a String[2]");
            }
            String[] pair = (String[]) value;
            if (pair.length != 2 || pair[0] == null || pair[1] == null) {
                throw new IllegalArgumentException("Header must contain name and value");
            }
            result.addElement(new String[]{pair[0], pair[1]});
        }
        return result;
    }

    private static byte[] copyBytes(byte[] source) {
        if (source == null || source.length == 0) {
            return new byte[0];
        }
        byte[] result = new byte[source.length];
        System.arraycopy(source, 0, result, 0, source.length);
        return result;
    }
}
