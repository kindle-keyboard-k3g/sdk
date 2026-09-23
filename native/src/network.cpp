#include "kindle/network.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef SO_NOSIGPIPE
#define KINDLE_HAS_SO_NOSIGPIPE 1
#else
#define KINDLE_HAS_SO_NOSIGPIPE 0
#endif

namespace kindle::network {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

constexpr size_t BODY_LIMIT        = 1024u * 1024u; // 1 MiB
constexpr size_t MAX_STATUS_LINE   = 1024u;
constexpr size_t MAX_HEADER_LINE   = 8192u;
constexpr size_t MAX_HEADER_COUNT  = 100u;

/** ASCII lower-case comparison for header name matching. */
bool iequal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

bool contains_forbidden_request_character(const std::string& value) {
    return value.find('\r') != std::string::npos ||
           value.find('\n') != std::string::npos ||
           value.find('\0') != std::string::npos;
}

/** Trim leading and trailing whitespace from a string view in-place. */
std::string trim(std::string s) {
    const auto is_ws = [](unsigned char c) { return std::isspace(c); };
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), is_ws));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), is_ws).base(), s.end());
    return s;
}

struct ParsedUrl {
    std::string scheme;
    std::string host;
    uint16_t    port  = 0;
    std::string path;
    bool        valid = false;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl r;
    // scheme
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return r;
    r.scheme = url.substr(0, scheme_end);
    if (r.scheme != "http" && r.scheme != "https") return r;

    const auto after_scheme = scheme_end + 3;
    const auto path_start   = url.find('/', after_scheme);
    const auto host_port_str =
        (path_start == std::string::npos)
            ? url.substr(after_scheme)
            : url.substr(after_scheme, path_start - after_scheme);

    r.path = (path_start == std::string::npos) ? "/" : url.substr(path_start);

    // host:port
    const auto colon = host_port_str.rfind(':');
    if (colon == std::string::npos) {
        r.host = host_port_str;
        r.port = (r.scheme == "https") ? 443u : 80u;
    } else {
        r.host          = host_port_str.substr(0, colon);
        const auto port_str = host_port_str.substr(colon + 1);
        int        p        = 0;
        auto [ptr, ec]      = std::from_chars(
            port_str.data(), port_str.data() + port_str.size(), p);
        if (ec != std::errc{} || ptr != port_str.data() + port_str.size() ||
            p < 1 || p > 65535) return r;
        r.port = static_cast<uint16_t>(p);
    }

    if (r.host.empty()) return r;
    r.valid = true;
    return r;
}

/** Find a header value by case-insensitive name; returns "" if absent. */
std::string find_header(const HttpHeaders& hdrs, const std::string& name) {
    for (const auto& h : hdrs) {
        if (iequal(h.first, name)) return h.second;
    }
    return "";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Default POSIX socket backend
// ---------------------------------------------------------------------------

namespace detail {

class PosixSocketBackend : public SocketBackend {
public:
    int connect(const std::string& host, uint16_t port) override {
        const std::string port_str = std::to_string(port);
        addrinfo hints{};
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* res = nullptr;
        if (::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0)
            return -1;

        int fd = -1;
        for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
            fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd < 0) continue;

#if KINDLE_HAS_SO_NOSIGPIPE
            int one = 1;
            ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
            if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
            ::close(fd);
            fd = -1;
        }
        ::freeaddrinfo(res);
        return fd;
    }

    bool send_all(int fd, const uint8_t* data, size_t size) override {
        size_t sent = 0;
        while (sent < size) {
            int flags = 0;
#ifndef SO_NOSIGPIPE
            flags = MSG_NOSIGNAL;
#endif
            const ssize_t n = ::send(
                fd,
                reinterpret_cast<const char*>(data + sent),
                size - sent,
                flags);
            if (n <= 0) return false;
            sent += static_cast<size_t>(n);
        }
        return true;
    }

    int64_t receive(int fd, uint8_t* data, size_t size) override {
        const ssize_t n = ::recv(fd, reinterpret_cast<char*>(data), size, 0);
        return n;
    }

    void close(int fd) override { ::close(fd); }
};

} // namespace detail

