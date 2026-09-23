#include "kindle/network.hpp"
#include "kindle/network_ipc.hpp"
#include "../platform/fake/fake_network.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

// ---- helpers ---------------------------------------------------------------

static int  g_passed = 0;
static int  g_total  = 0;

#define TEST(name, expr)                                                    \
    do {                                                                    \
        ++g_total;                                                          \
        if (expr) {                                                         \
            ++g_passed;                                                     \
            std::cout << "PASS: " << name << "\n";                         \
        } else {                                                            \
            std::cout << "FAIL: " << name << "\n";                         \
        }                                                                   \
    } while (0)

#define TEST_THROWS(name, expr)                                             \
    do {                                                                    \
        ++g_total;                                                          \
        bool threw = false;                                                 \
        try { expr; } catch (const std::exception&) { threw = true; }      \
        if (threw) {                                                        \
            ++g_passed;                                                     \
            std::cout << "PASS: " << name << "\n";                         \
        } else {                                                            \
            std::cout << "FAIL: " << name << " (expected exception)\n";    \
        }                                                                   \
    } while (0)

using namespace kindle::network;

// Helper: build a simple HTTP response string
static std::string ok_response(const std::string& body,
                                const std::string& extra_headers = "") {
    return "HTTP/1.1 200 OK\r\nContent-Length: " +
           std::to_string(body.size()) + "\r\n" +
           extra_headers +
           "\r\n" + body;
}

static std::string chunked_response(const std::string& body) {
    // Split body into two chunks
    size_t half = body.size() / 2;
    std::string c1 = body.substr(0, half);
    std::string c2 = body.substr(half);
    std::string resp = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
    // chunk 1, including an extension that must be ignored
    char buf[32];
    snprintf(buf, sizeof(buf), "%zx;ext=val\r\n", c1.size());
    resp += buf + c1 + "\r\n";
    // chunk 2
    snprintf(buf, sizeof(buf), "%zx\r\n", c2.size());
    resp += buf + c2 + "\r\n";
    // terminator
    resp += "0\r\n\r\n";
    return resp;
}

// ============================================================================
// Category A: ProxyConfig
// ============================================================================
static void test_proxy_config() {
    std::cout << "\n-- Category A: ProxyConfig --\n";

    // A1: Both env vars absent → unconfigured
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");
    unsetenv("KINDLE_WHISPERNET_PROXY_PORT");
    {
        ProxyConfig pc = ProxyConfig::from_environment();
        TEST("A1 unconfigured when both vars absent",
             !pc.is_configured() && pc.host.empty());
    }

    // A2: Both present and valid → configured
    setenv("KINDLE_WHISPERNET_PROXY_HOST", "192.168.1.1", 1);
    setenv("KINDLE_WHISPERNET_PROXY_PORT", "8080", 1);
    {
        ProxyConfig pc = ProxyConfig::from_environment();
        TEST("A2 configured with valid HOST+PORT",
             pc.is_configured() && pc.host == "192.168.1.1" && pc.port == 8080);
    }
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");
    unsetenv("KINDLE_WHISPERNET_PROXY_PORT");

    // A3: HOST present but PORT absent → throws
    setenv("KINDLE_WHISPERNET_PROXY_HOST", "proxy.example", 1);
    TEST_THROWS("A3 throws when HOST present PORT absent",
                ProxyConfig::from_environment());
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");

    // A4: PORT not a valid integer → throws
    setenv("KINDLE_WHISPERNET_PROXY_HOST", "proxy.example", 1);
    setenv("KINDLE_WHISPERNET_PROXY_PORT", "notanumber", 1);
    TEST_THROWS("A4 throws when PORT is not a valid integer",
                ProxyConfig::from_environment());
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");
    unsetenv("KINDLE_WHISPERNET_PROXY_PORT");

    // A5: PORT out of range (0) → throws
    setenv("KINDLE_WHISPERNET_PROXY_HOST", "proxy.example", 1);
    setenv("KINDLE_WHISPERNET_PROXY_PORT", "0", 1);
    TEST_THROWS("A5 throws when PORT is 0",
                ProxyConfig::from_environment());
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");
    unsetenv("KINDLE_WHISPERNET_PROXY_PORT");

    // A6: PORT out of range (>65535) → throws
    setenv("KINDLE_WHISPERNET_PROXY_HOST", "proxy.example", 1);
    setenv("KINDLE_WHISPERNET_PROXY_PORT", "99999", 1);
    TEST_THROWS("A6 throws when PORT > 65535",
                ProxyConfig::from_environment());
    unsetenv("KINDLE_WHISPERNET_PROXY_HOST");
    unsetenv("KINDLE_WHISPERNET_PROXY_PORT");

    // A7: is_configured false for empty host
    ProxyConfig pc;
    TEST("A7 is_configured() false for empty host", !pc.is_configured());
}

