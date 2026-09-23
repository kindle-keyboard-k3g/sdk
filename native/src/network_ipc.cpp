#include "kindle/network_ipc.hpp"
#include "kindle/network.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace kindle::network::ipc {

namespace {

constexpr size_t MAX_PAYLOAD_SIZE = 1024u * 1024u;

void push_u8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

void push_u16be(std::vector<uint8_t>& buf, uint16_t value) {
    buf.push_back(static_cast<uint8_t>(value >> 8));
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void push_u32be(std::vector<uint8_t>& buf, uint32_t value) {
    buf.push_back(static_cast<uint8_t>(value >> 24));
    buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(value & 0xFF));
}

void push_bytes(std::vector<uint8_t>& buf, const std::string& value) {
    buf.insert(buf.end(), value.begin(), value.end());
}

void push_bytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& value) {
    buf.insert(buf.end(), value.begin(), value.end());
}

bool has_room(const std::vector<uint8_t>& buf, size_t pos, size_t length) {
    if (pos > buf.size()) return false;
    return length <= buf.size() - pos;
}

bool read_u8(const std::vector<uint8_t>& buf, size_t& pos, uint8_t& out) {
    if (!has_room(buf, pos, 1)) return false;
    out = buf[pos++];
    return true;
}

bool read_u16be(const std::vector<uint8_t>& buf, size_t& pos, uint16_t& out) {
    if (!has_room(buf, pos, 2)) return false;
    out = static_cast<uint16_t>(
        (static_cast<uint16_t>(buf[pos]) << 8) |
        static_cast<uint16_t>(buf[pos + 1]));
    pos += 2;
    return true;
}

bool read_u32be(const std::vector<uint8_t>& buf, size_t& pos, uint32_t& out) {
    if (!has_room(buf, pos, 4)) return false;
    out = (static_cast<uint32_t>(buf[pos]) << 24) |
          (static_cast<uint32_t>(buf[pos + 1]) << 16) |
          (static_cast<uint32_t>(buf[pos + 2]) << 8) |
          static_cast<uint32_t>(buf[pos + 3]);
    pos += 4;
    return true;
}

bool read_string(const std::vector<uint8_t>& buf, size_t& pos,
                 size_t length, std::string& out) {
    if (!has_room(buf, pos, length)) return false;
    out.assign(reinterpret_cast<const char*>(buf.data() + pos), length);
    pos += length;
    return true;
}

bool read_bytes(const std::vector<uint8_t>& buf, size_t& pos,
                size_t length, std::vector<uint8_t>& out) {
    if (!has_room(buf, pos, length)) return false;
    out.assign(buf.begin() + static_cast<std::ptrdiff_t>(pos),
               buf.begin() + static_cast<std::ptrdiff_t>(pos + length));
    pos += length;
    return true;
}

bool valid_header_length(const std::string& name, const std::string& value) {
    return name.size() <= 65535u && value.size() <= 65535u;
}

} // namespace

std::vector<uint8_t> encode_request(const HttpRequest& req) {
    if (req.method.empty() || req.method.size() > 255u) return {};
    if (req.url.empty() || req.url.size() > 65535u) return {};
    if (req.headers.size() > 65535u) return {};
    if (req.body.size() > MAX_PAYLOAD_SIZE) return {};

    for (const auto& header : req.headers) {
        if (!valid_header_length(header.first, header.second)) return {};
    }

    std::vector<uint8_t> buf;
    push_u8(buf, SCHEMA_VERSION);
    push_u8(buf, static_cast<uint8_t>(req.method.size()));
    push_u16be(buf, static_cast<uint16_t>(req.url.size()));
    push_u16be(buf, static_cast<uint16_t>(req.headers.size()));
    push_u32be(buf, static_cast<uint32_t>(req.body.size()));
    push_bytes(buf, req.method);
    push_bytes(buf, req.url);
    for (const auto& header : req.headers) {
        push_u16be(buf, static_cast<uint16_t>(header.first.size()));
        push_u16be(buf, static_cast<uint16_t>(header.second.size()));
        push_bytes(buf, header.first);
        push_bytes(buf, header.second);
    }
    push_bytes(buf, req.body);

    if (buf.size() > MAX_PAYLOAD_SIZE) return {};
    return buf;
}

