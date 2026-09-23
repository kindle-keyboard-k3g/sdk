package com.amazon.kindle.bridge.network;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.Vector;

/**
 * Encodes and decodes HttpRequest / HttpResponse objects to/from the
 * KIND IPC binary payload format. Compatible with Java 1.4 / CDC 1.1.
 */
public final class HttpIpcCodec {

    private static final byte SCHEMA_VERSION = 1;
    private static final int MAX_PAYLOAD = 1024 * 1024;

    private HttpIpcCodec() {
    }

    public static byte[] encodeRequest(HttpRequest request) {
        byte[] methodBytes = toUtf8(request.getMethod());
        byte[] urlBytes = toUtf8(request.getUrl());
        if (methodBytes.length > 255) {
            throw new IllegalArgumentException("HTTP method exceeds 255 bytes");
        }
        if (urlBytes.length > 65535) {
            throw new IllegalArgumentException("HTTP url exceeds 65535 bytes");
        }

        Vector headers = request.getHeaders();
        byte[] body = request.getBody();
        if (headers.size() > 65535) {
            throw new IllegalArgumentException("Header count exceeds 65535");
        }
        if (body.length > MAX_PAYLOAD) {
            throw new IllegalArgumentException("HTTP body exceeds 1 MiB");
        }

        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            DataOutputStream output = new DataOutputStream(bytes);
            output.writeByte(SCHEMA_VERSION);
            output.writeByte(methodBytes.length);
            output.writeShort(urlBytes.length);
            output.writeShort(headers.size());
            output.writeInt(body.length);
            output.write(methodBytes);
            output.write(urlBytes);
            if (!writeHeaders(output, headers, true)) {
                return null;
            }
            output.write(body);
            output.flush();

            byte[] result = bytes.toByteArray();
            return result.length <= MAX_PAYLOAD ? result : null;
        } catch (IOException error) {
            return null;
        }
    }

    public static byte[] encodeResponse(HttpResponse response) {
        String reason = response.getReason();
        if (response.getStatusCode() == 0 && reason.length() == 0) {
            reason = response.getError();
        }
        byte[] reasonBytes = toUtf8(reason);
        if (reasonBytes.length > 65535) {
            return null;
        }

        Vector headers = response.getHeaders();
        byte[] body = response.getBody();
        if (headers.size() > 65535 || body.length > MAX_PAYLOAD) {
            return null;
        }

        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            DataOutputStream output = new DataOutputStream(bytes);
            output.writeByte(SCHEMA_VERSION);
            output.writeShort(response.getStatusCode());
            output.writeShort(reasonBytes.length);
            output.writeShort(headers.size());
            output.writeInt(body.length);
            output.write(reasonBytes);
            if (!writeHeaders(output, headers, false)) {
                return null;
            }
            output.write(body);
            output.flush();

            byte[] result = bytes.toByteArray();
            return result.length <= MAX_PAYLOAD ? result : null;
        } catch (IOException error) {
            return null;
        } catch (IllegalArgumentException error) {
            return null;
        }
    }

    private static boolean writeHeaders(DataOutputStream output, Vector headers,
                                        boolean throwOnInvalid) throws IOException {
        for (int index = 0; index < headers.size(); index++) {
            Object rawPair = headers.elementAt(index);
            if (!(rawPair instanceof String[])) {
                if (throwOnInvalid) {
                    throw new IllegalArgumentException("Header must be a String[2]");
                }
                return false;
            }
            String[] pair = (String[]) rawPair;
            if (pair.length != 2 || pair[0] == null || pair[1] == null) {
                if (throwOnInvalid) {
                    throw new IllegalArgumentException("Header must contain name and value");
                }
                return false;
            }
            byte[] name = toUtf8(pair[0]);
            byte[] value = toUtf8(pair[1]);
            if (name.length > 65535 || value.length > 65535) {
                if (throwOnInvalid) {
                    throw new IllegalArgumentException("Header name or value exceeds 65535 bytes");
                }
                return false;
            }
            output.writeShort(name.length);
            output.writeShort(value.length);
            output.write(name);
            output.write(value);
        }
        return true;
    }

    public static HttpRequest decodeRequest(byte[] payload) {
        if (payload == null || payload.length < 10 || payload.length > MAX_PAYLOAD) {
            return null;
        }
        int pos = 0;
        if ((payload[pos++] & 0xFF) != (SCHEMA_VERSION & 0xFF)) {
            return null;
        }
        int methodLength = payload[pos++] & 0xFF;
        int urlLength = readU16(payload, pos);
        pos += 2;
        int headerCount = readU16(payload, pos);
        pos += 2;
        long bodyLength = readU32(payload, pos);
        pos += 4;
        if (methodLength == 0 || urlLength == 0 || bodyLength > MAX_PAYLOAD) {
            return null;
        }

        String method = readUtf8(payload, pos, methodLength);
        if (method == null) {
            return null;
        }
        pos += methodLength;
        String url = readUtf8(payload, pos, urlLength);
        if (url == null) {
            return null;
        }
        pos += urlLength;

        int[] headerPosition = new int[]{pos};
        Vector headers = readHeaders(payload, headerCount, headerPosition);
        if (headers == null) {
            return null;
        }
        pos = headerPosition[0];

        int bodySize = (int) bodyLength;
        if (!hasRoom(payload, pos, bodySize)) {
            return null;
        }
        byte[] body = new byte[bodySize];
        System.arraycopy(payload, pos, body, 0, bodySize);
        pos += bodySize;
        if (pos != payload.length) {
            return null;
        }
        return new HttpRequest(method, url, headers, body);
    }

    public static HttpResponse decodeResponse(byte[] payload) {
        if (payload == null || payload.length < 11 || payload.length > MAX_PAYLOAD) {
            return null;
        }
        int pos = 0;
        if ((payload[pos++] & 0xFF) != (SCHEMA_VERSION & 0xFF)) {
            return null;
        }
        int statusCode = readU16(payload, pos);
        pos += 2;
        int reasonLength = readU16(payload, pos);
        pos += 2;
        int headerCount = readU16(payload, pos);
        pos += 2;
        long bodyLength = readU32(payload, pos);
        pos += 4;
        if (bodyLength > MAX_PAYLOAD) {
            return null;
        }

        String reason = readUtf8(payload, pos, reasonLength);
        if (reason == null) {
            return null;
        }
        pos += reasonLength;
        int[] headerPosition = new int[]{pos};
        Vector headers = readHeaders(payload, headerCount, headerPosition);
        if (headers == null) {
            return null;
        }
        pos = headerPosition[0];

        int bodySize = (int) bodyLength;
        if (!hasRoom(payload, pos, bodySize)) {
            return null;
        }
        byte[] body = new byte[bodySize];
        System.arraycopy(payload, pos, body, 0, bodySize);
        pos += bodySize;
        if (pos != payload.length) {
            return null;
        }
        return new HttpResponse(statusCode, reason, headers, body);
    }

    private static Vector readHeaders(byte[] payload, int headerCount, int[] position) {
        Vector headers = new Vector();
        int pos = position[0];
        for (int index = 0; index < headerCount; index++) {
            if (!hasRoom(payload, pos, 4)) {
                return null;
            }
            int nameLength = readU16(payload, pos);
            pos += 2;
            int valueLength = readU16(payload, pos);
            pos += 2;
            String name = readUtf8(payload, pos, nameLength);
            if (name == null) {
                return null;
            }
            pos += nameLength;
            String value = readUtf8(payload, pos, valueLength);
            if (value == null) {
                return null;
            }
            pos += valueLength;
            headers.addElement(new String[]{name, value});
        }
        position[0] = pos;
        return headers;
    }

    private static boolean hasRoom(byte[] buffer, int position, int length) {
        return position >= 0 && length >= 0 && position <= buffer.length &&
               length <= buffer.length - position;
    }

    private static String readUtf8(byte[] buffer, int position, int length) {
        if (!hasRoom(buffer, position, length)) {
            return null;
        }
        try {
            return new String(buffer, position, length, "UTF-8");
        } catch (UnsupportedEncodingException error) {
            return null;
        }
    }

    private static int readU16(byte[] buffer, int offset) {
        return ((buffer[offset] & 0xFF) << 8) | (buffer[offset + 1] & 0xFF);
    }

    private static long readU32(byte[] buffer, int offset) {
        return ((long) (buffer[offset] & 0xFF) << 24)
             | ((long) (buffer[offset + 1] & 0xFF) << 16)
             | ((long) (buffer[offset + 2] & 0xFF) << 8)
             | ((long) (buffer[offset + 3] & 0xFF));
    }

    private static byte[] toUtf8(String value) {
        if (value == null || value.length() == 0) {
            return new byte[0];
        }
        try {
            return value.getBytes("UTF-8");
        } catch (UnsupportedEncodingException error) {
            return new byte[0];
        }
    }
}
