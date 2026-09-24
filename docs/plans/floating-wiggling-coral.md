# Plan: Audio Subsystem for kindle-sdk

## Context

The kindle-sdk has no audio support. Three consumer apps need it:

- **dino** — game sound FX (jump, die, milestone) synthesized as tones. Must be non-blocking; one frame = ~80ms.
- **papergram** — notification ping on incoming messages. Small WAV embedded or fetched from network. Non-blocking.
- **kindle-myts** — terminal BEL (`\a`, 0x07). Short generated beep, currently unhandled by AnsiParser.

Kindle 3G hardware has real audio output: i.MX353 SSI + WM8750 codec, accessible via OSS `/dev/dsp` and mixer via `/dev/mixer`. Linux 2.6.26 with EGLIBC 2.11. Static linking required → OSS (`<sys/soundcard.h>`) is chosen over libasound to avoid ~500 KB library dependency. K3 RAM budget ≤ 30 MB for user apps.

Design: Approach B — fire-and-forget async player with dedicated worker thread. `play()` is non-blocking for callers; a background thread writes PCM to `/dev/dsp`. Volume control via `/dev/mixer` exposed through the interface, wired to `VolumeUp`/`VolumeDown` key events in each app.

Network audio: buffer-then-play. `fetch_audio()` downloads a WAV via `network::HttpClient`, enforces a 1 MB cap, returns an `AudioClip` ready for `play()`.

---

## Files to Create

### `native/include/kindle/audio.hpp`
Abstract interface and data types. No platform headers.

```
namespace kindle::audio {

enum class SampleFormat { S16LE };

struct AudioSpec {
    uint32_t sample_rate = 22050;  // K3 supports 8000, 11025, 22050, 44100
    uint8_t  channels    = 1;      // mono (K3 speaker is mono)
    SampleFormat format  = SampleFormat::S16LE;
};

struct AudioClip {
    std::vector<int16_t> samples;  // interleaved PCM
    AudioSpec spec;
};

// Synthesize a pure sine-tone clip (no file I/O)
// Caps duration at 5000 ms; freq must be 20–20000 Hz.
AudioClip make_beep(uint32_t freq_hz, uint32_t duration_ms,
                    const AudioSpec& spec = {});

// Parse RIFF/PCM WAV from file. Throws std::runtime_error on failure.
// Rejects files > 1 MB.
AudioClip load_wav(const std::string& path);

class AudioPlayer {
public:
    virtual ~AudioPlayer() = default;
    virtual bool    open(const AudioSpec& spec) = 0;  // returns false if /dev/dsp unavailable
    virtual void    close() = 0;
    virtual void    play(const AudioClip& clip) = 0;  // non-blocking; replaces queued clip
    virtual void    stop() = 0;
    virtual bool    is_playing() const = 0;
    virtual void    set_volume(uint8_t level) = 0;    // 0–100
    virtual uint8_t get_volume() const = 0;
};
}
```

### `native/include/kindle/linux_audio.hpp`
Declaration for `LinuxOssAudioPlayer`.

```
namespace kindle::audio {
class LinuxOssAudioPlayer : public AudioPlayer {
public:
    // dsp_path defaults to "/dev/dsp"; mixer_path defaults to "/dev/mixer"
    explicit LinuxOssAudioPlayer(std::string dsp_path   = "/dev/dsp",
                                  std::string mixer_path = "/dev/mixer");
    ~LinuxOssAudioPlayer() override;
    bool    open(const AudioSpec& spec) override;
    void    close() override;
    void    play(const AudioClip& clip) override;
    void    stop() override;
    bool    is_playing() const override;
    void    set_volume(uint8_t level) override;
    uint8_t get_volume() const override;

private:
    // worker thread, mutex, condition_variable, pending clip
};
}
```

### `native/include/kindle/fake_audio.hpp`
Header-only fake for tests. Mirrors `fake_eink.hpp` style.

