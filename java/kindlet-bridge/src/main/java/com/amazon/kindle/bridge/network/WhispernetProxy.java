package com.amazon.kindle.bridge.network;

/**
 * Holds the host and port of the device-local Whispernet 3G proxy.
 * Loaded from KINDLE_WHISPERNET_PROXY_HOST and KINDLE_WHISPERNET_PROXY_PORT
 * environment variables; never hardcoded.
 */
public final class WhispernetProxy {

    private final String host;
    private final int port;

    /**
     * Constructs a configured proxy address.
     *
     * @param host proxy hostname or IP address
     * @param port proxy TCP port (1-65535)
     * @throws IllegalArgumentException if host is null/empty or port is out of range
     */
    public WhispernetProxy(String host, int port) {
        if (host == null || host.trim().length() == 0) {
            throw new IllegalArgumentException("Proxy host must not be empty");
        }
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Proxy port out of range: " + port);
        }
        this.host = host;
        this.port = port;
    }

    /**
     * Loads proxy configuration from environment variables.
     * Returns null if KINDLE_WHISPERNET_PROXY_HOST is absent.
     *
     * @return configured WhispernetProxy, or null if unconfigured
     * @throws IllegalArgumentException if HOST is set but PORT is absent or invalid
     */
    public static WhispernetProxy fromEnvironment() {
        String host = System.getProperty("KINDLE_WHISPERNET_PROXY_HOST");
        if (host == null) {
            host = System.getenv("KINDLE_WHISPERNET_PROXY_HOST");
        }
        if (host == null || host.trim().length() == 0) {
            return null;
        }
        String portStr = System.getProperty("KINDLE_WHISPERNET_PROXY_PORT");
        if (portStr == null) {
            portStr = System.getenv("KINDLE_WHISPERNET_PROXY_PORT");
        }
        if (portStr == null || portStr.trim().length() == 0) {
            throw new IllegalArgumentException(
                "KINDLE_WHISPERNET_PROXY_HOST is set but KINDLE_WHISPERNET_PROXY_PORT is absent");
        }
        int port;
        try {
            port = Integer.parseInt(portStr.trim());
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                "KINDLE_WHISPERNET_PROXY_PORT is not a valid integer: " + portStr);
        }
        return new WhispernetProxy(host.trim(), port);
    }

    /** Returns the proxy hostname or IP address. */
    public String getHost() {
        return host;
    }

    /** Returns the proxy TCP port. */
    public int getPort() {
        return port;
    }

    public String toString() {
        return host + ":" + port;
    }
}