bool decode_request(const std::vector<uint8_t>& payload, HttpRequest& out) {
    if (payload.size() > MAX_PAYLOAD_SIZE) return false;

    size_t pos = 0;
    uint8_t schema = 0;
    uint8_t method_len = 0;
    uint16_t url_len = 0;
    uint16_t header_count = 0;
    uint32_t body_len = 0;

    if (!read_u8(payload, pos, schema) || schema != SCHEMA_VERSION) return false;
    if (!read_u8(payload, pos, method_len)) return false;
    if (!read_u16be(payload, pos, url_len)) return false;
    if (!read_u16be(payload, pos, header_count)) return false;
    if (!read_u32be(payload, pos, body_len)) return false;
    if (body_len > MAX_PAYLOAD_SIZE) return false;
    if (method_len == 0 || url_len == 0) return false;
    if (header_count > payload.size()) return false;

    if (!read_string(payload, pos, method_len, out.method)) return false;
    if (!read_string(payload, pos, url_len, out.url)) return false;

    out.headers.clear();
    out.headers.reserve(header_count);
    for (uint16_t index = 0; index < header_count; ++index) {
        uint16_t name_len = 0;
        uint16_t value_len = 0;
        if (!read_u16be(payload, pos, name_len)) return false;
        if (!read_u16be(payload, pos, value_len)) return false;
        std::string name;
        std::string value;
        if (!read_string(payload, pos, name_len, name)) return false;
        if (!read_string(payload, pos, value_len, value)) return false;
        out.headers.emplace_back(std::move(name), std::move(value));
    }

    if (!read_bytes(payload, pos, body_len, out.body)) return false;
    return pos == payload.size();
}

std::vector<uint8_t> encode_response(const HttpResponse& resp) {
    if (resp.status_code < 0 || resp.status_code > 65535) return {};
    std::string reason = resp.reason;
    if (resp.status_code == 0 && reason.empty()) reason = resp.error;
    if (reason.size() > 65535u) return {};
    if (resp.headers.size() > 65535u) return {};
    if (resp.body.size() > MAX_PAYLOAD_SIZE) return {};

    for (const auto& header : resp.headers) {
        if (!valid_header_length(header.first, header.second)) return {};
    }

    std::vector<uint8_t> buf;
    push_u8(buf, SCHEMA_VERSION);
    push_u16be(buf, static_cast<uint16_t>(resp.status_code));
    push_u16be(buf, static_cast<uint16_t>(reason.size()));
    push_u16be(buf, static_cast<uint16_t>(resp.headers.size()));
    push_u32be(buf, static_cast<uint32_t>(resp.body.size()));
    push_bytes(buf, reason);
    for (const auto& header : resp.headers) {
        push_u16be(buf, static_cast<uint16_t>(header.first.size()));
        push_u16be(buf, static_cast<uint16_t>(header.second.size()));
        push_bytes(buf, header.first);
        push_bytes(buf, header.second);
    }
    push_bytes(buf, resp.body);

    if (buf.size() > MAX_PAYLOAD_SIZE) return {};
    return buf;
}

bool decode_response(const std::vector<uint8_t>& payload, HttpResponse& out) {
    if (payload.size() > MAX_PAYLOAD_SIZE) return false;

    size_t pos = 0;
    uint8_t schema = 0;
    uint16_t status = 0;
    uint16_t reason_len = 0;
    uint16_t header_count = 0;
    uint32_t body_len = 0;

    if (!read_u8(payload, pos, schema) || schema != SCHEMA_VERSION) return false;
    if (!read_u16be(payload, pos, status)) return false;
    if (!read_u16be(payload, pos, reason_len)) return false;
    if (!read_u16be(payload, pos, header_count)) return false;
    if (!read_u32be(payload, pos, body_len)) return false;
    if (body_len > MAX_PAYLOAD_SIZE) return false;
    if (header_count > payload.size()) return false;

    out.status_code = static_cast<int>(status);
    out.error.clear();
    if (!read_string(payload, pos, reason_len, out.reason)) return false;

    out.headers.clear();
    out.headers.reserve(header_count);
    for (uint16_t index = 0; index < header_count; ++index) {
        uint16_t name_len = 0;
        uint16_t value_len = 0;
        if (!read_u16be(payload, pos, name_len)) return false;
        if (!read_u16be(payload, pos, value_len)) return false;
        std::string name;
        std::string value;
        if (!read_string(payload, pos, name_len, name)) return false;
        if (!read_string(payload, pos, value_len, value)) return false;
        out.headers.emplace_back(std::move(name), std::move(value));
    }

    if (!read_bytes(payload, pos, body_len, out.body)) return false;
    if (pos != payload.size()) return false;
    if (out.status_code == 0) out.error = out.reason;
    return true;
}

} // namespace kindle::network::ipc
