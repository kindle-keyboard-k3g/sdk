#pragma once
#include "kindle/audio.hpp"

namespace kindle::audio {

/**
 * Deterministic in-memory fake audio player for host tests and CI.
 */
class FakeAudioPlayer : public AudioPlayer {
public:
    bool open(const AudioSpec& spec) override {
        spec_ = spec;
        is_open_ = true;
        return true;
    }

    void close() override {
        is_open_ = false;
        is_playing_ = false;
    }

    void play(const AudioClip& clip) override {
        last_clip_ = clip;
        ++play_count_;
        is_playing_ = true;
    }

    void stop() override {
        is_playing_ = false;
        ++stop_count_;
    }

    [[nodiscard]] bool is_playing() const override {
        return is_playing_;
    }

    void set_volume(uint8_t level) override {
        volume_ = level;
    }

    [[nodiscard]] uint8_t get_volume() const override {
        return volume_;
    }

    // Test inspection helpers
    [[nodiscard]] uint32_t get_play_count() const noexcept { return play_count_; }
    [[nodiscard]] uint32_t get_stop_count() const noexcept { return stop_count_; }
    [[nodiscard]] const AudioClip& get_last_clip() const noexcept { return last_clip_; }
    [[nodiscard]] bool is_open() const noexcept { return is_open_; }
    [[nodiscard]] const AudioSpec& get_spec() const noexcept { return spec_; }

private:
    AudioSpec spec_;
    AudioClip last_clip_;
    uint32_t  play_count_{0};
    uint32_t  stop_count_{0};
    uint8_t   volume_{70};
    bool      is_open_{false};
    bool      is_playing_{false};
};

} // namespace kindle::audio
