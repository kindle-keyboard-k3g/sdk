#include "kindle/audio.hpp"
#include "kindle/audio_loader.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace kindle::audio {

constexpr size_t MAX_WAV_BYTES = 1024 * 1024; // 1 MB hardware safety budget

AudioClip make_beep(uint32_t freq_hz, uint32_t duration_ms, const AudioSpec& spec) {
    if (freq_hz < 20 || freq_hz > 20000) {
        throw std::invalid_argument("Frequency out of valid range (20 Hz - 20000 Hz)");
    }

    uint32_t effective_duration_ms = std::min(duration_ms, 5000u);
    uint32_t sample_rate = spec.sample_rate > 0 ? spec.sample_rate : 22050;
    size_t num_samples = static_cast<size_t>(sample_rate) * effective_duration_ms / 1000;

    AudioClip clip;
    clip.spec = spec;
    clip.spec.sample_rate = sample_rate;
    clip.spec.channels = 1;
    clip.spec.format = SampleFormat::S16LE;
    clip.samples.resize(num_samples);

    constexpr float PI2 = 6.28318530717958647692f;
    float step = (PI2 * static_cast<float>(freq_hz)) / static_cast<float>(sample_rate);

    for (size_t i = 0; i < num_samples; ++i) {
        float val = std::sin(step * static_cast<float>(i));
        // Soft clipping / scaling for 16-bit signed PCM
        clip.samples[i] = static_cast<int16_t>(val * 32767.0f);
    }

    return clip;
}

AudioClip parse_wav(const uint8_t* data, size_t size) {
    if (size > MAX_WAV_BYTES) {
        throw std::runtime_error("WAV size exceeds 1 MB limit");
    }
    if (size < 44) {
        throw std::runtime_error("WAV data too small for RIFF header");
    }

    // Check RIFF and WAVE magic signatures
    if (std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0) {
        throw std::runtime_error("Invalid WAV file: missing RIFF/WAVE header");
    }

    // Traverse subchunks
    size_t offset = 12;
    bool found_fmt = false;
    bool found_data = false;

    uint16_t audio_format = 0;
    uint16_t num_channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    const uint8_t* pcm_data = nullptr;
    uint32_t pcm_data_bytes = 0;

    while (offset + 8 <= size) {
        char chunk_id[5] = {0};
        std::memcpy(chunk_id, data + offset, 4);

        uint32_t chunk_size = 0;
        std::memcpy(&chunk_size, data + offset + 4, 4);

        size_t next_offset = offset + 8 + chunk_size;

        if (std::strcmp(chunk_id, "fmt ") == 0) {
            if (chunk_size < 16 || offset + 8 + 16 > size) {
                throw std::runtime_error("Corrupted fmt chunk in WAV file");
            }
            std::memcpy(&audio_format, data + offset + 8, 2);
            std::memcpy(&num_channels, data + offset + 10, 2);
            std::memcpy(&sample_rate, data + offset + 12, 4);
            std::memcpy(&bits_per_sample, data + offset + 22, 2);
            found_fmt = true;
        } else if (std::strcmp(chunk_id, "data") == 0) {
            if (offset + 8 + chunk_size > size) {
                throw std::runtime_error("Corrupted data chunk in WAV file");
            }
            pcm_data = data + offset + 8;
            pcm_data_bytes = chunk_size;
            found_data = true;
            break; // Finished parsing mandatory chunks
        }

        offset = next_offset;
    }

    if (!found_fmt || !found_data || pcm_data == nullptr) {
        throw std::runtime_error("Incomplete WAV: missing fmt or data chunk");
    }

    if (audio_format != 1) { // 1 = Linear PCM
        throw std::runtime_error("Unsupported WAV format: only linear PCM is supported");
    }
    if (bits_per_sample != 16) {
        throw std::runtime_error("Unsupported bit depth: only 16-bit PCM is supported");
    }

    AudioClip clip;
    clip.spec.sample_rate = sample_rate;
    clip.spec.channels = static_cast<uint8_t>(num_channels);
    clip.spec.format = SampleFormat::S16LE;

    size_t sample_count = pcm_data_bytes / sizeof(int16_t);
    clip.samples.resize(sample_count);
    std::memcpy(clip.samples.data(), pcm_data, sample_count * sizeof(int16_t));

    return clip;
}

AudioClip load_wav(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open WAV file: " + path);
    }

    std::streamsize file_size = file.tellg();
    if (file_size < 0 || static_cast<size_t>(file_size) > MAX_WAV_BYTES) {
        throw std::runtime_error("WAV file exceeds 1 MB limit: " + path);
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
        throw std::runtime_error("Failed to read WAV file: " + path);
    }

    return parse_wav(buffer.data(), buffer.size());
}

} // namespace kindle::audio
