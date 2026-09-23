// native/include/kindle/refresh_policy.hpp
#pragma once
#include "kindle/eink.hpp"
#include "kindle/dirty.hpp"
#include <cstdint>
#include <vector>
#include <initializer_list>

namespace kindle {

class RefreshPolicy {
public:
    virtual ~RefreshPolicy() = default;
    virtual RefreshMode decide(const DirtyRegion& region,
                               uint32_t screen_w, uint32_t screen_h) = 0;
    virtual void reset() = 0;
};

// Escalate to Flash after partial_limit consecutive Partial updates.
// The counter resets automatically after a Flash decision.
class CounterPolicy : public RefreshPolicy {
public:
    explicit CounterPolicy(uint32_t partial_limit);
    RefreshMode decide(const DirtyRegion& region,
                       uint32_t screen_w, uint32_t screen_h) override;
    void reset() override;
private:
    uint32_t partial_limit_;
    uint32_t count_ = 0;
};

// Escalate to Flash when dirty area exceeds dirty_ratio_threshold (0.0-1.0).
class AreaPolicy : public RefreshPolicy {
public:
    explicit AreaPolicy(float dirty_ratio_threshold);
    RefreshMode decide(const DirtyRegion& region,
                       uint32_t screen_w, uint32_t screen_h) override;
    void reset() override;
private:
    float threshold_;
};

// Flash when ANY sub-policy would Flash; resets all on reset().
class CompositePolicy : public RefreshPolicy {
public:
    CompositePolicy(std::initializer_list<RefreshPolicy*> policies);
    RefreshMode decide(const DirtyRegion& region,
                       uint32_t screen_w, uint32_t screen_h) override;
    void reset() override;
private:
    std::vector<RefreshPolicy*> policies_;
};

} // namespace kindle
