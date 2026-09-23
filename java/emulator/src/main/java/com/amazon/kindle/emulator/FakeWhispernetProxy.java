package com.amazon.kindle.emulator;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Vector;

/**
 * Deterministic in-process Whispernet proxy for emulator development.
 * It serves only canned local routes and never forwards to the Internet.
 */
public class FakeWhispernetProxy {

    public static final String HOST_PROPERTY = "kindle.whispernet.proxy.host";
    public static final String PORT_PROPERTY = "kindle.whispernet.proxy.port";

    private static final int MAX_REQUEST_LINE = 8192;
    private static final int MAX_HEADER_LINE = 8192;
    private static final int MAX_HEADER_COUNT = 100;
    private static final int MAX_BODY = 1024 * 1024;

    private ServerSocket serverSocket;
    private Thread acceptThread;
    private volatile boolean running;
    private boolean propertiesInstalled;
    private String previousHost;
    private String previousPort;

    public void start() throws IOException {
        if (running) {
            throw new IllegalStateException("FakeWhispernetProxy is already running");
        }
        serverSocket = new ServerSocket(0);
        running = true;
        acceptThread = new Thread(new Runnable() {
            public void run() {
                acceptConnections();
            }
        });
        acceptThread.setDaemon(true);
        acceptThread.start();
    }

    public String getHost() {
        return "127.0.0.1";
    }

    public int getPort() {
        if (serverSocket == null) {
            throw new IllegalStateException("FakeWhispernetProxy not started");
        }
        return serverSocket.getLocalPort();
    }

    /** Installs this proxy's system properties while saving prior values. */
    public synchronized void installSystemProperties() {
        if (!propertiesInstalled) {
            previousHost = System.getProperty(HOST_PROPERTY);
            previousPort = System.getProperty(PORT_PROPERTY);
            propertiesInstalled = true;
        }
        System.setProperty(HOST_PROPERTY, getHost());
        System.setProperty(PORT_PROPERTY, String.valueOf(getPort()));
    }

    /** Restores the exact system property state present before installation. */
    public synchronized void restoreSystemProperties() {
        if (!propertiesInstalled) {
            return;
        }
        restoreProperty(HOST_PROPERTY, previousHost);
        restoreProperty(PORT_PROPERTY, previousPort);
        previousHost = null;
        previousPort = null;
        propertiesInstalled = false;
    }

    public void stop() {
        running = false;
        try {
            if (serverSocket != null) {
                serverSocket.close();
            }
        } catch (IOException ignored) {
        }
        if (acceptThread != null) {
            try {
                acceptThread.join(1000);
            } catch (InterruptedException error) {
                Thread.currentThread().interrupt();
            }
        }
        serverSocket = null;
        acceptThread = null;
    }

    private static void restoreProperty(String key, String value) {
        if (value == null) {
            System.clearProperty(key);
        } else {
            System.setProperty(key, value);
        }
    }

    private void acceptConnections() {
        while (running) {
            try {
                Socket client = serverSocket.accept();
                Thread handler = new Thread(new ConnectionHandler(client));
                handler.setDaemon(true);
                handler.start();
            } catch (IOException error) {
                if (running) {
                    System.err.println("[FakeWhispernetProxy] accept error: " + error.getMessage());
                }
            }
        }
    }

    private static final class ConnectionHandler implements Runnable {
        private final Socket client;

        ConnectionHandler(Socket client) {
            this.client = client;
        }

        public void run() {
            try {
                client.setSoTimeout(10000);
                InputStream input = client.getInputStream();
                OutputStream output = client.getOutputStream();
                String requestLine = readLine(input, MAX_REQUEST_LINE);
                if (requestLine == null) {
                    return;
                }

                Vector headers = new Vector();
                int contentLength = readHeaders(input, headers);
                byte[] body = readBody(input, contentLength);
                String[] requestParts = splitRequestLine(requestLine);
                if (requestParts == null) {
                    writeNotFound(output);
                    return;
                }

                if ("CONNECT".equalsIgnoreCase(requestParts[0])) {
                    writeConnectEstablished(output);
                    echoTunnel(input, output);
                    return;
                }

                String path = routePath(requestParts[1]);
                if ("GET".equalsIgnoreCase(requestParts[0]) && "/health".equals(path)) {
                    writeResponse(output, "application/json", "{\"status\":\"ok\"}".getBytes("UTF-8"));
                } else if ("GET".equalsIgnoreCase(requestParts[0]) && "/echo".equals(path)) {
                    writeResponse(output, "application/json", headersJson(headers).getBytes("UTF-8"));
                } else if ("POST".equalsIgnoreCase(requestParts[0]) && "/post".equals(path)) {
                    writeResponse(output, "application/octet-stream", body);
                } else {
                    writeNotFound(output);
                }
            } catch (IOException error) {
                System.err.println("[FakeWhispernetProxy] handler error: " + error.getMessage());
            } finally {
                try {
                    client.close();
                } catch (IOException ignored) {
                }
            }
        }

