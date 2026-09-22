#include "kindle/network_ipc.hpp"
#include "kindle/network.hpp"
#include <cstring>
#include <stdexcept>

namespace kindle::network::ipc {

// ---------------------------------------------------------------------------
// Encoding helpers
// ---------------------------------------------------------------------------

static void push_u8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

static void push_u16be(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void push_u32be(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v >> 24));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void push_bytes(std::vector<uint8_t>& buf, const std::string& s) {
    buf.insert(buf.end(), s.begin(), s.end());
}

static void push_bytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& v) {
    buf.insert(buf.end(), v.begin(), v.end());
}

// ---------------------------------------------------------------------------
// Decoding helpers
// ---------------------------------------------------------------------------

static bool read_u8(const std::vector<uint8_t>& buf, size_t& pos, uint8_t& out) {
    if (pos >= buf.size()) return false;
    out = buf[pos++];
    return true;
}

static bool read_u16be(const std::vector<uint8_t>& buf, size_t& pos, uint16_t& out) {
    if (pos + 2 > buf.size()) return false;
    out = static_cast<uint16_t>(
        (static_cast<uint16_t>(buf[pos]) << 8) |
         static_cast<uint16_t>(buf[pos + 1]));
    pos += 2;
    return true;
}

static bool read_u32be(const std::vector<uint8_t>& buf, size_t& pos, uint32_t& out) {
    if (pos + 4 > buf.size()) return false;
    out = (static_cast<uint32_t>(buf[pos])     << 24) |
          (static_cast<uint32_t>(buf[pos + 1]) << 16) |
          (static_cast<uint32_t>(buf[pos + 2]) <<  8) |
           static_cast<uint32_t>(buf[pos + 3]);
    pos += 4;
    return true;
}

static bool read_string(const std::vector<uint8_t>& buf, size_t& pos,
                        size_t len, std::string& out) {
    if (pos + len > buf.size()) return false;
    out.assign(reinterpret_cast<const char*>(buf.data() + pos), len);
    pos += len;
    return true;
}

static bool read_bytes(const std::vector<uint8_t>& buf, size_t& pos,
                       size_t len, std::vector<uint8_t>& out) {
    if (pos + len > buf.size()) return false;
    out.assign(buf.begin() + static_cast<std::ptrdiff_t>(pos),
               buf.begin() + static_cast<std::ptrdiff_t>(pos + len));
    pos += len;
    return true;
}

// ---------------------------------------------------------------------------
// encode_request
// Request fixed header: u8 schema_version + u8 method_length + u16 url_length
//                       + u16 header_count + u32 body_length  = 10 bytes
// ---------------------------------------------------------------------------

std::vector<uint8_t> encode_request(const HttpRequest& req) {
    if (req.method.size() > 255)   return {};
    if (req.url.size()    > 65535) return {};
    if (req.headers.size() > 65535) return {};

    std::vector<uint8_t> buf;

    // Fixed header (10 bytes)
    push_u8(buf,  SCHEMA_VERSION);
    push_u8(buf,  static_cast<uint8_t>(req.method.size()));
    push_u16be(buf, static_cast<uint16_t>(req.url.size()));
    push_u16be(buf, static_cast<uint16_t>(req.headers.size()));
    push_u32be(buf, static_cast<uint32_t>(req.body.size()));

    // Variable fields
    push_bytes(buf, req.method);
    push_bytes(buf, req.url);
    for (const auto& h : req.headers) {
        if (h.first.size() > 65535 || h.second.size() > 65535) return {};
        push_u16be(buf, static_cast<uint16_t>(h.first.size()));
        push_u16be(buf, static_cast<uint16_t>(h.second.size()));
        push_bytes(buf, h.first);
        push_bytes(buf, h.second);
    }
    push_bytes(buf, req.body);

    // 1 MiB payload limit (matches MAX_PAYLOAD_SIZE in ipc.hpp)
    constexpr size_t MAX_PAYLOAD_SIZE = 1024 * 1024;
    if (buf.size() > MAX_PAYLOAD_SIZE) return {};

    return buf;
}

// ---------------------------------------------------------------------------
// decode_request
// ---------------------------------------------------------------------------