```
namespace kindle::audio {
class FakeAudioPlayer : public AudioPlayer {
public:
    bool    open(const AudioSpec&) override { is_open_ = true; return true; }
    void    close() override { is_open_ = false; is_playing_ = false; }
    void    play(const AudioClip& clip) override {
        last_clip_ = clip; ++play_count_; is_playing_ = true;
    }
    void    stop() override { is_playing_ = false; ++stop_count_; }
    bool    is_playing() const override { return is_playing_; }
    void    set_volume(uint8_t v) override { volume_ = v; }
    uint8_t get_volume() const override { return volume_; }

    // Test inspection
    uint32_t         get_play_count() const { return play_count_; }
    uint32_t         get_stop_count() const { return stop_count_; }
    const AudioClip& get_last_clip()  const { return last_clip_; }
    bool             is_open()        const { return is_open_; }
private:
    AudioClip last_clip_;
    uint32_t  play_count_ = 0;
    uint32_t  stop_count_ = 0;
    uint8_t   volume_     = 70;
    bool      is_open_    = false;
    bool      is_playing_ = false;
};
}
```

### `native/include/kindle/audio_loader.hpp`
Network clip fetcher. Depends on `network.hpp`.

```
namespace kindle::audio {
// Fetches a WAV from url via client. Enforces 1 MB cap.
// Throws std::runtime_error on HTTP failure or invalid WAV.
AudioClip fetch_audio(network::HttpClient& client, const std::string& url);
}
```

### `native/src/audio.cpp`
Implements `make_beep` and `load_wav`.

- `make_beep`: generate N = sample_rate × duration_ms / 1000 samples using
  `sample[i] = 32767 × sin(2π × freq × i / sample_rate)`. Integer-only math
  (fixed-point sine or `sinf` from softfp VFP). Cap at 5000 ms / 20–20000 Hz.
- `load_wav`: open file, validate "RIFF"/"WAVE"/"fmt "/"data" chunks, assert
  PCM format (wFormatTag=1), channels=1, 16-bit. Read samples into vector.
  Reject if file > 1 MB before reading.

### `native/src/audio_loader.cpp`
Implements `fetch_audio`.

- Call `client.get(url)`, check `response.ok()`.
- Guard: `response.body.size() > 1 MB → throw`.
- Copy body bytes to a temp buffer, call `load_wav` logic on the in-memory buffer
  (extract a helper `parse_wav(const uint8_t*, size_t)` used by both `load_wav`
  and `fetch_audio`).

### `native/platform/linux/oss_audio_linux.cpp`
Implements `LinuxOssAudioPlayer`.

**open():**
```
fd_ = ::open(dsp_path_, O_WRONLY);
// SNDCTL_DSP_SETFMT → AFMT_S16_LE
// SNDCTL_DSP_CHANNELS → 1
// SNDCTL_DSP_SPEED → spec.sample_rate
// SNDCTL_DSP_SETFRAGMENT → 0x00040009 (16 frags of 512 bytes each)
// open /dev/mixer, read initial volume
// launch worker std::thread
```

**play():**
```
{ std::lock_guard lock(mutex_); pending_ = clip; }
cv_.notify_one();
```

**worker thread loop:**
```
while (!quit_) {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&]{ return quit_ || pending_.has_value(); });
    if (quit_) break;
    AudioClip clip = std::move(*pending_); pending_.reset();
    is_playing_ = true;
    lock.unlock();
    // write clip.samples as raw bytes to fd_ in 512-sample chunks
    is_playing_ = false;
}
```

**set_volume():**
```
int packed = (level << 8) | level;   // same for both L and R channels
::ioctl(mixer_fd_, SOUND_MIXER_WRITE_PCM, &packed);
volume_ = level;
```

**close():**
```
{ std::lock_guard lock(mutex_); quit_ = true; }
cv_.notify_all();
thread_.join();
::close(fd_); ::close(mixer_fd_);
```

Edge cases:
- `open()` returns `false` if `/dev/dsp` fails → callers degrade silently.
- `play()` called while playing: new clip replaces pending_ (next in queue), current write finishes.
- `stop()`: sets `stop_requested_` atomic; worker skips remaining write chunks.
- Fragment size of 512 samples @ 22050 Hz ≈ 23ms per write — low latency, no overrun.