// ---------------------------------------------------------------------------
// ProxyConfig
// ---------------------------------------------------------------------------

bool ProxyConfig::is_configured() const {
    return !host.empty() && port != 0;
}

ProxyConfig ProxyConfig::from_environment() {
    const char* h = std::getenv("KINDLE_WHISPERNET_PROXY_HOST");
    const char* p = std::getenv("KINDLE_WHISPERNET_PROXY_PORT");

    const bool has_host = h != nullptr && h[0] != '\0';
    const bool has_port = p != nullptr && p[0] != '\0';

    if (!has_host && !has_port) return ProxyConfig{};

    if (has_host && !has_port)
        throw std::invalid_argument(
            "KINDLE_WHISPERNET_PROXY_HOST is set but KINDLE_WHISPERNET_PROXY_PORT is missing");
    if (!has_host && has_port)
        throw std::invalid_argument(
            "KINDLE_WHISPERNET_PROXY_PORT is set but KINDLE_WHISPERNET_PROXY_HOST is missing");

    int port_val = 0;
    const std::string port_str(p);
    auto [ptr, ec] = std::from_chars(
        port_str.data(), port_str.data() + port_str.size(), port_val);
    if (ec != std::errc{} || ptr != port_str.data() + port_str.size() ||
        port_val < 1 || port_val > 65535)
        throw std::invalid_argument(
            "KINDLE_WHISPERNET_PROXY_PORT must be an integer in 1-65535");

    return ProxyConfig{std::string(h), static_cast<uint16_t>(port_val)};
}

// ---------------------------------------------------------------------------
// TcpConnection
// ---------------------------------------------------------------------------

TcpConnection::TcpConnection(const ProxyConfig& proxy)
    : proxy_(proxy),
      backend_(std::make_shared<detail::PosixSocketBackend>()) {}

TcpConnection::TcpConnection(const ProxyConfig& proxy,
                             std::shared_ptr<detail::SocketBackend> backend)
    : proxy_(proxy), backend_(std::move(backend)) {}

TcpConnection::~TcpConnection() { close(); }

bool TcpConnection::connect(const std::string& host, uint16_t port) {
    close();
    fd_ = backend_->connect(host, port);
    return fd_ >= 0;
}

