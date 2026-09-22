package com.amazon.kindle.bridge.network;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Vector;

/**
 * Encodes and decodes HttpRequest / HttpResponse objects to/from the
 * KIND IPC binary payload format.
 *
 * Wire format is big-endian; all multi-byte fields are unsigned.
 *
 * Request payload:
 *   u8  schema_version  (always 1)
 *   u8  method_length
 *   u16 url_length
 *   u16 header_count
 *   u32 body_length
 *   bytes method, url, [u16 name_len, u16 val_len, bytes name, bytes val] * N, body
 *
 * Response payload:
 *   u8  schema_version  (always 1)
 *   u16 status_code     (0 = transport error)
 *   u16 reason_length
 *   u16 header_count
 *   u32 body_length
 *   bytes reason, [u16 name_len, u16 val_len, bytes name, bytes val] * N, body
 *
 * Compatible with Java 1.4 / CDC 1.1.
 */
public final class HttpIpcCodec {

    private static final byte SCHEMA_VERSION = 1;
    private static final int  MAX_PAYLOAD    = 1024 * 1024;

    private HttpIpcCodec() {}

    // -------------------------------------------------------------------------
    // Encoding
    // -------------------------------------------------------------------------

    /**
     * Encodes an HttpRequest into a KIND IPC payload byte array.
     *
     * @param req request to encode
     * @return encoded bytes, or null if the request exceeds protocol limits
     * @throws IllegalArgumentException if method exceeds 255 bytes or url exceeds 65535 bytes
     */
    public static byte[] encodeRequest(HttpRequest req) {
        byte[] methodBytes = toUtf8(req.getMethod());
        byte[] urlBytes    = toUtf8(req.getUrl());
        if (methodBytes.length > 255) {
            throw new IllegalArgumentException("HTTP method exceeds 255 bytes");
        }
        if (urlBytes.length > 65535) {
            throw new IllegalArgumentException("HTTP url exceeds 65535 bytes");
        }
        Vector headers = req.getHeaders();
        if (headers.size() > 65535) {
            throw new IllegalArgumentException("Header count exceeds 65535");
        }

        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);

            // Fixed header (10 bytes)
            dos.writeByte(SCHEMA_VERSION);
            dos.writeByte(methodBytes.length);
            dos.writeShort(urlBytes.length);
            dos.writeShort(headers.size());
            dos.writeInt(req.getBody().length);

            // Variable fields
            dos.write(methodBytes);
            dos.write(urlBytes);
            for (int i = 0; i < headers.size(); i++) {
                String[] pair = (String[]) headers.elementAt(i);
                byte[] nameBytes  = toUtf8(pair[0]);
                byte[] valueBytes = toUtf8(pair[1]);
                dos.writeShort(nameBytes.length);
                dos.writeShort(valueBytes.length);
                dos.write(nameBytes);
                dos.write(valueBytes);
            }
            if (req.getBody().length > 0) {
                dos.write(req.getBody());
            }
            dos.flush();

