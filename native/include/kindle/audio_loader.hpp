#pragma once
#include "kindle/audio.hpp"
#include "kindle/network.hpp"
#include <string>

namespace kindle::audio {

/**
 * Parses raw RIFF/WAVE PCM audio data from memory.
 *
 * @param data Pointer to WAV byte stream.
 * @param size Total byte size of data (must not exceed 1 MB).
 * @return Decoded AudioClip.
 * @throws std::runtime_error If header is invalid, not PCM format, or size exceeds 1 MB.
 */
AudioClip parse_wav(const uint8_t* data, size_t size);

/**
 * Downloads a WAV audio file over HTTP via Whispernet HttpClient and decodes it.
 *
 * @param client Initialized HttpClient instance.
 * @param url Destination HTTP audio URL.
 * @return Decoded AudioClip.
 * @throws std::runtime_error On transport error, HTTP status >= 300, or invalid WAV content.
 */
AudioClip fetch_audio(network::HttpClient& client, const std::string& url);

} // namespace kindle::audio
