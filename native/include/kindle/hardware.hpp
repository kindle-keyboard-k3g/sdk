#pragma once
#include "target.hpp"
#include <string>
#include <cstdint>

namespace kindle {

struct MemoryInfo {
    uint64_t total_ram_bytes;
    uint64_t free_ram_bytes;
    uint64_t available_ram_bytes;
};

class HardwareProbe {
public:
    virtual ~HardwareProbe() = default;
    virtual HardwareProfile detect() const = 0;
    virtual MemoryInfo query_memory() const = 0;
};

} // namespace kindle
