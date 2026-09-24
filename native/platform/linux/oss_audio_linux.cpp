#include "kindle/linux_audio.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <algorithm>
#include <cstring>

namespace kindle::audio {

LinuxOssAudioPlayer::LinuxOssAudioPlayer(std::string dsp_path, std::string mixer_path)
    : dsp_path_(std::move(dsp_path)),
      mixer_path_(std::move(mixer_path)) {}

LinuxOssAudioPlayer::~LinuxOssAudioPlayer() {
    close();
}

bool LinuxOssAudioPlayer::open(const AudioSpec& spec) {
    close();

    dsp_fd_ = ::open(dsp_path_.c_str(), O_WRONLY);
    if (dsp_fd_ < 0) {
        return false;
    }

    int format = AFMT_S16_LE;
    if (::ioctl(dsp_fd_, SNDCTL_DSP_SETFMT, &format) < 0) {
        ::close(dsp_fd_);
        dsp_fd_ = -1;
        return false;
    }

    int channels = static_cast<int>(spec.channels);
    if (::ioctl(dsp_fd_, SNDCTL_DSP_CHANNELS, &channels) < 0) {
        ::close(dsp_fd_);
        dsp_fd_ = -1;
        return false;
    }

    int speed = static_cast<int>(spec.sample_rate);
    if (::ioctl(dsp_fd_, SNDCTL_DSP_SPEED, &speed) < 0) {
        ::close(dsp_fd_);
        dsp_fd_ = -1;
        return false;
    }

    // Set buffer fragment to 16 fragments of 512 bytes (256 samples @ 16-bit)
    // 0x00100009 = 16 fragments of 2^9 (512) bytes
    int frag = 0x00100009;
    (void)::ioctl(dsp_fd_, SNDCTL_DSP_SETFRAGMENT, &frag);

    spec_ = spec;

    // Attempt to open mixer device for hardware volume control
    mixer_fd_ = ::open(mixer_path_.c_str(), O_RDWR);
    if (mixer_fd_ >= 0) {
        int packed = 0;
        if (::ioctl(mixer_fd_, SOUND_MIXER_READ_PCM, &packed) == 0) {
            volume_ = static_cast<uint8_t>(packed & 0xFF);
        }
    }

    quit_ = false;
    stop_requested_ = false;
    is_playing_ = false;
    worker_thread_ = std::thread(&LinuxOssAudioPlayer::worker_loop, this);

    return true;
}

void LinuxOssAudioPlayer::close() {
    quit_ = true;
    stop_requested_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_clip_.reset();
    }
    cv_.notify_all();

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    if (dsp_fd_ >= 0) {
        ::close(dsp_fd_);
        dsp_fd_ = -1;
    }
    if (mixer_fd_ >= 0) {
        ::close(mixer_fd_);
        mixer_fd_ = -1;
    }

    is_playing_ = false;
}

void LinuxOssAudioPlayer::play(const AudioClip& clip) {
    if (dsp_fd_ < 0) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_clip_ = clip;
        stop_requested_ = false;
    }
    cv_.notify_one();
}

void LinuxOssAudioPlayer::stop() {
    stop_requested_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_clip_.reset();
    }
}

bool LinuxOssAudioPlayer::is_playing() const {
    return is_playing_.load();
}

void LinuxOssAudioPlayer::set_volume(uint8_t level) {
    uint8_t clamped = std::min(level, static_cast<uint8_t>(100));
    volume_ = clamped;

    if (mixer_fd_ >= 0) {
        int packed = (static_cast<int>(clamped) << 8) | static_cast<int>(clamped);
        (void)::ioctl(mixer_fd_, SOUND_MIXER_WRITE_PCM, &packed);
    }
}

uint8_t LinuxOssAudioPlayer::get_volume() const {
    if (mixer_fd_ >= 0) {
        int packed = 0;
        if (::ioctl(mixer_fd_, SOUND_MIXER_READ_PCM, &packed) == 0) {
            return static_cast<uint8_t>(packed & 0xFF);
        }
    }
    return volume_;
}

void LinuxOssAudioPlayer::worker_loop() {
    constexpr size_t CHUNK_SAMPLES = 256;

    while (!quit_) {
        AudioClip current;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return quit_.load() || pending_clip_.has_value();
            });

            if (quit_) {
                break;
            }

            current = std::move(*pending_clip_);
            pending_clip_.reset();
        }

        is_playing_ = true;

        const int16_t* ptr = current.samples.data();
        size_t remaining_samples = current.samples.size();

        while (remaining_samples > 0 && !stop_requested_ && !quit_) {
            size_t chunk = std::min(remaining_samples, CHUNK_SAMPLES);
            size_t bytes_to_write = chunk * sizeof(int16_t);

            ssize_t written = ::write(dsp_fd_, ptr, bytes_to_write);
            if (written <= 0) {
                break;
            }

            size_t written_samples = static_cast<size_t>(written) / sizeof(int16_t);
            ptr += written_samples;
            remaining_samples -= written_samples;
        }

        is_playing_ = false;
    }
}

} // namespace kindle::audio
