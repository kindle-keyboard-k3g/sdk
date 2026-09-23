package com.amazon.kindle.bridge.network;

/**
 * Holds the host and port of the device-local Whispernet proxy.
 * Configuration uses standard Java system properties for CDC compatibility.
 */
public final class WhispernetProxy {

    public static final String HOST_PROPERTY = "kindle.whispernet.proxy.host";
    public static final String PORT_PROPERTY = "kindle.whispernet.proxy.port";

    private final String host;
    private final int port;

    public WhispernetProxy(String host, int port) {
        if (host == null || host.trim().length() == 0) {
            throw new IllegalArgumentException("Proxy host must not be empty");
        }
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Proxy port out of range: " + port);
        }
        this.host = host.trim();
        this.port = port;
    }

    /**
     * Loads proxy configuration from system properties.
     *
     * <p>System.getenv() is a Java 5 API and is not available on CDC; callers
     * must use system properties rather than relying on an environment shim.</p>
     *
     * @return configured proxy, or null when the host property is absent
     * @throws IllegalArgumentException when the properties are incomplete or invalid
     */
    public static WhispernetProxy fromSystemProperties() {
        String host = System.getProperty(HOST_PROPERTY);
        if (host == null || host.trim().length() == 0) {
            return null;
        }

        String portText = System.getProperty(PORT_PROPERTY);
        if (portText == null || portText.trim().length() == 0) {
            throw new IllegalArgumentException(
                PORT_PROPERTY + " is required when " + HOST_PROPERTY + " is set");
        }

        int port;
        try {
            port = Integer.parseInt(portText.trim());
        } catch (NumberFormatException error) {
            throw new IllegalArgumentException(
                PORT_PROPERTY + " is not a valid integer: " + portText);
        }
        return new WhispernetProxy(host, port);
    }

    public String getHost() {
        return host;
    }

    public int getPort() {
        return port;
    }

    public boolean isConfigured() {
        return host.length() > 0 && port > 0;
    }

    public String toString() {
        return host + ":" + port;
    }
}
