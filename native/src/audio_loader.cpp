#include "kindle/audio_loader.hpp"
#include <stdexcept>
#include <string>

namespace kindle::audio {

constexpr size_t MAX_FETCH_BYTES = 1024 * 1024; // 1 MB limit for cellular 3G & RAM

AudioClip fetch_audio(network::HttpClient& client, const std::string& url) {
    auto response = client.get(url);
    if (!response.ok()) {
        throw std::runtime_error("HTTP audio fetch failed (" + std::to_string(response.status_code) +
                                 "): " + response.error);
    }

    if (response.body.size() > MAX_FETCH_BYTES) {
        throw std::runtime_error("Fetched audio exceeds 1 MB limit (" +
                                 std::to_string(response.body.size()) + " bytes)");
    }

    return parse_wav(response.body.data(), response.body.size());
}

} // namespace kindle::audio