        private static int readHeaders(InputStream input, Vector headers) throws IOException {
            int contentLength = 0;
            int count = 0;
            while (true) {
                String line = readLine(input, MAX_HEADER_LINE);
                if (line == null) {
                    throw new IOException("Unexpected EOF while reading headers");
                }
                if (line.length() == 0) {
                    return contentLength;
                }
                count++;
                if (count > MAX_HEADER_COUNT) {
                    throw new IOException("Too many request headers");
                }
                int colon = line.indexOf(':');
                if (colon <= 0) {
                    continue;
                }
                String name = line.substring(0, colon).trim();
                String value = line.substring(colon + 1).trim();
                headers.addElement(new String[]{name, value});
                if ("content-length".equalsIgnoreCase(name)) {
                    try {
                        contentLength = Integer.parseInt(value);
                    } catch (NumberFormatException error) {
                        throw new IOException("Invalid Content-Length");
                    }
                    if (contentLength < 0 || contentLength > MAX_BODY) {
                        throw new IOException("Request body exceeds 1 MiB limit");
                    }
                }
            }
        }

        private static byte[] readBody(InputStream input, int length) throws IOException {
            byte[] body = new byte[length];
            int offset = 0;
            while (offset < length) {
                int count = input.read(body, offset, length - offset);
                if (count < 0) {
                    throw new IOException("Unexpected EOF while reading request body");
                }
                offset += count;
            }
            return body;
        }

        private static String[] splitRequestLine(String line) {
            int firstSpace = line.indexOf(' ');
            if (firstSpace <= 0) {
                return null;
            }
            int secondSpace = line.indexOf(' ', firstSpace + 1);
            if (secondSpace <= firstSpace + 1) {
                return null;
            }
            return new String[]{line.substring(0, firstSpace),
                                line.substring(firstSpace + 1, secondSpace)};
        }

        private static String routePath(String target) {
            int schemeEnd = target.indexOf("://");
            if (schemeEnd >= 0) {
                int pathStart = target.indexOf('/', schemeEnd + 3);
                return pathStart >= 0 ? stripQuery(target.substring(pathStart)) : "/";
            }
            return stripQuery(target);
        }

        private static String stripQuery(String path) {
            int query = path.indexOf('?');
            return query >= 0 ? path.substring(0, query) : path;
        }

        private static String headersJson(Vector headers) {
            StringBuffer json = new StringBuffer("{");
            for (int index = 0; index < headers.size(); index++) {
                if (index > 0) {
                    json.append(',');
                }
                String[] pair = (String[]) headers.elementAt(index);
                json.append('"').append(jsonEscape(pair[0])).append("\":\"");
                json.append(jsonEscape(pair[1])).append('"');
            }
            json.append('}');
            return json.toString();
        }

        private static String jsonEscape(String value) {
            StringBuffer escaped = new StringBuffer();
            for (int index = 0; index < value.length(); index++) {
                char character = value.charAt(index);
                if (character == '\\' || character == '"') {
                    escaped.append('\\');
                }
                if (character == '\r') {
                    escaped.append("\\r");
                } else if (character == '\n') {
                    escaped.append("\\n");
                } else if (character < 0x20) {
                    escaped.append('?');
                } else {
                    escaped.append(character);
                }
            }
            return escaped.toString();
        }

        private static void writeResponse(OutputStream output, String contentType,
                                          byte[] body) throws IOException {
            String header = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\n\r\n";
            output.write(header.getBytes("US-ASCII"));
            output.write(body);
            output.flush();
        }

        private static void writeNotFound(OutputStream output) throws IOException {
            output.write("HTTP/1.1 404 Not Found\r\n\r\n".getBytes("US-ASCII"));
            output.flush();
        }

        private static void writeConnectEstablished(OutputStream output) throws IOException {
            output.write("HTTP/1.1 200 Connection established\r\n\r\n".getBytes("US-ASCII"));
            output.flush();
        }

        private static void echoTunnel(InputStream input, OutputStream output)
                throws IOException {
            byte[] buffer = new byte[4096];
            int count;
            while ((count = input.read(buffer)) != -1) {
                output.write(buffer, 0, count);
                output.flush();
            }
        }

        private static String readLine(InputStream input, int maximumLength)
                throws IOException {
            StringBuffer line = new StringBuffer();
            int value;
            while ((value = input.read()) != -1) {
                if (value == '\n') {
                    int length = line.length();
                    if (length > 0 && line.charAt(length - 1) == '\r') {
                        line.deleteCharAt(length - 1);
                    }
                    return line.toString();
                }
                line.append((char) value);
                if (line.length() > maximumLength) {
                    throw new IOException("HTTP line exceeds maximum length");
                }
            }
            return line.length() == 0 ? null : line.toString();
        }
    }
}
