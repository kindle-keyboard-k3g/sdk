package com.amazon.kindle.bridge.network;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

/**
 * Raw TCP socket client that routes connections through the Whispernet 3G proxy
 * via the HTTP CONNECT tunnel method.
 * Compatible with Java 1.4 / CDC 1.1.
 */
public class WhispernetSocketClient {

    private static final int SOCKET_TIMEOUT_MS = 30000; // 30 s

    private final WhispernetProxy proxy;

    /**
     * Constructs a socket client bound to the given proxy.
     *
     * @param proxy configured Whispernet proxy; must not be null
     */
    public WhispernetSocketClient(WhispernetProxy proxy) {
        if (proxy == null) {
            throw new IllegalArgumentException("WhispernetProxy must not be null");
        }
        this.proxy = proxy;
    }

    /**
     * Opens a TCP tunnel to targetHost:targetPort through the Whispernet proxy.
     * The caller owns the returned socket and must close it when done.
     *
     * @param targetHost remote host to reach through the proxy
     * @param targetPort remote port to reach through the proxy
     * @return connected Socket tunneled via CONNECT
     * @throws IOException if the proxy refuses the connection or the tunnel fails
     */
    public Socket connect(String targetHost, int targetPort) throws IOException {
        if (targetHost == null || targetHost.trim().length() == 0) {
            throw new IllegalArgumentException("targetHost must not be empty");
        }
        if (targetHost.indexOf('\r') >= 0 || targetHost.indexOf('\n') >= 0) {
            throw new IOException("targetHost contains CRLF characters");
        }
        if (targetPort < 1 || targetPort > 65535) {
            throw new IllegalArgumentException("targetPort out of range: " + targetPort);
        }

        Socket socket = new Socket(proxy.getHost(), proxy.getPort());
        socket.setSoTimeout(SOCKET_TIMEOUT_MS);
        boolean success = false;
        try {
            establishTunnel(socket, targetHost, targetPort);
            success = true;
            return socket;
        } finally {
            if (!success) {
                try { socket.close(); } catch (IOException ignored) {}
            }
        }
    }

    private void establishTunnel(Socket socket, String targetHost, int targetPort)
            throws IOException {
        OutputStream out = socket.getOutputStream();
        String connectLine = "CONNECT " + targetHost + ":" + targetPort + " HTTP/1.0\r\n"
                           + "Host: " + targetHost + ":" + targetPort + "\r\n\r\n";
        out.write(connectLine.getBytes("US-ASCII"));
        out.flush();

        // Read proxy response status line + headers
        InputStream in = socket.getInputStream();
        String statusLine = readLine(in);
        if (statusLine == null || !statusLine.startsWith("HTTP/")) {
            throw new IOException("Proxy CONNECT returned invalid response: " + statusLine);
        }
        int firstSpace = statusLine.indexOf(' ');
        if (firstSpace < 0) {
            throw new IOException("Malformed CONNECT response: " + statusLine);
        }
        String codeStr = statusLine.substring(firstSpace + 1).trim();
        int secondSpace = codeStr.indexOf(' ');
        int statusCode = Integer.parseInt(
            secondSpace < 0 ? codeStr : codeStr.substring(0, secondSpace));
        if (statusCode != 200) {
            throw new IOException("Proxy rejected CONNECT with status " + statusCode);
        }
        // Drain remaining headers
        String line;
        do {
            line = readLine(in);
        } while (line != null && line.length() > 0);
    }

    /**
     * Reads one CR-LF terminated line from the stream.
     * Returns the line without the terminator, or null on EOF.
     */
    private static String readLine(InputStream in) throws IOException {
        StringBuffer sb = new StringBuffer();
        int b;
        while ((b = in.read()) != -1) {
            if (b == '\n') {
                // Remove trailing CR if present
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
}