// ============================================================================
// Category B: HTTP request formatting
// ============================================================================
static void test_request_formatting() {
    std::cout << "\n-- Category B: Request Formatting --\n";

    // B1: GET through proxy → absolute-form
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("hello"));
        ProxyConfig proxy{"myproxy", 3128};
        HttpClient client(proxy, fake);
        auto resp = client.get("http://example.com/path");
        std::string req = fake->captured_request(1);
        TEST("B1 GET through proxy sends absolute-form",
             req.find("GET http://example.com/path HTTP/1.1") != std::string::npos);
    }

    // B2: GET without proxy → origin-form
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("world"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/path");
        std::string req = fake->captured_request(1);
        TEST("B2 GET without proxy sends origin-form",
             req.find("GET /path HTTP/1.1") != std::string::npos &&
             req.find("GET http://") == std::string::npos);
    }

    // B3: POST with body → Content-Length header present and correct
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("created"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        std::vector<uint8_t> body = {'d','a','t','a'};
        auto resp = client.post("http://example.com/submit", {}, body);
        std::string req = fake->captured_request(1);
        TEST("B3 POST sends Content-Length: 4",
             req.find("Content-Length: 4") != std::string::npos);
    }

    // B4: Request includes Host and Connection: close
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("ok"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        client.get("http://example.com/");
        std::string req = fake->captured_request(1);
        TEST("B4 request includes Host header",
             req.find("Host: example.com") != std::string::npos);
        TEST("B4b request includes Connection: close",
             req.find("Connection: close") != std::string::npos);
    }

    // B5: User header is included in request
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("ok"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        HttpRequest req;
        req.method  = "GET";
        req.url     = "http://example.com/";
        req.headers = {{"X-Custom", "myvalue"}};
        client.execute(req);
        std::string sent = fake->captured_request(1);
        TEST("B5 user-supplied header appears in request",
             sent.find("X-Custom: myvalue") != std::string::npos);
    }
}

// ============================================================================
// Category C: Response parsing
// ============================================================================
static void test_response_parsing() {
    std::cout << "\n-- Category C: Response Parsing --\n";

    // C1: Content-Length body
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(ok_response("hello world"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        std::string body(resp.body.begin(), resp.body.end());
        TEST("C1 Content-Length body parsed correctly",
             resp.status_code == 200 && body == "hello world");
    }

    // C2: Chunked body
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(chunked_response("chunkeddata"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        std::string body(resp.body.begin(), resp.body.end());
        TEST("C2 chunked body assembled correctly",
             resp.status_code == 200 && body == "chunkeddata");
    }

    // C3: Close-delimited body
    {
        // No Content-Length, no chunked → read until EOF
        std::string raw = "HTTP/1.1 200 OK\r\n\r\nbodydata";
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response(raw);
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        std::string body(resp.body.begin(), resp.body.end());
        TEST("C3 close-delimited body read correctly",
             resp.status_code == 200 && body == "bodydata");
    }

    // C4: Mixed-case headers recognised
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        std::string raw = "HTTP/1.1 200 OK\r\ncontent-type: text/plain\r\nContent-Length: 2\r\n\r\nhi";
        fake->enqueue_response(raw);
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        // Check body was read correctly (Content-Length was parsed)
        std::string body(resp.body.begin(), resp.body.end());
        TEST("C4 mixed-case Content-Length parsed",
             resp.status_code == 200 && body == "hi");
    }

    // C5: Fragmented reads
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->set_fragment_size(5); // deliver 5 bytes at a time
        fake->enqueue_response(ok_response("fragmented"));
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        std::string body(resp.body.begin(), resp.body.end());
        TEST("C5 fragmented reads assembled correctly",
             resp.status_code == 200 && body == "fragmented");
    }
}

