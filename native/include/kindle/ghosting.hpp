// native/include/kindle/ghosting.hpp
#pragma once
#include "kindle/dirty.hpp"
#include "kindle/eink.hpp"
#include "kindle/pixmap.hpp"
#include <cstdint>
#include <chrono>

namespace kindle {

// Detects pixels that transitioned dark (<=0x7F) -> white (>=0xF0).
// Returns dirty region of those pixels, or empty() if none found.
class DarkToWhiteCleaner {
public:
    DirtyRegion detect(const Pixmap& prev, const Pixmap& current) const;
};

// After idle_ms of inactivity, returns successive checkerboard tiles.
// Each call to next_tile() advances one tile. Returns {0,0,0,0} if inactive.
class IdleRefreshScheduler {
public:
    explicit IdleRefreshScheduler(uint32_t idle_ms = 5000,
                                   uint32_t tile_w = 150,
                                   uint32_t tile_h = 200);

    void     notify_activity();
    EinkRect next_tile(uint32_t screen_w, uint32_t screen_h);
    bool     is_active() const;
    void     reset();

private:
    using Clock = std::chrono::steady_clock;

    uint32_t idle_ms_;
    uint32_t tile_w_;
    uint32_t tile_h_;
    Clock::time_point last_activity_ = Clock::now();
    uint32_t tile_index_ = 0;
};

} // namespace kindle
