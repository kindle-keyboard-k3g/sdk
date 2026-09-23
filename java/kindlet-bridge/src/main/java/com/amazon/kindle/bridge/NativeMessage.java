package com.amazon.kindle.bridge;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Binary framed message structure for the Kindle Native IPC protocol.
 * Conforms to the wire format: Magic (4B), Version (1B), Type (1B), Flags (2B), Request ID (4B), Length (4B), Payload.
 */
public class NativeMessage {

    /** Wire format magic header ('KIND'). */
    public static final int MAGIC = 0x4B494E44; // 'KIND'
    /** Protocol format version. */
    public static final byte VERSION = 0x01;

    /** Ping health check message type. */
    public static final byte TYPE_PING = 0x01;
    /** Pong response message type. */
    public static final byte TYPE_PONG = 0x02;
    /** Whispernet proxy HTTP request type. */
    public static final byte TYPE_HTTP_REQUEST  = 0x06;
    /** Whispernet proxy HTTP response type. */
    public static final byte TYPE_HTTP_RESPONSE = 0x07;
    /** Command execution request type. */
    public static final byte TYPE_COMMAND = 0x10;
    /** Command response result type. */
    public static final byte TYPE_RESPONSE = 0x11;
    /** Asynchronous event notification type. */
    public static final byte TYPE_NOTIFICATION = 0x20;
    /** Process shutdown request type. */
    public static final byte TYPE_SHUTDOWN = (byte) 0xFF;

    private byte type;
    private short flags;
    private int requestId;
    private byte[] payload;

    /**
     * Constructs a new message frame.
     *
     * @param type message type code
     * @param requestId correlation request identifier
     * @param payload payload byte array
     */
    public NativeMessage(byte type, int requestId, byte[] payload) {
        this.type = type;
        this.flags = 0;
        this.requestId = requestId;
        this.payload = (payload != null) ? payload : new byte[0];
    }

    /**
     * Returns message type code.
     *
     * @return type byte
     */
    public byte getType() { return type; }

    /**
     * Returns protocol bitmask flags.
     *
     * @return flags short
     */
    public short getFlags() { return flags; }

    /**
     * Returns correlation request ID.
     *
     * @return request ID integer
     */
    public int getRequestId() { return requestId; }

    /**
     * Returns raw payload byte array.
     *
     * @return byte array
     */
    public byte[] getPayload() { return payload; }

    /**
     * Decodes payload bytes into a UTF-8 string.
     *
     * @return decoded string
     */
    public String getPayloadAsString() {
        return new String(payload);
    }

    /**
     * Serializes this frame and writes it to the output stream.
     *
     * @param out target stream
     * @throws IOException on write error
     */
    public void writeTo(OutputStream out) throws IOException {
        DataOutputStream dos = new DataOutputStream(out);
        dos.writeInt(MAGIC);
        dos.writeByte(VERSION);
        dos.writeByte(type);
        dos.writeShort(flags);
        dos.writeInt(requestId);
        dos.writeInt(payload.length);
        if (payload.length > 0) {
            dos.write(payload);
        }
        dos.flush();
    }

    /**
     * Deserializes a message frame from the input stream.
     *
     * @param in target stream
     * @return decoded NativeMessage instance
     * @throws IOException on invalid magic, version, or EOF
     */
    public static NativeMessage readFrom(InputStream in) throws IOException {
        DataInputStream dis = new DataInputStream(in);
        int magic = dis.readInt();
        if (magic != MAGIC) {
            throw new IOException("Invalid IPC magic header: " + Integer.toHexString(magic));
        }
        byte ver = dis.readByte();
        if (ver != VERSION) {
            throw new IOException("Unsupported IPC version: " + ver);
        }
        byte type = dis.readByte();
        short flags = dis.readShort();
        int reqId = dis.readInt();
        int len = dis.readInt();
        if (len < 0 || len > 1024 * 1024) {
            throw new IOException("Invalid payload length: " + len);
        }
        byte[] payload = new byte[len];
        if (len > 0) {
            dis.readFully(payload);
        }
        NativeMessage msg = new NativeMessage(type, reqId, payload);
        msg.flags = flags;
        return msg;
    }
}
