#pragma once
#include "kindle/network.hpp"
#include <cstdint>
#include <vector>

namespace kindle::network::ipc {

/** Schema version embedded in every payload. */
constexpr uint8_t SCHEMA_VERSION = 1;

/**
 * Encode an HttpRequest into a KIND IPC payload byte vector.
 * Returns an empty vector on error: method > 255 bytes, url > 65535 bytes,
 * header count > 65535, or total payload > MAX_PAYLOAD_SIZE (1 MiB).
 * All multi-byte integers are unsigned big-endian.
 */
std::vector<uint8_t> encode_request(const HttpRequest& req);

/**
 * Decode a KIND IPC payload into an HttpRequest.
 * Returns false if the payload is malformed, truncated, or has an unknown
 * schema_version. Returns true and populates out on success.
 */
bool decode_request(const std::vector<uint8_t>& payload, HttpRequest& out);

/**
 * Encode an HttpResponse into a KIND IPC payload byte vector.
 * Returns an empty vector on error (same conditions as encode_request).
 */
std::vector<uint8_t> encode_response(const HttpResponse& resp);

/**
 * Decode a KIND IPC payload into an HttpResponse.
 * Returns false on malformed input; true and populates out on success.
 */
bool decode_response(const std::vector<uint8_t>& payload, HttpResponse& out);

} // namespace kindle::network::ipc
