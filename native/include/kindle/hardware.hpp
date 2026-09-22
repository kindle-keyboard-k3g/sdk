#pragma once
#include "target.hpp"
#include <string>
#include <cstdint>

namespace kindle {

/**
 * System memory status report.
 */
struct MemoryInfo {
    uint64_t total_ram_bytes;       ///< Total installed physical RAM in bytes
    uint64_t free_ram_bytes;        ///< Unused physical RAM in bytes
    uint64_t available_ram_bytes;   ///< Available memory considering reclaimable buffers
};

/**
 * Abstract interface for probing SoC hardware and memory on Linux Kindle platforms.
 */
class HardwareProbe {
public:
    virtual ~HardwareProbe() = default;

    /**
     * Probes CPU and board characteristics to detect Kindle hardware profile.
     *
     * @return HardwareProfile populated with model, SoC, dimensions, and memory limits.
     */
    virtual HardwareProfile detect() const = 0;

    /**
     * Reads current memory usage statistics from /proc/meminfo.
     *
     * @return MemoryInfo containing total, free, and available memory in bytes.
     */
    virtual MemoryInfo query_memory() const = 0;
};

} // namespace kindle
