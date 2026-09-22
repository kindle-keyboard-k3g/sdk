package com.amazon.kindle.bridge.network;

import java.util.Vector;

/**
 * Immutable HTTP response value type returned by Whispernet proxy calls.
 * Compatible with Java 1.4 / CDC 1.1 (no generics, no enhanced for).
 * A status_code of 0 indicates a transport-layer error; check getError().
 */
public final class HttpResponse {

    private final int statusCode;
    private final String reason;
    private final String error;
    private final Vector headers;  // Vector of String[2] {name, value}
    private final byte[] body;

    /**
     * Constructs a successful HTTP response.
     *
     * @param statusCode HTTP status code (e.g., 200)
     * @param reason     reason phrase (e.g., "OK")
     * @param headers    ordered header pairs; each element must be String[2]
     * @param body       response body bytes; null treated as empty
     */
    public HttpResponse(int statusCode, String reason, Vector headers, byte[] body) {
        this.statusCode = statusCode;
        this.reason     = (reason != null) ? reason : "";
        this.error      = "";
        this.headers    = (headers != null) ? headers : new Vector();
        this.body       = (body != null) ? body : new byte[0];
    }

    /**
     * Constructs a transport-error response (statusCode == 0).
     *
     * @param error human-readable error description
     */
    public HttpResponse(String error) {
        this.statusCode = 0;
        this.reason     = "";
        this.error      = (error != null) ? error : "unknown error";
        this.headers    = new Vector();
        this.body       = new byte[0];
    }

    /** Returns the HTTP status code, or 0 on transport error. */
    public int getStatusCode() { return statusCode; }

    /** Returns the HTTP reason phrase, or empty string on transport error. */
    public String getReason() { return reason; }

    /**
     * Returns the transport error message.
     * Non-empty only when statusCode == 0.
     */
    public String getError() { return error; }

    /**
     * Returns the ordered header list.
     * Each element is a String[2] array: {name, value}.
     */
    public Vector getHeaders() { return headers; }

    /** Returns the response body bytes (never null; may be empty). */
    public byte[] getBody() { return body; }

    /**
     * Returns true if this response represents a successful HTTP exchange
     * (statusCode in 200-299 range).
     */
    public boolean isOk() {
        return statusCode >= 200 && statusCode < 300;
    }

    /**
     * Returns true if this response represents a transport-layer error
     * (statusCode == 0 and error is non-empty).
     */
    public boolean isTransportError() {
        return statusCode == 0;
    }

    /**
     * Finds the first header value matching the given name (case-insensitive).
     *
     * @param name header name to search
     * @return header value, or null if not found
     */
    public String getHeader(String name) {
        if (name == null) {
            return null;
        }
        for (int i = 0; i < headers.size(); i++) {
            String[] pair = (String[]) headers.elementAt(i);
            if (name.equalsIgnoreCase(pair[0])) {
                return pair[1];
            }
        }
        return null;
    }
}
