#pragma once
#include <string>
#include <cstdint>

namespace kindle {

/**
 * Supported legacy Kindle device models.
 */
enum class TargetModel {
    KindleKeyboard,  ///< Kindle Keyboard (K3 / K3G / K3W)
    KindleDX,        ///< Kindle DX / DX Graphite
    Unknown          ///< Unrecognized or host development environment
};

/**
 * System-on-Chip hardware classification.
 */
enum class SocType {
    Imx353,          ///< Freescale MCIMX353 (ARM1136JF-S @ 532 MHz)
    Imx31,           ///< Freescale MCIMX31L (ARM1136JF-S @ 400 MHz)
    Unknown          ///< Non-Kindle host SoC
};

/**
 * Hardware characteristics and runtime constraints for a Kindle device.
 */
struct HardwareProfile {
    TargetModel model;          ///< Kindle device model
    SocType soc;                ///< Detected System-on-Chip
    std::string cpu_arch;       ///< CPU architecture string (e.g. "armv6")
    uint32_t screen_width;      ///< Native display width in pixels
    uint32_t screen_height;     ///< Native display height in pixels
    uint64_t total_ram_bytes;   ///< Total physical RAM capacity in bytes
};

} // namespace kindle
