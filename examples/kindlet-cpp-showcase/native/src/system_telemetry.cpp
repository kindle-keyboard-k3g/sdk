#include "system_telemetry.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <inttypes.h>

namespace showcase {

std::string SystemTelemetry::query_telemetry_json() {
    std::string cpu_model = "Unknown Host CPU";
    std::string hardware = "Generic Linux Host";
    std::string soc = "Unknown";
    std::string arch = "x86_64";
    uint64_t total_ram = 0;
    uint64_t free_ram = 0;
    uint64_t available_ram = 0;

    // Probe /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos || line.find("Processor") != std::string::npos) {
                size_t colon = line.find(':');
                if (colon != std::string::npos && colon + 2 < line.size()) {
                    cpu_model = line.substr(colon + 2);
                }
            } else if (line.find("Hardware") != std::string::npos) {
                size_t colon = line.find(':');
                if (colon != std::string::npos && colon + 2 < line.size()) {
                    hardware = line.substr(colon + 2);
                    if (hardware.find("MX35") != std::string::npos) {
                        soc = "Freescale i.MX353";
                        arch = "armv6";
                    } else if (hardware.find("MX31") != std::string::npos) {
                        soc = "Freescale i.MX31";
                        arch = "armv6";
                    }
                }
            }
        }
    }

    // Probe /proc/meminfo
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                uint64_t kb = 0;
                std::sscanf(line.c_str(), "MemTotal: %" SCNu64 " kB", &kb);
                total_ram = kb * 1024;
            } else if (line.find("MemFree:") == 0) {
                uint64_t kb = 0;
                std::sscanf(line.c_str(), "MemFree: %" SCNu64 " kB", &kb);
                free_ram = kb * 1024;
            } else if (line.find("MemAvailable:") == 0) {
                uint64_t kb = 0;
                std::sscanf(line.c_str(), "MemAvailable: %" SCNu64 " kB", &kb);
                available_ram = kb * 1024;
            }
        }
    }

    if (available_ram == 0 && free_ram > 0) {
        available_ram = free_ram;
    }

    std::ostringstream json;
    json << "{"
         << "\"status\":\"ok\","
         << "\"cpu\":\"" << cpu_model << "\","
         << "\"hardware\":\"" << hardware << "\","
         << "\"soc\":\"" << soc << "\","
         << "\"arch\":\"" << arch << "\","
         << "\"total_ram\":" << total_ram << ","
         << "\"free_ram\":" << free_ram << ","
         << "\"available_ram\":" << available_ram
         << "}";

    return json.str();
}

EinkPattern SystemTelemetry::generate_pattern(uint32_t width, uint32_t height, int pattern_type) {
    EinkPattern pattern;
    pattern.width = width;
    pattern.height = height;
    pattern.pixels.resize(width * height, 255);

    if (pattern_type == 0) {
        // 16-level grayscale stepped horizontal bars
        // Split height into 16 horizontal bands
        uint32_t band_h = height / 16;
        if (band_h == 0) band_h = 1;

        for (uint32_t y = 0; y < height; ++y) {
            uint32_t level = (y / band_h);
            if (level > 15) level = 15;
            // 16 levels: 0, 17, 34, ..., 255
            uint8_t gray = static_cast<uint8_t>((level * 255) / 15);
            for (uint32_t x = 0; x < width; ++x) {
                pattern.pixels[y * width + x] = gray;
            }
        }
    } else if (pattern_type == 1) {
        // Radial gradient centered in the canvas quantized to 16 levels
        double cx = width / 2.0;
        double cy = height / 2.0;
        double max_r = std::sqrt(cx * cx + cy * cy);
        if (max_r < 1.0) max_r = 1.0;

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                double dx = x - cx;
                double dy = y - cy;
                double dist = std::sqrt(dx * dx + dy * dy);
                double norm = dist / max_r;
                if (norm > 1.0) norm = 1.0;
                int level = static_cast<int>(norm * 15.0);
                if (level < 0) level = 0;
                if (level > 15) level = 15;
                pattern.pixels[y * width + x] = static_cast<uint8_t>((level * 255) / 15);
            }
        }
    } else {
        // Checkerboard with high-contrast borders
        uint32_t check_size = 16;
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                bool is_dark = ((x / check_size) + (y / check_size)) % 2 == 0;
                pattern.pixels[y * width + x] = is_dark ? 0 : 255;
            }
        }
    }

    return pattern;
}

std::string SystemTelemetry::encode_pattern_hex(const EinkPattern& pattern) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : pattern.pixels) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace showcase