// ============================================================================
// Category D: Error handling
// ============================================================================
static void test_error_handling() {
    std::cout << "\n-- Category D: Error Handling --\n";

    // D1: Bad status line
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response("GARBAGE\r\n\r\n");
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        TEST("D1 bad status line returns transport error",
             resp.status_code == 0 && !resp.error.empty());
    }

    // D2: Invalid chunk size
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nZZZZ\r\nbody\r\n0\r\n\r\n");
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        TEST("D2 invalid chunk size returns transport error",
             resp.status_code == 0);
    }

    // D3: Body exceeds 1 MiB via Content-Length
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        // Report Content-Length > 1 MiB
        size_t big = 1024 * 1024 + 1;
        std::string raw = "HTTP/1.1 200 OK\r\nContent-Length: " +
                          std::to_string(big) + "\r\n\r\n";
        raw += std::string(big, 'x'); // won't actually be read but limit kicks in
        fake->enqueue_response(raw);
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        TEST("D3 Content-Length > 1 MiB returns transport error",
             resp.status_code == 0);
    }

    // D4: Body exceeds 1 MiB via chunked
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        // Announce a chunk larger than 1 MiB
        size_t big = 1024 * 1024 + 1;
        char buf[64];
        snprintf(buf, sizeof(buf), "%zx\r\n", big);
        std::string raw = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
        raw += buf;
        raw += std::string(big, 'y');
        raw += "\r\n0\r\n\r\n";
        fake->enqueue_response(raw);
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        TEST("D4 chunked body > 1 MiB returns transport error",
             resp.status_code == 0);
    }

    // D5: Connect failure
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->set_connect_fails(true);
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("http://example.com/");
        TEST("D5 connect failure returns transport error",
             resp.status_code == 0 && !resp.error.empty());
    }
}

// ============================================================================
// Category E: CONNECT tunnel
// ============================================================================
static void test_connect_tunnel() {
    std::cout << "\n-- Category E: CONNECT Tunnel --\n";

    // E1: connect_tunnel() sends correct CONNECT request
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response("HTTP/1.1 200 Connection established\r\n\r\n");
        ProxyConfig proxy{"myproxy", 3128};
        TcpConnection conn(proxy, fake);
        bool ok = conn.connect_tunnel("target.example.com", 443);
        std::string req = fake->captured_request(1);
        TEST("E1 CONNECT request sent with correct target",
             req.find("CONNECT target.example.com:443") != std::string::npos);
        TEST("E1b connect_tunnel returns true on 200",
             ok);
    }

    // E2: connect_tunnel() returns false on proxy 407
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response("HTTP/1.1 407 Proxy Auth Required\r\n\r\n");
        ProxyConfig proxy{"myproxy", 3128};
        TcpConnection conn(proxy, fake);
        bool ok = conn.connect_tunnel("target.example.com", 443);
        TEST("E2 connect_tunnel returns false on 407",
             !ok);
    }

    // E3: connect_tunnel() returns false on malformed response
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        fake->enqueue_response("GARBAGE RESPONSE\r\n\r\n");
        ProxyConfig proxy{"myproxy", 3128};
        TcpConnection conn(proxy, fake);
        bool ok = conn.connect_tunnel("target.example.com", 443);
        TEST("E3 connect_tunnel returns false on malformed response",
             !ok);
    }

    // E4: connect_tunnel() drains all proxy response headers
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        // 200 with several headers — the tunnel is established after draining them
        fake->enqueue_response(
            "HTTP/1.1 200 Connection established\r\n"
            "Via: 1.1 proxy\r\n"
            "Proxy-Agent: MyProxy/1.0\r\n"
            "\r\n");
        ProxyConfig proxy{"myproxy", 3128};
        TcpConnection conn(proxy, fake);
        bool ok = conn.connect_tunnel("target.example.com", 443);
        TEST("E4 connect_tunnel drains all proxy headers and returns true",
             ok);
    }
}

