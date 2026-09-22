#include "kindle/hardware.hpp"
#include <fstream>
#include <sstream>
#include <inttypes.h>

namespace kindle {

class LinuxHardwareProbe : public HardwareProbe {
public:
    HardwareProfile detect() const override {
        HardwareProfile prof;
        prof.model = TargetModel::Unknown;
        prof.soc = SocType::Unknown;
        prof.cpu_arch = "unknown";
        prof.screen_width = 0;
        prof.screen_height = 0;
        prof.total_ram_bytes = 0;

        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("Hardware") != std::string::npos) {
                    if (line.find("MX35") != std::string::npos) {
                        prof.model = TargetModel::KindleKeyboard;
                        prof.soc = SocType::Imx353;
                        prof.cpu_arch = "armv6";
                        prof.screen_width = 600;
                        prof.screen_height = 800;
                        prof.total_ram_bytes = 256 * 1024 * 1024;
                    } else if (line.find("MX31") != std::string::npos) {
                        prof.model = TargetModel::KindleDX;
                        prof.soc = SocType::Imx31;
                        prof.cpu_arch = "armv6";
                        prof.screen_width = 824;
                        prof.screen_height = 1200;
                        prof.total_ram_bytes = 128 * 1024 * 1024;
                    }
                }
            }
        }
        return prof;
    }

    MemoryInfo query_memory() const override {
        MemoryInfo mem{0, 0, 0};
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    uint64_t kb = 0;
                    std::sscanf(line.c_str(), "MemTotal: %" SCNu64 " kB", &kb);
                    mem.total_ram_bytes = kb * 1024;
                } else if (line.find("MemFree:") == 0) {
                    uint64_t kb = 0;
                    std::sscanf(line.c_str(), "MemFree: %" SCNu64 " kB", &kb);
                    mem.free_ram_bytes = kb * 1024;
                } else if (line.find("MemAvailable:") == 0) {
                    uint64_t kb = 0;
                    std::sscanf(line.c_str(), "MemAvailable: %" SCNu64 " kB", &kb);
                    mem.available_ram_bytes = kb * 1024;
                }
            }
        }
        return mem;
    }
};

} // namespace kindle
