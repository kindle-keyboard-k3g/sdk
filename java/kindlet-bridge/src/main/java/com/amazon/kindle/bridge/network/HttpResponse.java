package com.amazon.kindle.bridge.network;

import java.util.Vector;

/**
 * Immutable HTTP response value type returned by Whispernet proxy calls.
 * Compatible with Java 1.4 / CDC 1.1.
 */
public final class HttpResponse {

    private final int statusCode;
    private final String reason;
    private final String error;
    private final Vector headers;
    private final byte[] body;

    public HttpResponse(int statusCode, String reason, Vector headers, byte[] body) {
        this.statusCode = statusCode;
        this.reason = reason != null ? reason : "";
        this.error = "";
        this.headers = copyHeaders(headers);
        this.body = copyBytes(body);
    }

    public HttpResponse(String error) {
        this.statusCode = 0;
        this.reason = "";
        this.error = error != null ? error : "unknown error";
        this.headers = new Vector();
        this.body = new byte[0];
    }

    public int getStatusCode() {
        return statusCode;
    }

    public String getReason() {
        return reason;
    }

    public String getError() {
        return error;
    }

    /** Returns a defensive copy of the ordered header list. */
    public Vector getHeaders() {
        return copyHeaders(headers);
    }

    /** Returns a defensive copy of the response body. */
    public byte[] getBody() {
        return copyBytes(body);
    }

    public boolean isOk() {
        return statusCode >= 200 && statusCode < 300;
    }

    public boolean isTransportError() {
        return statusCode == 0;
    }

    public String getHeader(String name) {
        if (name == null) {
            return null;
        }
        for (int index = 0; index < headers.size(); index++) {
            String[] pair = (String[]) headers.elementAt(index);
            if (name.equalsIgnoreCase(pair[0])) {
                return pair[1];
            }
        }
        return null;
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
