#pragma once
#include <string>
#include <cstdint>

namespace kindle {

enum class TargetModel {
    KindleKeyboard,  // K3 / K3G
    KindleDX,        // DX / DX Graphite
    Unknown
};

enum class SocType {
    Imx353,
    Imx31,
    Unknown
};

struct HardwareProfile {
    TargetModel model;
    SocType soc;
    std::string cpu_arch;
    uint32_t screen_width;
    uint32_t screen_height;
    uint64_t total_ram_bytes;
};

} // namespace kindle