bool TcpConnection::read_line(std::string& line, size_t max_len) {
    line.clear();
    uint8_t ch = 0;
    while (true) {
        const int64_t n = backend_->receive(fd_, &ch, 1);
        if (n <= 0) return false;
        if (ch == '\n') {
            // strip trailing \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
        if (line.size() >= max_len) return false; // line too long
        line.push_back(static_cast<char>(ch));
    }
}

bool TcpConnection::connect_tunnel(const std::string& host, uint16_t port) {
    close();
    if (contains_forbidden_request_character(host)) return false;

    if (!proxy_.is_configured()) {
        // No proxy: direct TCP to target
        return connect(host, port);
    }

    // Connect to proxy first
    fd_ = backend_->connect(proxy_.host, proxy_.port);
    if (fd_ < 0) return false;

    // Send CONNECT request
    const std::string target = host + ":" + std::to_string(port);
    const std::string req =
        "CONNECT " + target + " HTTP/1.1\r\n"
        "Host: " + target + "\r\n"
        "\r\n";
    if (!backend_->send_all(
            fd_,
            reinterpret_cast<const uint8_t*>(req.data()),
            req.size())) {
        close();
        return false;
    }

    // Read status line
    std::string status_line;
    if (!read_line(status_line, MAX_STATUS_LINE)) { close(); return false; }

    // Parse: "HTTP/x.y 200 ..."
    if (status_line.size() < 12 ||
        status_line.substr(0, 5) != "HTTP/" ||
        status_line.substr(8, 1) != " ") {
        close();
        return false;
    }
    const std::string code_str = status_line.substr(9, 3);
    int code = 0;
    auto [ptr, ec] = std::from_chars(
        code_str.data(), code_str.data() + code_str.size(), code);
    if (ec != std::errc{} || ptr != code_str.data() + code_str.size() ||
        code != 200) {
        close();
        return false;
    }

    // Drain remaining proxy headers
    size_t header_count = 0;
    while (true) {
        std::string hdr;
        if (!read_line(hdr, MAX_HEADER_LINE)) { close(); return false; }
        if (hdr.empty()) break; // blank line = end of headers
        if (++header_count > MAX_HEADER_COUNT) { close(); return false; }
    }

    return true;
}

bool TcpConnection::send_all(const uint8_t* data, size_t size) {
    if (fd_ < 0) return false;
    return backend_->send_all(fd_, data, size);
}

int64_t TcpConnection::receive(uint8_t* data, size_t size) {
    if (fd_ < 0) return -1;
    return backend_->receive(fd_, data, size);
}

bool TcpConnection::is_open() const { return fd_ >= 0; }

void TcpConnection::close() {
    if (fd_ >= 0) {
        backend_->close(fd_);
        fd_ = -1;
    }
}

// ---------------------------------------------------------------------------
// HttpClient — internal helpers
// ---------------------------------------------------------------------------

namespace {

struct ResponseReader {
    detail::SocketBackend* backend;
    int fd;
    std::vector<uint8_t> buf; // lookahead buffer
    size_t buf_pos = 0;

    // Read one byte; returns false on EOF/error
    bool read_byte(uint8_t& out) {
        if (buf_pos < buf.size()) {
            out = buf[buf_pos++];
            return true;
        }
        const int64_t n = backend->receive(fd, &out, 1);
        return n > 0;
    }

    // Read a CRLF-terminated line (strips CRLF, enforces max_len)
    bool read_line(std::string& line, size_t max_len) {
        line.clear();
        uint8_t ch = 0;
        while (true) {
            if (!read_byte(ch)) return false;
            if (ch == '\n') {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return true;
            }
            if (line.size() >= max_len) return false;
            line.push_back(static_cast<char>(ch));
        }
    }

    // Read exactly n bytes
    bool read_exact(std::vector<uint8_t>& out, size_t n) {
        out.reserve(out.size() + n);
        uint8_t tmp[4096];
        size_t remaining = n;
        while (remaining > 0) {
            const size_t chunk = std::min(remaining, sizeof(tmp));
            const int64_t got  = backend->receive(fd, tmp, chunk);
            if (got <= 0) return false;
            out.insert(out.end(), tmp, tmp + got);
            remaining -= static_cast<size_t>(got);
        }
        return true;
    }

    bool read_crlf_line(std::string& line, size_t max_len) {
        line.clear();
        uint8_t ch = 0;
        while (true) {
            if (!read_byte(ch)) return false;
            if (ch == '\r') {
                uint8_t next = 0;
                if (!read_byte(next) || next != '\n') return false;
                return true;
            }
            if (ch == '\n') return false;
            if (line.size() >= max_len) return false;
            line.push_back(static_cast<char>(ch));
        }
    }

    // Read until connection close
    bool read_until_eof(std::vector<uint8_t>& out) {
        uint8_t tmp[4096];
        while (true) {
            const int64_t n = backend->receive(fd, tmp, sizeof(tmp));
            if (n == 0) break; // EOF
            if (n < 0) return false;
            const size_t received = static_cast<size_t>(n);
            if (received > BODY_LIMIT - out.size()) return false;
            out.insert(out.end(), tmp, tmp + received);
        }
        return true;
    }

    // Read chunked transfer-encoded body
    bool read_chunked(std::vector<uint8_t>& out) {
        while (true) {
            std::string size_line;
            if (!read_crlf_line(size_line, 128)) return false;
            size_line = trim(size_line);
            if (size_line.empty()) return false;

            size_t digit_count = 0;
            while (digit_count < size_line.size()) {
                const unsigned char digit =
                    static_cast<unsigned char>(size_line[digit_count]);
                if (!std::isxdigit(digit)) break;
                ++digit_count;
            }
            if (digit_count == 0) return false;
            for (size_t index = digit_count; index < size_line.size(); ++index) {
                if (!std::isspace(static_cast<unsigned char>(size_line[index])))
                    return false;
            }

            size_t chunk_size = 0;
            for (size_t index = 0; index < digit_count; ++index) {
                const unsigned char digit =
                    static_cast<unsigned char>(size_line[index]);
                const size_t value = std::isdigit(digit)
                    ? static_cast<size_t>(digit - '0')
                    : static_cast<size_t>(std::tolower(digit) - 'a' + 10);
                if (chunk_size > (BODY_LIMIT - value) / 16u) return false;
                chunk_size = chunk_size * 16u + value;
            }

            if (chunk_size == 0) {
                std::string trailer;
                while (true) {
                    if (!read_crlf_line(trailer, MAX_HEADER_LINE)) return false;
                    if (trailer.empty()) return true;
                }
            }

            if (chunk_size > BODY_LIMIT - out.size()) return false;
            if (!read_exact(out, chunk_size)) return false;

            uint8_t first = 0;
            uint8_t second = 0;
            if (!read_byte(first) || !read_byte(second) ||
                first != '\r' || second != '\n') return false;
        }
    }
};

HttpResponse build_error(const std::string& msg) {
    return HttpResponse{0, {}, {}, {}, msg};
}

HttpResponse do_execute(const HttpRequest& request,
                        const ProxyConfig& proxy,
                        detail::SocketBackend* backend_ptr) {
    // Validate all request fields before any socket operation.
    if (request.method.empty()) return build_error("empty method");
    if (contains_forbidden_request_character(request.method))
        return build_error("invalid method: contains CRLF or NUL");
    if (contains_forbidden_request_character(request.url))
        return build_error("invalid URL: contains CRLF or NUL");
    for (const auto& header : request.headers) {
        if (contains_forbidden_request_character(header.first) ||
            contains_forbidden_request_character(header.second))
            return build_error("invalid header: contains CRLF or NUL");
    }

    const ParsedUrl purl = parse_url(request.url);
    if (!purl.valid) return build_error("invalid URL: " + request.url);

    // HTTPS guard — never send plaintext
    if (purl.scheme == "https") {
        return build_error("TLS backend not available");
    }

    const bool use_proxy = proxy.is_configured();
    const std::string& connect_host = use_proxy ? proxy.host : purl.host;
    const uint16_t     connect_port = use_proxy ? proxy.port : purl.port;

    const int fd = backend_ptr->connect(connect_host, connect_port);
    if (fd < 0) return build_error("connection refused to " + connect_host +
                                   ":" + std::to_string(connect_port));

    // Scope-guard: always close fd
    struct FdGuard {
        detail::SocketBackend* b; int f;
        ~FdGuard() { if (f >= 0) b->close(f); }
    } guard{backend_ptr, fd};

    // Build request line
    // Absolute-form through proxy; origin-form direct
    const std::string request_target =
        use_proxy ? request.url : purl.path;

    std::ostringstream req_stream;
    req_stream << request.method << " " << request_target << " HTTP/1.1\r\n";

    // Default headers (user-supplied override)
    bool has_host   = false;
    bool has_conn   = false;
    bool has_cl     = false;
    for (const auto& h : request.headers) {
        if (iequal(h.first, "Host"))           has_host = true;
        if (iequal(h.first, "Connection"))     has_conn = true;
        if (iequal(h.first, "Content-Length")) has_cl   = true;
    }
    if (!has_host) {
        req_stream << "Host: " << purl.host;
        if ((purl.scheme == "http"  && purl.port != 80) ||
            (purl.scheme == "https" && purl.port != 443))
            req_stream << ":" << purl.port;
        req_stream << "\r\n";
    }
    if (!has_conn) req_stream << "Connection: close\r\n";
    if (!has_cl && !request.body.empty())
        req_stream << "Content-Length: " << request.body.size() << "\r\n";

    for (const auto& h : request.headers) {
        if (h.first.find('\r')  != std::string::npos ||
            h.first.find('\n')  != std::string::npos ||
            h.second.find('\r') != std::string::npos ||
            h.second.find('\n') != std::string::npos)
            return build_error("invalid header: contains CRLF");
        req_stream << h.first << ": " << h.second << "\r\n";
    }
    req_stream << "\r\n";

    const std::string req_str = req_stream.str();
    if (!backend_ptr->send_all(
            fd,
            reinterpret_cast<const uint8_t*>(req_str.data()),
            req_str.size()))
        return build_error("failed to send request headers");

    if (!request.body.empty()) {
        if (!backend_ptr->send_all(fd, request.body.data(), request.body.size()))
            return build_error("failed to send request body");
    }

    // Read response
    ResponseReader reader{backend_ptr, fd, {}, 0};

    // Status line
    std::string status_line;
    if (!reader.read_line(status_line, MAX_STATUS_LINE))
        return build_error("failed to read status line");

    // Parse "HTTP/1.x NNN reason"
    if (status_line.size() < 12 || status_line.substr(0, 5) != "HTTP/")
        return build_error("malformed status line: " + status_line);

    const auto space1 = status_line.find(' ', 5);
    if (space1 == std::string::npos)
        return build_error("malformed status line (no status code)");

    const std::string code_str = status_line.substr(space1 + 1, 3);
    int status = 0;
    {
        auto [ptr, ec] = std::from_chars(
            code_str.data(), code_str.data() + code_str.size(), status);
        if (ec != std::errc{} || ptr != code_str.data() + code_str.size() ||
            status < 100 || status > 999)
            return build_error("invalid status code: " + code_str);
    }
    const std::string reason =
        (space1 + 4 < status_line.size())
            ? trim(status_line.substr(space1 + 4))
            : "";

    // Response headers
    HttpHeaders resp_headers;
    size_t hdr_count = 0;
    while (true) {
        std::string hdr;
        if (!reader.read_line(hdr, MAX_HEADER_LINE))
            return build_error("failed to read response header");
        if (hdr.empty()) break;
        if (++hdr_count > MAX_HEADER_COUNT)
            return build_error("too many response headers");
        const auto colon = hdr.find(':');
        if (colon == std::string::npos) continue; // skip malformed header
        resp_headers.emplace_back(
            trim(hdr.substr(0, colon)),
            trim(hdr.substr(colon + 1)));
    }

    // Body
    std::vector<uint8_t> body;
    const std::string te  = find_header(resp_headers, "Transfer-Encoding");
    const std::string cl  = find_header(resp_headers, "Content-Length");

    // No body for 1xx, 204, 304
    const bool no_body = (status / 100 == 1) || status == 204 || status == 304;

    if (!no_body) {
        bool ok = true;
        if (iequal(te, "chunked")) {
            ok = reader.read_chunked(body);
        } else if (!cl.empty()) {
            size_t content_len = 0;
            {
                auto [ptr, ec] = std::from_chars(
                    cl.data(), cl.data() + cl.size(), content_len);
                if (ec != std::errc{} || ptr != cl.data() + cl.size())
                    return build_error("invalid Content-Length: " + cl);
            }
            if (content_len > BODY_LIMIT)
                return build_error("response body exceeds 1 MiB limit");
            ok = reader.read_exact(body, content_len);
        } else {
            ok = reader.read_until_eof(body);
        }
        if (!ok) return build_error("failed to read response body");
    }

    return HttpResponse{status, reason, std::move(resp_headers), std::move(body), {}};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// HttpClient
// ---------------------------------------------------------------------------

HttpClient::HttpClient(const ProxyConfig& proxy)
    : proxy_(proxy),
      backend_(std::make_shared<detail::PosixSocketBackend>()) {}

HttpClient::HttpClient(const ProxyConfig& proxy,
                       std::shared_ptr<detail::SocketBackend> backend)
    : proxy_(proxy), backend_(std::move(backend)) {}

HttpResponse HttpClient::execute(const HttpRequest& request) {
    try {
        return do_execute(request, proxy_, backend_.get());
    } catch (const std::exception& e) {
        return build_error(std::string("unexpected exception: ") + e.what());
    } catch (...) {
        return build_error("unexpected unknown exception");
    }
}

HttpResponse HttpClient::get(const std::string& url,
                             const HttpHeaders& headers) {
    return execute(HttpRequest{"GET", url, headers, {}});
}

HttpResponse HttpClient::post(const std::string& url,
                              const HttpHeaders& headers,
                              const std::vector<uint8_t>& body) {
    return execute(HttpRequest{"POST", url, headers, body});
}

} // namespace kindle::network
