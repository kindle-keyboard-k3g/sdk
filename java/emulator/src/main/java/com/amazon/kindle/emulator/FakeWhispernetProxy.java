package com.amazon.kindle.emulator;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * In-process fake HTTP proxy that emulates the Whispernet 3G proxy for desktop
 * development. Forwards requests to the real internet via java.net.URL, allowing
 * kindlets to call WhispernetHttpClient on a developer machine without a physical
 * device.
 *
 * Start with {@link #start()} before setting KINDLE_WHISPERNET_PROXY_HOST/PORT
 * environment properties; stop with {@link #stop()} when done.
 *
 * This class is only intended for development and testing — do not ship it in
 * production kindlet JARs.
 */
public class FakeWhispernetProxy {

    private ServerSocket serverSocket;
    private Thread acceptThread;
    private volatile boolean running = false;

    /**
     * Starts the fake proxy on an OS-assigned port.
     *
     * @throws IOException if the server socket cannot be bound
     */
    public void start() throws IOException {
        serverSocket = new ServerSocket(0);
        running = true;

        System.setProperty("KINDLE_WHISPERNET_PROXY_HOST", "127.0.0.1");
        System.setProperty("KINDLE_WHISPERNET_PROXY_PORT", String.valueOf(serverSocket.getLocalPort()));
        System.out.println("[FakeWhispernetProxy] listening on 127.0.0.1:" + serverSocket.getLocalPort());

        acceptThread = new Thread(new Runnable() {
            public void run() {
                while (running) {
                    try {
                        Socket client = serverSocket.accept();
                        Thread handler = new Thread(new ConnectionHandler(client));
                        handler.setDaemon(true);
                        handler.start();
                    } catch (IOException e) {
                        if (running) {
                            System.err.println("[FakeWhispernetProxy] accept error: " + e.getMessage());
                        }
                    }
                }
            }
        });
        acceptThread.setDaemon(true);
        acceptThread.start();
    }

    /**
     * Returns the port the fake proxy is listening on.
     *
     * @return TCP port number
     * @throws IllegalStateException if start() has not been called
     */
    public int getPort() {
        if (serverSocket == null) {
            throw new IllegalStateException("FakeWhispernetProxy not started");
        }
        return serverSocket.getLocalPort();
    }

    /**
     * Stops the fake proxy and releases its port.
     */
    public void stop() {
        running = false;
        try {
            if (serverSocket != null) {
                serverSocket.close();
            }
        } catch (IOException ignored) {}
        System.clearProperty("KINDLE_WHISPERNET_PROXY_HOST");
        System.clearProperty("KINDLE_WHISPERNET_PROXY_PORT");
    }

    // -------------------------------------------------------------------------
    // Per-connection handler: reads one request, forwards it, writes response
    // -------------------------------------------------------------------------

    private static class ConnectionHandler implements Runnable {
        private final Socket client;
        private static final int MAX_BODY = 1024 * 1024;

        ConnectionHandler(Socket client) {
            this.client = client;
        }

        public void run() {
            try {
                client.setSoTimeout(10000);
                InputStream in   = client.getInputStream();
                OutputStream out = client.getOutputStream();

                String requestLine = readLine(in);
                if (requestLine == null || requestLine.trim().length() == 0) {
                    return;
                }

                // Drain request headers; collect Content-Length
                int contentLength = 0;
                String headerLine;
                while ((headerLine = readLine(in)) != null && headerLine.length() > 0) {
                    if (headerLine.toLowerCase().startsWith("content-length:")) {
                        try {
                            contentLength = Integer.parseInt(headerLine.substring(15).trim());
                        } catch (NumberFormatException ignored) {}
                    }
                }

                // Drain any request body (for POST/PUT)
                if (contentLength > 0) {
                    byte[] bodyBuf = new byte[Math.min(contentLength, MAX_BODY)];
                    int remaining = bodyBuf.length;
                    int offset = 0;
                    while (remaining > 0) {
                        int read = in.read(bodyBuf, offset, remaining);
                        if (read < 0) break;
                        offset += read;
                        remaining -= read;
                    }
                }

                // Parse: "METHOD absolute-url HTTP/version"
                String[] parts = requestLine.split(" ", 3);
                if (parts.length < 2) {
                    writeError(out, 400, "Bad Request");
                    return;
                }
                String url = parts[1];
                if (url.startsWith("https://")) {
                    writeError(out, 501, "HTTPS not supported by FakeWhispernetProxy");
                    return;
                }

                // Forward via java.net.URL
                java.net.URL target;
                try {
                    target = new java.net.URL(url);
                } catch (java.net.MalformedURLException e) {
                    writeError(out, 400, "Malformed URL");
                    return;
                }

                java.net.URLConnection conn = target.openConnection();
                conn.setConnectTimeout(10000);
                conn.setReadTimeout(10000);
                conn.setDoInput(true);

                int responseCode = 200;
                String responseMessage = "OK";
                if (conn instanceof java.net.HttpURLConnection) {
                    java.net.HttpURLConnection http = (java.net.HttpURLConnection) conn;
                    http.setRequestMethod(parts[0]);
                    responseCode    = http.getResponseCode();
                    responseMessage = http.getResponseMessage();
                }

                byte[] body = readAll(conn.getInputStream());

                // Write minimal HTTP/1.0 response
                String responseHeader = "HTTP/1.0 " + responseCode + " " + responseMessage + "\r\n"
                                      + "Content-Length: " + body.length + "\r\n"
                                      + "\r\n";
                out.write(responseHeader.getBytes("US-ASCII"));
                out.write(body);
                out.flush();

            } catch (IOException e) {
                System.err.println("[FakeWhispernetProxy] handler error: " + e.getMessage());
            } finally {
                try { client.close(); } catch (IOException ignored) {}
            }
        }

        private static void writeError(OutputStream out, int code, String message) throws IOException {
            String resp = "HTTP/1.0 " + code + " " + message + "\r\nContent-Length: 0\r\n\r\n";
            out.write(resp.getBytes("US-ASCII"));
            out.flush();
        }

        private static String readLine(InputStream in) throws IOException {
            StringBuffer sb = new StringBuffer();
            int b;
            while ((b = in.read()) != -1) {
                if (b == '\n') {
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

        private static byte[] readAll(InputStream in) throws IOException {
            java.io.ByteArrayOutputStream baos = new java.io.ByteArrayOutputStream();
            byte[] buf = new byte[4096];
            int read;
            while ((read = in.read(buf)) != -1) {
                baos.write(buf, 0, read);
                if (baos.size() > MAX_BODY) break;
            }
            return baos.toByteArray();
        }
    }
}
