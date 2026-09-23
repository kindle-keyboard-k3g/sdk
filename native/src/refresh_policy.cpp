// native/src/refresh_policy.cpp
#include "kindle/refresh_policy.hpp"

namespace kindle {

// CounterPolicy

CounterPolicy::CounterPolicy(uint32_t partial_limit)
    : partial_limit_(partial_limit) {}

RefreshMode CounterPolicy::decide(const DirtyRegion&, uint32_t, uint32_t) {
    if (++count_ > partial_limit_) {
        count_ = 0;
        return RefreshMode::Flash;
    }
    return RefreshMode::Partial;
}

void CounterPolicy::reset() { count_ = 0; }

// AreaPolicy

AreaPolicy::AreaPolicy(float threshold) : threshold_(threshold) {}

RefreshMode AreaPolicy::decide(const DirtyRegion& r, uint32_t w, uint32_t h) {
    return (r.area_ratio(w, h) >= threshold_) ? RefreshMode::Flash
                                               : RefreshMode::Partial;
}

void AreaPolicy::reset() {}

// CompositePolicy

CompositePolicy::CompositePolicy(std::initializer_list<RefreshPolicy*> ps)
    : policies_(ps) {}

RefreshMode CompositePolicy::decide(const DirtyRegion& r, uint32_t w, uint32_t h) {
    for (auto* p : policies_) {
        if (p->decide(r, w, h) == RefreshMode::Flash) {
            for (auto* q : policies_) if (q != p) q->reset();
            return RefreshMode::Flash;
        }
    }
    return RefreshMode::Partial;
}

void CompositePolicy::reset() {
    for (auto* p : policies_) p->reset();
}

} // namespace kindle