// ============================================================================
// Category F: HTTPS guard
// ============================================================================
static void test_https_guard() {
    std::cout << "\n-- Category F: HTTPS Guard --\n";

    // F1: https:// URL returns error
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        auto resp = client.get("https://example.com/");
        TEST("F1 https:// URL returns transport error",
             resp.status_code == 0 && !resp.error.empty());
    }

    // F2: No socket connection attempted for https://
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        client.get("https://example.com/");
        TEST("F2 no connect() call for https:// URL",
             fake->connect_call_count() == 0);
    }
}

// ============================================================================
// Category G: CRLF header injection guard
// ============================================================================
static void test_crlf_injection_guard() {
    std::cout << "\n-- Category G: CRLF Injection Guard --\n";

    // G1: Header name containing \r\n → error, nothing sent
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        HttpRequest req;
        req.method  = "GET";
        req.url     = "http://example.com/";
        req.headers = {{"Bad\r\nHeader", "value"}};
        auto resp = client.execute(req);
        TEST("G1 header name with CRLF returns error",
             resp.status_code == 0 && !resp.error.empty());
        TEST("G1b nothing sent when CRLF in header name",
             fake->captured_request(1).empty());
    }

    // G2: Header value containing \n → error, nothing sent
    {
        auto fake = std::make_shared<FakeSocketBackend>();
        ProxyConfig no_proxy;
        HttpClient client(no_proxy, fake);
        HttpRequest req;
        req.method  = "GET";
        req.url     = "http://example.com/";
        req.headers = {{"X-Header", "injected\nEvil: value"}};
        auto resp = client.execute(req);
        TEST("G2 header value with newline returns error",
             resp.status_code == 0 && !resp.error.empty());
        TEST("G2b nothing sent when CRLF in header value",
             fake->captured_request(1).empty());
    }
}

