// native/tests/test_refresh_policy.cpp
#include "kindle/refresh_policy.hpp"
#include "kindle/dirty.hpp"
#include <cassert>
#include <iostream>

int main() {
    // CounterPolicy: escalate after N partial updates
    kindle::CounterPolicy counter(3);

    kindle::DirtyRegion small;
    small.mark(0, 0, 10, 10);

    assert(counter.decide(small, 600, 800) == kindle::RefreshMode::Partial);
    assert(counter.decide(small, 600, 800) == kindle::RefreshMode::Partial);
    assert(counter.decide(small, 600, 800) == kindle::RefreshMode::Partial);
    // 4th call: limit reached -> Flash
    assert(counter.decide(small, 600, 800) == kindle::RefreshMode::Flash);

    // reset() clears counter
    counter.reset();
    assert(counter.decide(small, 600, 800) == kindle::RefreshMode::Partial);

    // AreaPolicy: escalate when dirty >= threshold
    kindle::AreaPolicy area(0.5f);
    kindle::DirtyRegion big;
    big.mark_all(600, 800);  // 100% dirty
    assert(area.decide(big, 600, 800) == kindle::RefreshMode::Flash);
    assert(area.decide(small, 600, 800) == kindle::RefreshMode::Partial);

    // CompositePolicy: any sub-policy triggers Flash
    kindle::CounterPolicy cp2(2);
    kindle::AreaPolicy ap2(0.9f);
    kindle::CompositePolicy composite({&cp2, &ap2});

    kindle::DirtyRegion tiny;
    tiny.mark(0, 0, 2, 2);
    assert(composite.decide(tiny, 600, 800) == kindle::RefreshMode::Partial);
    assert(composite.decide(tiny, 600, 800) == kindle::RefreshMode::Partial);
    // cp2 limit reached
    assert(composite.decide(tiny, 600, 800) == kindle::RefreshMode::Flash);

    std::cout << "PASS: test_refresh_policy\n";
    return 0;
}