bool decode_request(const std::vector<uint8_t>& payload, HttpRequest& out) {
    size_t pos = 0;

    uint8_t  schema = 0;
    uint8_t  method_len = 0;
    uint16_t url_len = 0;
    uint16_t header_count = 0;
    uint32_t body_len = 0;

    if (!read_u8(payload, pos, schema))           return false;
    if (schema != SCHEMA_VERSION)                 return false;
    if (!read_u8(payload, pos, method_len))       return false;
    if (!read_u16be(payload, pos, url_len))       return false;
    if (!read_u16be(payload, pos, header_count))  return false;
    if (!read_u32be(payload, pos, body_len))      return false;

    // Read method and url
    if (!read_string(payload, pos, method_len, out.method)) return false;
    if (!read_string(payload, pos, url_len,    out.url))    return false;

    // Read headers
    out.headers.clear();
    out.headers.reserve(header_count);
    for (uint16_t i = 0; i < header_count; ++i) {
        uint16_t name_len = 0, value_len = 0;
        if (!read_u16be(payload, pos, name_len))  return false;
        if (!read_u16be(payload, pos, value_len)) return false;
        std::string name, value;
        if (!read_string(payload, pos, name_len,  name))  return false;
        if (!read_string(payload, pos, value_len, value)) return false;
        out.headers.emplace_back(std::move(name), std::move(value));
    }

    // Read body
    if (!read_bytes(payload, pos, body_len, out.body)) return false;

    // Ensure we consumed exactly the payload
    return pos == payload.size();
}

// ---------------------------------------------------------------------------
// encode_response
// Response fixed header: u8 schema_version + u16 status_code + u16 reason_length
//                        + u16 header_count + u32 body_length  = 11 bytes
// ---------------------------------------------------------------------------

std::vector<uint8_t> encode_response(const HttpResponse& resp) {
    if (resp.reason.size()  > 65535) return {};
    if (resp.headers.size() > 65535) return {};

    std::vector<uint8_t> buf;

    // Fixed header (11 bytes)
    push_u8(buf,  SCHEMA_VERSION);
    push_u16be(buf, static_cast<uint16_t>(resp.status_code));
    push_u16be(buf, static_cast<uint16_t>(resp.reason.size()));
    push_u16be(buf, static_cast<uint16_t>(resp.headers.size()));
    push_u32be(buf, static_cast<uint32_t>(resp.body.size()));

    // Variable fields
    push_bytes(buf, resp.reason);
    for (const auto& h : resp.headers) {
        if (h.first.size() > 65535 || h.second.size() > 65535) return {};
        push_u16be(buf, static_cast<uint16_t>(h.first.size()));
        push_u16be(buf, static_cast<uint16_t>(h.second.size()));
        push_bytes(buf, h.first);
        push_bytes(buf, h.second);
    }
    push_bytes(buf, resp.body);

    constexpr size_t MAX_PAYLOAD_SIZE = 1024 * 1024;
    if (buf.size() > MAX_PAYLOAD_SIZE) return {};

    return buf;
}

// ---------------------------------------------------------------------------
// decode_response
// ---------------------------------------------------------------------------

bool decode_response(const std::vector<uint8_t>& payload, HttpResponse& out) {
    size_t pos = 0;

    uint8_t  schema = 0;
    uint16_t status = 0;
    uint16_t reason_len = 0;
    uint16_t header_count = 0;
    uint32_t body_len = 0;

    if (!read_u8(payload, pos, schema))           return false;
    if (schema != SCHEMA_VERSION)                 return false;
    if (!read_u16be(payload, pos, status))        return false;
    if (!read_u16be(payload, pos, reason_len))    return false;
    if (!read_u16be(payload, pos, header_count))  return false;
    if (!read_u32be(payload, pos, body_len))      return false;

    out.status_code = static_cast<int>(status);

    if (!read_string(payload, pos, reason_len, out.reason)) return false;

    out.headers.clear();
    out.headers.reserve(header_count);
    for (uint16_t i = 0; i < header_count; ++i) {
        uint16_t name_len = 0, value_len = 0;
        if (!read_u16be(payload, pos, name_len))  return false;
        if (!read_u16be(payload, pos, value_len)) return false;
        std::string name, value;
        if (!read_string(payload, pos, name_len,  name))  return false;
        if (!read_string(payload, pos, value_len, value)) return false;
        out.headers.emplace_back(std::move(name), std::move(value));
    }

    if (!read_bytes(payload, pos, body_len, out.body)) return false;

    return pos == payload.size();
}

} // namespace kindle::network::ipc