// ============================================================================
// Category H: IPC Codec
// ============================================================================
static void test_ipc_codec() {
    using namespace kindle::network;
    using namespace kindle::network::ipc;

    // H1: encode_request + decode_request round-trip (GET, headers, empty body)
    {
        HttpRequest orig;
        orig.method  = "GET";
        orig.url     = "http://example.com/path?q=1";
        orig.headers = {{"Host", "example.com"}, {"Accept", "text/html"}};
        auto payload = encode_request(orig);
        TEST("H1 encode_request returns non-empty", !payload.empty());
        HttpRequest decoded;
        bool ok = decode_request(payload, decoded);
        TEST("H1 decode_request succeeds", ok);
        TEST("H1 method round-trips",  decoded.method == orig.method);
        TEST("H1 url round-trips",     decoded.url    == orig.url);
        TEST("H1 header count",        decoded.headers.size() == orig.headers.size());
        TEST("H1 header[0] round-trips", decoded.headers[0].first  == "Host" &&
                                         decoded.headers[0].second == "example.com");
        TEST("H1 body empty",          decoded.body.empty());
    }

    // H2: encode_request + decode_request round-trip (POST with binary body)
    {
        HttpRequest orig;
        orig.method  = "POST";
        orig.url     = "http://example.com/api";
        orig.headers = {{"Content-Type", "application/octet-stream"}};
        orig.body    = {0x00, 0x01, 0xFF, 0x00, 0x42};
        auto payload = encode_request(orig);
        TEST("H2 encode POST returns non-empty", !payload.empty());
        HttpRequest decoded;
        bool ok = decode_request(payload, decoded);
        TEST("H2 decode POST succeeds", ok);
        TEST("H2 binary body round-trips", decoded.body == orig.body);
    }

    // H3: decode_request with truncated payload → returns false
    {
        HttpRequest orig;
        orig.method = "GET";
        orig.url    = "http://example.com/";
        auto payload = encode_request(orig);
        payload.resize(payload.size() / 2);
        HttpRequest decoded;
        TEST("H3 truncated payload rejected", !decode_request(payload, decoded));
    }

    // H4: decode_request with body_length exceeding payload → returns false
    {
        HttpRequest orig;
        orig.method = "GET";
        orig.url    = "http://example.com/";
        auto payload = encode_request(orig);
        // Overwrite body_length field (bytes 6-9) with a huge value
        if (payload.size() >= 10) {
            payload[6] = 0x01;
            payload[7] = 0x00;
            payload[8] = 0x00;
            payload[9] = 0x00;
        }
        HttpRequest decoded;
        TEST("H4 body_length overflow rejected", !decode_request(payload, decoded));
    }

    // H5: encode_request with method > 255 chars → returns empty vector
    {
        HttpRequest req;
        req.method = std::string(256, 'X');
        req.url    = "http://example.com/";
        auto payload = encode_request(req);
        TEST("H5 method > 255 bytes yields empty", payload.empty());
    }

    // H6: encode_response + decode_response round-trip (200 OK with headers and body)
    {
        HttpResponse orig;
        orig.status_code = 200;
        orig.reason      = "OK";
        orig.headers     = {{"Content-Type", "text/plain"}, {"X-Foo", "bar"}};
        orig.body        = {'H', 'e', 'l', 'l', 'o'};
        auto payload = encode_response(orig);
        TEST("H6 encode_response returns non-empty", !payload.empty());
        HttpResponse decoded;
        bool ok = decode_response(payload, decoded);
        TEST("H6 decode_response succeeds", ok);
        TEST("H6 status_code round-trips", decoded.status_code == 200);
        TEST("H6 reason round-trips",      decoded.reason      == "OK");
        TEST("H6 header count",            decoded.headers.size() == 2);
        TEST("H6 body round-trips",        decoded.body == orig.body);
    }

    // H7: decode_response with status_code=0 (transport error) decodes correctly
    {
        HttpResponse orig;
        orig.status_code = 0;
        orig.reason      = "transport error";
        orig.body        = {};
        auto payload = encode_response(orig);
        HttpResponse decoded;
        bool ok = decode_response(payload, decoded);
        TEST("H7 status 0 decodes", ok);
        TEST("H7 status 0 code",    decoded.status_code == 0);
        TEST("H7 status 0 reason",  decoded.reason      == "transport error");
    }

    // H8: decode_response with schema_version != 1 → returns false
    {
        HttpResponse orig;
        orig.status_code = 200;
        orig.reason      = "OK";
        auto payload = encode_response(orig);
        if (!payload.empty()) payload[0] = 0x02; // corrupt schema_version
        HttpResponse decoded;
        TEST("H8 wrong schema_version rejected", !decode_response(payload, decoded));
    }

    // H9: encode_request total size > 1 MiB → returns empty vector
    {
        HttpRequest req;
        req.method = "GET";
        req.url    = "http://example.com/";
        req.body.resize(1024 * 1024 + 1, 'X');
        auto payload = encode_request(req);
        TEST("H9 payload > 1 MiB yields empty", payload.empty());
    }
}

// ============================================================================
// main
// ============================================================================
int main() {
    std::cout << "Running test_network...\n";

    test_proxy_config();
    test_request_formatting();
    test_response_parsing();
    test_error_handling();
    test_connect_tunnel();
    test_https_guard();
    test_crlf_injection_guard();
    test_ipc_codec();

    std::cout << "\n" << g_passed << "/" << g_total << " tests passed\n";
    return (g_passed == g_total) ? 0 : 1;
}

