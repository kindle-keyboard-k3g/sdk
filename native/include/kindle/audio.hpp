#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kindle::audio {

/**
 * Supported sample format encodings.
 */
enum class SampleFormat {
    S16LE  ///< Signed 16-bit Little-Endian PCM
};

/**
 * Audio stream specification.
 */
struct AudioSpec {
    uint32_t sample_rate{22050};                ///< Samples per second (K3: 8000, 11025, 22050, 44100)
    uint8_t  channels{1};                       ///< Number of channels (1 = mono, 2 = stereo)
    SampleFormat format{SampleFormat::S16LE};   ///< Sample encoding format
};

/**
 * In-memory raw PCM audio buffer.
 */
struct AudioClip {
    std::vector<int16_t> samples;  ///< Interleaved signed 16-bit PCM samples
    AudioSpec spec;                ///< Audio stream specification
};

/**
 * Synthesizes a pure sine-wave beep tone.
 *
 * @param freq_hz Frequency in Hertz (20 to 20000 Hz).
 * @param duration_ms Duration in milliseconds (capped at 5000 ms).
 * @param spec Audio specification (sample rate and channels).
 * @return Synthesized AudioClip.
 * @throws std::invalid_argument If frequency is out of valid range.
 */
AudioClip make_beep(uint32_t freq_hz, uint32_t duration_ms, const AudioSpec& spec = {});

/**
 * Loads a RIFF/PCM 16-bit mono WAV file from the filesystem.
 *
 * @param path Filepath to the WAV file.
 * @return Decoded AudioClip.
 * @throws std::runtime_error If file cannot be read, format is unsupported, or size > 1 MB.
 */
AudioClip load_wav(const std::string& path);

/**
 * Abstract interface for audio playback and volume control on Kindle devices.
 */
class AudioPlayer {
public:
    virtual ~AudioPlayer() = default;

    /**
     * Initializes the audio device node with the requested specification.
     *
     * @param spec Desired sample rate, channel count, and format.
     * @return true if opened successfully, false on failure (e.g. device missing).
     */
    virtual bool open(const AudioSpec& spec) = 0;

    /**
     * Stops any active playback, joins worker threads, and closes device handles.
     */
    virtual void close() = 0;

    /**
     * Asynchronously queues an audio clip for playback (non-blocking).
     * Replaces any pending queued clip.
     *
     * @param clip PCM audio clip to play.
     */
    virtual void play(const AudioClip& clip) = 0;

    /**
     * Aborts any currently playing audio clip immediately.
     */
    virtual void stop() = 0;

    /**
     * Checks if audio is currently playing.
     *
     * @return true if actively outputting audio, false otherwise.
     */
    [[nodiscard]] virtual bool is_playing() const = 0;

    /**
     * Sets hardware/software output volume level.
     *
     * @param level Volume percentage from 0 (muted) to 100 (maximum).
     */
    virtual void set_volume(uint8_t level) = 0;

    /**
     * Queries the current volume level.
     *
     * @return Volume percentage from 0 to 100.
     */
    [[nodiscard]] virtual uint8_t get_volume() const = 0;
};

} // namespace kindle::audio
