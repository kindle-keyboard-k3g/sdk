#include "kindle/audio.hpp"
#include "kindle/fake_audio.hpp"
#include "kindle/audio_loader.hpp"
#include "kindle/linux_audio.hpp"
#include "kindle/network.hpp"
#include "../platform/fake/fake_network.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static int g_passed = 0;
static int g_total  = 0;

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

using namespace kindle::audio;

// Helper to construct a valid 16-bit mono PCM WAV in memory
static std::vector<uint8_t> make_test_wav_data(uint32_t sample_rate,
                                               const std::vector<int16_t>& pcm_samples) {
    std::vector<uint8_t> out;
    uint32_t data_bytes = static_cast<uint32_t>(pcm_samples.size() * sizeof(int16_t));
    uint32_t riff_chunk_size = 36 + data_bytes;

    out.reserve(44 + data_bytes);

    // RIFF header
    out.push_back('R'); out.push_back('I'); out.push_back('F'); out.push_back('F');
    out.push_back(static_cast<uint8_t>(riff_chunk_size & 0xFF));
    out.push_back(static_cast<uint8_t>((riff_chunk_size >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((riff_chunk_size >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((riff_chunk_size >> 24) & 0xFF));

    // WAVE format
    out.push_back('W'); out.push_back('A'); out.push_back('V'); out.push_back('E');

    // fmt subchunk
    out.push_back('f'); out.push_back('m'); out.push_back('t'); out.push_back(' ');
    uint32_t subchunk1_size = 16;
    out.push_back(static_cast<uint8_t>(subchunk1_size & 0xFF));
    out.push_back(static_cast<uint8_t>((subchunk1_size >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((subchunk1_size >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((subchunk1_size >> 24) & 0xFF));

    // AudioFormat = 1 (PCM)
    uint16_t audio_format = 1;
    out.push_back(static_cast<uint8_t>(audio_format & 0xFF));
    out.push_back(static_cast<uint8_t>((audio_format >> 8) & 0xFF));

    // NumChannels = 1
    uint16_t num_channels = 1;
    out.push_back(static_cast<uint8_t>(num_channels & 0xFF));
    out.push_back(static_cast<uint8_t>((num_channels >> 8) & 0xFF));

    // SampleRate
    out.push_back(static_cast<uint8_t>(sample_rate & 0xFF));
    out.push_back(static_cast<uint8_t>((sample_rate >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((sample_rate >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((sample_rate >> 24) & 0xFF));

    // ByteRate = SampleRate * NumChannels * BitsPerSample/8
    uint32_t byte_rate = sample_rate * num_channels * 2;
    out.push_back(static_cast<uint8_t>(byte_rate & 0xFF));
    out.push_back(static_cast<uint8_t>((byte_rate >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((byte_rate >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((byte_rate >> 24) & 0xFF));

    // BlockAlign = NumChannels * BitsPerSample/8 = 2
    uint16_t block_align = 2;
    out.push_back(static_cast<uint8_t>(block_align & 0xFF));
    out.push_back(static_cast<uint8_t>((block_align >> 8) & 0xFF));

    // BitsPerSample = 16
    uint16_t bits_per_sample = 16;
    out.push_back(static_cast<uint8_t>(bits_per_sample & 0xFF));
    out.push_back(static_cast<uint8_t>((bits_per_sample >> 8) & 0xFF));

    // data subchunk
    out.push_back('d'); out.push_back('a'); out.push_back('t'); out.push_back('a');
    out.push_back(static_cast<uint8_t>(data_bytes & 0xFF));
    out.push_back(static_cast<uint8_t>((data_bytes >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((data_bytes >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((data_bytes >> 24) & 0xFF));

    // PCM samples
    for (int16_t s : pcm_samples) {
        out.push_back(static_cast<uint8_t>(s & 0xFF));
        out.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    }

    return out;
}

void test_make_beep() {
    std::cout << "--- Testing make_beep ---\n";

    AudioSpec spec;
    spec.sample_rate = 22050;
    spec.channels    = 1;
    spec.format      = SampleFormat::S16LE;

    // 100ms at 22050 Hz = 2205 samples
    AudioClip clip = make_beep(440, 100, spec);
    TEST("make_beep sample count", clip.samples.size() == 2205);
    TEST("make_beep spec preserved", clip.spec.sample_rate == 22050 && clip.spec.channels == 1);

    // Verify samples are non-zero (non-silent)
    bool has_nonzero = false;
    for (int16_t s : clip.samples) {
        if (s != 0) {
            has_nonzero = true;
            break;
        }
    }
    TEST("make_beep generates audible samples", has_nonzero);

    // Guard: duration capped at 5000ms
    AudioClip long_clip = make_beep(440, 10000, spec);
    TEST("make_beep duration capped at 5s", long_clip.samples.size() == 22050 * 5);

    // Guard: invalid frequency
    TEST_THROWS("make_beep rejects freq < 20", make_beep(10, 100, spec));
    TEST_THROWS("make_beep rejects freq > 20000", make_beep(25000, 100, spec));
}

void test_wav_loader() {
    std::cout << "--- Testing load_wav and parse_wav ---\n";

    std::vector<int16_t> sample_data = {0, 1000, 2000, 3000, -1000, -2000, -3000, 0};
    std::vector<uint8_t> wav_bytes = make_test_wav_data(22050, sample_data);

    const std::string tmp_file = "/tmp/test_kindle_audio.wav";
    {
        std::ofstream ofs(tmp_file, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(wav_bytes.data()), wav_bytes.size());
    }

    AudioClip loaded = load_wav(tmp_file);
    TEST("load_wav sample count matches", loaded.samples.size() == sample_data.size());
    TEST("load_wav sample rate matches", loaded.spec.sample_rate == 22050);
    TEST("load_wav channels matches mono", loaded.spec.channels == 1);
    TEST("load_wav sample content matches", loaded.samples == sample_data);

    // Test rejection of non-existent file
    TEST_THROWS("load_wav missing file throws", load_wav("/tmp/non_existent_file_kindle.wav"));

    // Test rejection of corrupt WAV (truncated)
    const std::string corrupt_file = "/tmp/test_kindle_corrupt.wav";
    {
        std::ofstream ofs(corrupt_file, std::ios::binary);
        ofs.write("RIFF1234WAVEfmt ", 16);
    }
    TEST_THROWS("load_wav corrupt file throws", load_wav(corrupt_file));

    // Test rejection of file > 1 MB
    const std::string oversized_file = "/tmp/test_kindle_oversized.wav";
    {
        std::ofstream ofs(oversized_file, std::ios::binary);
        std::vector<char> dummy(1024 * 1024 + 10, 'A');
        ofs.write(dummy.data(), dummy.size());
    }
    TEST_THROWS("load_wav > 1 MB throws", load_wav(oversized_file));
}

void test_fake_audio_player() {
    std::cout << "--- Testing FakeAudioPlayer ---\n";

    FakeAudioPlayer player;
    TEST("FakeAudioPlayer default closed", !player.is_open());
    TEST("FakeAudioPlayer default volume 70", player.get_volume() == 70);

    AudioSpec spec{22050, 1, SampleFormat::S16LE};
    TEST("FakeAudioPlayer open succeeds", player.open(spec));
    TEST("FakeAudioPlayer is_open", player.is_open());

    AudioClip clip = make_beep(880, 50, spec);
    player.play(clip);
    TEST("FakeAudioPlayer play count is 1", player.get_play_count() == 1);
    TEST("FakeAudioPlayer is_playing", player.is_playing());
    TEST("FakeAudioPlayer last_clip sample count matches", player.get_last_clip().samples.size() == clip.samples.size());

    player.stop();
    TEST("FakeAudioPlayer stop count is 1", player.get_stop_count() == 1);
    TEST("FakeAudioPlayer is not playing after stop", !player.is_playing());

    // Volume control
    player.set_volume(85);
    TEST("FakeAudioPlayer volume set/get", player.get_volume() == 85);

    player.close();
    TEST("FakeAudioPlayer closed", !player.is_open());
    TEST("FakeAudioPlayer is not playing after close", !player.is_playing());
}

void test_fetch_audio() {
    std::cout << "--- Testing fetch_audio ---\n";

    auto backend = std::make_shared<FakeSocketBackend>();
    kindle::network::ProxyConfig proxy;
    kindle::network::HttpClient client(proxy, backend);

    std::vector<int16_t> sample_data = {500, 1500, 2500, -500};
    std::vector<uint8_t> wav_bytes = make_test_wav_data(11025, sample_data);

    std::string http_body(wav_bytes.begin(), wav_bytes.end());
    std::string http_response = "HTTP/1.1 200 OK\r\nContent-Length: " +
                                std::to_string(http_body.size()) + "\r\n\r\n" + http_body;
    backend->enqueue_response(http_response);

    AudioClip clip = fetch_audio(client, "http://kindle.local/audio/ping.wav");
    TEST("fetch_audio succeeds", clip.samples.size() == sample_data.size());
    TEST("fetch_audio sample rate matches", clip.spec.sample_rate == 11025);
    TEST("fetch_audio samples match", clip.samples == sample_data);

    // Test HTTP 404 failure throws
    backend->enqueue_response("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
    TEST_THROWS("fetch_audio 404 throws", fetch_audio(client, "http://kindle.local/audio/missing.wav"));

    // Test response > 1 MB throws
    std::string big_body(1024 * 1024 + 64, '\0');
    std::string big_resp = "HTTP/1.1 200 OK\r\nContent-Length: " +
                           std::to_string(big_body.size()) + "\r\n\r\n" + big_body;
    backend->enqueue_response(big_resp);
    TEST_THROWS("fetch_audio > 1MB throws", fetch_audio(client, "http://kindle.local/audio/huge.wav"));
}

void test_linux_oss_audio_player() {
    std::cout << "--- Testing LinuxOssAudioPlayer ---\n";

    LinuxOssAudioPlayer player("/dev/non_existent_dsp_device", "/dev/non_existent_mixer_device");
    AudioSpec spec{22050, 1, SampleFormat::S16LE};

    TEST("LinuxOssAudioPlayer returns false on missing device", !player.open(spec));
    TEST("LinuxOssAudioPlayer is not playing when unopened", !player.is_playing());

    // Operations on unopened player must be safe no-ops
    AudioClip clip = make_beep(440, 50, spec);
    player.play(clip);
    TEST("play on unopened player does not change playing state", !player.is_playing());

    player.stop();
    TEST("stop on unopened player is safe", !player.is_playing());

    player.set_volume(42);
    TEST("set/get volume on unopened player falls back to software volume", player.get_volume() == 42);

    player.set_volume(150); // should clamp to 100
    TEST("volume clamps to 100", player.get_volume() == 100);

    player.close();
    TEST("close on unopened player is idempotent", !player.is_playing());
}

int main() {
    std::cout << "Running test_audio...\n";

    test_make_beep();
    test_wav_loader();
    test_fake_audio_player();
    test_fetch_audio();
    test_linux_oss_audio_player();

    std::cout << "\n==============================\n";
    std::cout << "Results: " << g_passed << " / " << g_total << " passed\n";
    std::cout << "==============================\n";

    return (g_passed == g_total) ? 0 : 1;
}
