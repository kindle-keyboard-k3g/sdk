#pragma once
#include "kindle/audio.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace kindle::audio {

/**
 * Hardware OSS /dev/dsp and /dev/mixer audio player for Kindle devices (K3/K3G/DX).
 * Utilizes a single low-overhead background thread for non-blocking asynchronous playback
 * and controls hardware gain via Linux OSS mixer ioctls.
 */
class LinuxOssAudioPlayer : public AudioPlayer {
public:
    explicit LinuxOssAudioPlayer(std::string dsp_path = "/dev/dsp",
                                 std::string mixer_path = "/dev/mixer");
    ~LinuxOssAudioPlayer() override;

    bool open(const AudioSpec& spec) override;
    void close() override;
    void play(const AudioClip& clip) override;
    void stop() override;
    [[nodiscard]] bool is_playing() const override;
    void set_volume(uint8_t level) override;
    [[nodiscard]] uint8_t get_volume() const override;

private:
    void worker_loop();

    std::string dsp_path_;
    std::string mixer_path_;
    int dsp_fd_{-1};
    int mixer_fd_{-1};
    AudioSpec spec_;

    std::thread worker_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::optional<AudioClip> pending_clip_;
    std::atomic<bool> quit_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> is_playing_{false};
    uint8_t volume_{70};
};

} // namespace kindle::audio