### `native/tests/test_audio.cpp`
Unit tests using `FakeAudioPlayer` (no real audio hardware needed).

Tests to cover:
- `make_beep` produces correct sample count and non-zero amplitude
- `load_wav` parses a minimal RIFF/PCM WAV blob (embed a 100-sample test WAV as a constexpr array)
- `load_wav` throws on truncated/invalid WAV
- `load_wav` throws when size > 1 MB
- `FakeAudioPlayer::play` increments `play_count_`, stores `last_clip_`
- `FakeAudioPlayer::stop` increments `stop_count_`, clears `is_playing_`
- `FakeAudioPlayer::set_volume` / `get_volume` round-trip
- `fetch_audio` with a mock `HttpClient` (inject `FakeSocketBackend` from existing network tests)
- `fetch_audio` throws when response body > 1 MB

---

## Files to Modify

### `native/CMakeLists.txt`
Add to `kindle_native` sources:
```cmake
src/audio.cpp
src/audio_loader.cpp
platform/fake/fake_audio.cpp      # (or keep header-only; no .cpp needed for fake)
platform/linux/oss_audio_linux.cpp
```

Add test target:
```cmake
add_executable(test_audio tests/test_audio.cpp)
target_link_libraries(test_audio kindle_native)
add_test(NAME test_audio COMMAND test_audio)
```

No new CMake options — OSS has zero external dependencies.

---

## Per-App Integration Guidance (not in SDK, documented here for implementers)

### dino
- At app startup: instantiate `LinuxOssAudioPlayer` (or `FakeAudioPlayer` for tests).
- Pre-generate clips: `jump_sound_ = make_beep(880, 60)`, `die_sound_ = make_beep(220, 300)`,
  `milestone_sound_ = make_beep(1320, 150)`.
- Call `audio_.play(jump_sound_)` in game event handlers.
- Wire in input handler:
  ```cpp
  case KeyCode::VolumeUp:   audio_.set_volume(std::min(100u, audio_.get_volume() + 10u)); break;
  case KeyCode::VolumeDown: audio_.set_volume(audio_.get_volume() >= 10u ? audio_.get_volume() - 10u : 0u); break;
  ```

### papergram
- Embed a short notification WAV as `constexpr uint8_t kPingWav[]` (a 0.5s sine chirp, ~44 KB).
- On message received: `audio_.play(ping_clip_)`.
- For custom sounds: `fetch_audio(http_client_, notification_url_)`, store as `AudioClip`, play.
- Volume wired to same key handler pattern.

### kindle-myts
- `AnsiParser` currently has no BEL handler. Add `on_bel()` to `IAnsiHandler` interface.
- In `TerminalSession::on_bel()`: `audio_.play(bell_clip_)` where
  `bell_clip_ = make_beep(440, 200)` created at startup.
- Volume keys wire into `InputManager` like other keys.

---

## Volume Control Rules

| Condition | Behavior |
|-----------|----------|
| `/dev/mixer` unavailable | `set_volume` is no-op; `get_volume` returns default 70 |
| Volume set while playing | Takes effect immediately on next write chunk |
| Volume = 0 | Silence; PCM writes continue (driver handles muting) |
| App receives VolumeUp | Call `set_volume(min(100, get_volume() + 10))` |
| App receives VolumeDown | Call `set_volume(get_volume() >= 10 ? get_volume() - 10 : 0)` |

---

## Verification

```bash
cd native/build
cmake .. && make test_audio
./test_audio         # all tests pass without audio hardware

# On Kindle hardware or with ALSA OSS loaded:
# Write a small test program that calls make_beep + LinuxOssAudioPlayer
# Hear the tone through headphones or speaker

# Cross-compile check:
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-Kindle-ARMv6.cmake
make kindle_native   # must link with no new external symbols
```

No new libraries introduced. All tests run on host (x86_64) using `FakeAudioPlayer`.
