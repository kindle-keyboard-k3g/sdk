#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kindle::network {

/**
 * Whispernet proxy configuration loaded from environment variables.
 * KINDLE_WHISPERNET_PROXY_HOST and KINDLE_WHISPERNET_PROXY_PORT.
 */
struct ProxyConfig {
    std::string host;
    uint16_t    port = 0;

    /** Returns true when host is non-empty and port is non-zero. */
    bool is_configured() const;

    /**
     * Reads KINDLE_WHISPERNET_PROXY_HOST and KINDLE_WHISPERNET_PROXY_PORT.
     * Returns unconfigured (empty host, port 0) when both vars are absent.
     * Throws std::invalid_argument when vars are present but malformed.
     */
    static ProxyConfig from_environment();
};

/** HTTP header list as name-value pairs. */
using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

/** HTTP request to execute through the proxy. */
struct HttpRequest {
    std::string          method;
    std::string          url;
    HttpHeaders          headers;
    std::vector<uint8_t> body;
};

/**
 * HTTP response from the proxy or a transport error.
 * status_code == 0 indicates a transport-level failure; check error field.
 */
struct HttpResponse {
    int                  status_code = 0;
    std::string          reason;
    HttpHeaders          headers;
    std::vector<uint8_t> body;
    std::string          error;

    bool ok() const { return status_code >= 200 && status_code < 300; }
};

namespace detail {

/** Injectable socket abstraction for unit tests. */
struct SocketBackend {
    virtual ~SocketBackend() = default;
    /** Connect to host:port. Returns fd >= 0 on success, -1 on failure. */
    virtual int connect(const std::string& host, uint16_t port) = 0;
    /** Send all bytes. Returns false if any send fails. */
    virtual bool send_all(int fd, const uint8_t* data, size_t size) = 0;
    /** Receive up to size bytes. Returns bytes read, 0 on EOF, -1 on error. */
    virtual int64_t receive(int fd, uint8_t* data, size_t size) = 0;
    /** Close the file descriptor. */
    virtual void close(int fd) = 0;
};

} // namespace detail

/**
 * RAII TCP connection over a real or injected socket backend.
 * Supports direct connections and HTTP CONNECT proxy tunnels.
 */
class TcpConnection {
public:
    explicit TcpConnection(const ProxyConfig& proxy);
    TcpConnection(const ProxyConfig& proxy,
                  std::shared_ptr<detail::SocketBackend> backend);
    ~TcpConnection();

    TcpConnection(const TcpConnection&)            = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    /** Open a direct TCP connection to host:port. */
    bool connect(const std::string& host, uint16_t port);

    /**
     * Open an HTTP CONNECT tunnel through the configured proxy to host:port.
     * Returns true only when the proxy responds with 200.
     */
    bool connect_tunnel(const std::string& host, uint16_t port);

    bool    send_all(const uint8_t* data, size_t size);
    int64_t receive(uint8_t* data, size_t size);
    bool    is_open() const;
    void    close();

private:
    ProxyConfig                            proxy_;
    std::shared_ptr<detail::SocketBackend> backend_;
    int                                    fd_ = -1;

    bool read_line(std::string& line, size_t max_len);
};

/**
 * HTTP client that routes requests through the Whispernet proxy.
 * HTTPS is not supported natively — returns an explicit error rather
 * than sending plaintext through a CONNECT tunnel.
 */
class HttpClient {
public:
    explicit HttpClient(const ProxyConfig& proxy);
    HttpClient(const ProxyConfig& proxy,
               std::shared_ptr<detail::SocketBackend> backend);

    HttpResponse execute(const HttpRequest& request);
    HttpResponse get(const std::string& url, const HttpHeaders& headers = {});
    HttpResponse post(const std::string& url, const HttpHeaders& headers,
                      const std::vector<uint8_t>& body);

private:
    ProxyConfig                            proxy_;
    std::shared_ptr<detail::SocketBackend> backend_;
};

} // namespace kindle::network
