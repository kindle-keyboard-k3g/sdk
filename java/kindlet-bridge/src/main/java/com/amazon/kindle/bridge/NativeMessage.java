package com.amazon.kindle.bridge;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Message frame for the Kindle Native IPC protocol.
 */
public class NativeMessage {

    public static final int MAGIC = 0x4B494E44; // 'KIND'
    public static final byte VERSION = 0x01;

    public static final byte TYPE_PING = 0x01;
    public static final byte TYPE_PONG = 0x02;
    public static final byte TYPE_COMMAND = 0x10;
    public static final byte TYPE_RESPONSE = 0x11;
    public static final byte TYPE_NOTIFICATION = 0x20;
    public static final byte TYPE_SHUTDOWN = (byte) 0xFF;

    private byte type;
    private short flags;
    private int requestId;
    private byte[] payload;

    public NativeMessage(byte type, int requestId, byte[] payload) {
        this.type = type;
        this.flags = 0;
        this.requestId = requestId;
        this.payload = (payload != null) ? payload : new byte[0];
    }

    public byte getType() { return type; }
    public short getFlags() { return flags; }
    public int getRequestId() { return requestId; }
    public byte[] getPayload() { return payload; }

    public String getPayloadAsString() {
        return new String(payload);
    }

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