            byte[] result = baos.toByteArray();
            if (result.length > MAX_PAYLOAD) {
                return null;
            }
            return result;
        } catch (IOException e) {
            return null;
        }
    }

    /**
     * Encodes an HttpResponse into a KIND IPC payload byte array.
     *
     * @param resp response to encode
     * @return encoded bytes, or null if the response exceeds protocol limits
     */
    public static byte[] encodeResponse(HttpResponse resp) {
        byte[] reasonBytes = toUtf8(resp.getReason());
        if (reasonBytes.length > 65535) {
            return null;
        }
        Vector headers = resp.getHeaders();
        if (headers.size() > 65535) {
            return null;
        }

        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);

            // Fixed header (11 bytes)
            dos.writeByte(SCHEMA_VERSION);
            dos.writeShort(resp.getStatusCode());
            dos.writeShort(reasonBytes.length);
            dos.writeShort(headers.size());
            dos.writeInt(resp.getBody().length);

            // Variable fields
            dos.write(reasonBytes);
            for (int i = 0; i < headers.size(); i++) {
                String[] pair = (String[]) headers.elementAt(i);
                byte[] nameBytes  = toUtf8(pair[0]);
                byte[] valueBytes = toUtf8(pair[1]);
                dos.writeShort(nameBytes.length);
                dos.writeShort(valueBytes.length);
                dos.write(nameBytes);
                dos.write(valueBytes);
            }
            if (resp.getBody().length > 0) {
                dos.write(resp.getBody());
            }
            dos.flush();

            byte[] result = baos.toByteArray();
            if (result.length > MAX_PAYLOAD) {
                return null;
            }
            return result;
        } catch (IOException e) {
            return null;
        }
    }

    // -------------------------------------------------------------------------
    // Decoding
    // -------------------------------------------------------------------------

    /**
     * Decodes a KIND IPC payload into an HttpRequest.
     *
     * @param payload raw payload bytes
     * @return decoded HttpRequest, or null if the payload is malformed
     */
    public static HttpRequest decodeRequest(byte[] payload) {
        if (payload == null || payload.length < 10) {
            return null;
        }
        int pos = 0;
        int schema = payload[pos++] & 0xFF;
        if (schema != (SCHEMA_VERSION & 0xFF)) {
            return null;
        }
        int methodLen   = payload[pos++] & 0xFF;
        int urlLen      = readU16(payload, pos); pos += 2;
        int headerCount = readU16(payload, pos); pos += 2;
        long bodyLen    = readU32(payload, pos); pos += 4;

        if (bodyLen < 0 || bodyLen > MAX_PAYLOAD) {
            return null;
        }

        if (pos + methodLen > payload.length) return null;
        String method = new String(payload, pos, methodLen);
        pos += methodLen;

        if (pos + urlLen > payload.length) return null;
        String url = new String(payload, pos, urlLen);
        pos += urlLen;

        Vector headers = new Vector();
        for (int i = 0; i < headerCount; i++) {
            if (pos + 4 > payload.length) return null;
            int nameLen  = readU16(payload, pos); pos += 2;
            int valueLen = readU16(payload, pos); pos += 2;
            if (pos + nameLen > payload.length) return null;
            String name = new String(payload, pos, nameLen);
            pos += nameLen;
            if (pos + valueLen > payload.length) return null;
            String value = new String(payload, pos, valueLen);
            pos += valueLen;
            headers.addElement(new String[]{name, value});
        }

        int blen = (int) bodyLen;
        if (pos + blen > payload.length) return null;
        byte[] body = new byte[blen];
        System.arraycopy(payload, pos, body, 0, blen);
        pos += blen;

        if (pos != payload.length) return null;

        return new HttpRequest(method, url, headers, body);
    }

    /**
     * Decodes a KIND IPC payload into an HttpResponse.
     *
     * @param payload raw payload bytes
     * @return decoded HttpResponse, or null if the payload is malformed
     */
    public static HttpResponse decodeResponse(byte[] payload) {
        if (payload == null || payload.length < 11) {
            return null;
        }
        int pos = 0;
        int schema = payload[pos++] & 0xFF;
        if (schema != (SCHEMA_VERSION & 0xFF)) {
            return null;
        }
        int statusCode  = readU16(payload, pos); pos += 2;
        int reasonLen   = readU16(payload, pos); pos += 2;
        int headerCount = readU16(payload, pos); pos += 2;
        long bodyLen    = readU32(payload, pos); pos += 4;

        if (bodyLen < 0 || bodyLen > MAX_PAYLOAD) {
            return null;
        }

        if (pos + reasonLen > payload.length) return null;
        String reason = new String(payload, pos, reasonLen);
        pos += reasonLen;

        Vector headers = new Vector();
        for (int i = 0; i < headerCount; i++) {
            if (pos + 4 > payload.length) return null;
            int nameLen  = readU16(payload, pos); pos += 2;
            int valueLen = readU16(payload, pos); pos += 2;
            if (pos + nameLen > payload.length) return null;
            String name = new String(payload, pos, nameLen);
            pos += nameLen;
            if (pos + valueLen > payload.length) return null;
            String value = new String(payload, pos, valueLen);
            pos += valueLen;
            headers.addElement(new String[]{name, value});
        }

        int blen = (int) bodyLen;
        if (pos + blen > payload.length) return null;
        byte[] body = new byte[blen];
        System.arraycopy(payload, pos, body, 0, blen);
        pos += blen;

        if (pos != payload.length) return null;

        return new HttpResponse(statusCode, reason, headers, body);
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    private static int readU16(byte[] buf, int off) {
        return ((buf[off] & 0xFF) << 8) | (buf[off + 1] & 0xFF);
    }

    private static long readU32(byte[] buf, int off) {
        return ((long) (buf[off]     & 0xFF) << 24)
             | ((long) (buf[off + 1] & 0xFF) << 16)
             | ((long) (buf[off + 2] & 0xFF) <<  8)
             | ((long) (buf[off + 3] & 0xFF));
    }

    private static byte[] toUtf8(String s) {
        if (s == null || s.length() == 0) {
            return new byte[0];
        }
        try {
            return s.getBytes("UTF-8");
        } catch (java.io.UnsupportedEncodingException e) {
            return s.getBytes();
        }
    }
}
