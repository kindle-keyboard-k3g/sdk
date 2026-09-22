package com.amazon.kindle.bridge.network;

import java.util.Vector;

/**
 * Immutable HTTP request value type for Whispernet proxy calls.
 * Compatible with Java 1.4 / CDC 1.1 (no generics, no enhanced for).
 */
public final class HttpRequest {

    private final String method;
    private final String url;
    private final Vector headers;  // Vector of String[2] {name, value}
    private final byte[] body;

    /**
     * Constructs an HTTP request.
     *
     * @param method HTTP method (GET, POST, etc.); must not be null/empty
     * @param url    absolute URL; must not be null/empty
     * @throws IllegalArgumentException if method or url is null/empty
     */
    public HttpRequest(String method, String url) {
        this(method, url, new Vector(), new byte[0]);
    }

    /**
     * Constructs an HTTP request with headers and body.
     *
     * @param method  HTTP method
     * @param url     absolute URL
     * @param headers ordered header pairs; each element must be String[2]
     * @param body    request body bytes; null treated as empty
     */
    public HttpRequest(String method, String url, Vector headers, byte[] body) {
        if (method == null || method.trim().length() == 0) {
            throw new IllegalArgumentException("HTTP method must not be empty");
        }
        if (url == null || url.trim().length() == 0) {
            throw new IllegalArgumentException("HTTP url must not be empty");
        }
        this.method  = method;
        this.url     = url;
        this.headers = (headers != null) ? headers : new Vector();
        this.body    = (body != null) ? body : new byte[0];
    }

    /** Returns the HTTP method string. */
    public String getMethod() { return method; }

    /** Returns the absolute URL string. */
    public String getUrl() { return url; }

    /**
     * Returns the ordered header list.
     * Each element is a String[2] array: {name, value}.
     */
    public Vector getHeaders() { return headers; }

    /** Returns the request body bytes (never null; may be empty). */
    public byte[] getBody() { return body; }

    /**
     * Returns a new HttpRequest with one extra header appended.
     *
     * @param name  header name
     * @param value header value
     * @return new HttpRequest with the added header
     */
    public HttpRequest withHeader(String name, String value) {
        Vector newHeaders = new Vector(headers);
        newHeaders.addElement(new String[]{name, value});
        return new HttpRequest(method, url, newHeaders, body);
    }

    /**
     * Returns a new HttpRequest with the given body.
     *
     * @param newBody replacement body bytes
     * @return new HttpRequest with the new body
     */
    public HttpRequest withBody(byte[] newBody) {
        return new HttpRequest(method, url, headers, newBody);
    }
}
