#include "kindle/hardware.hpp"
#include <sstream>

namespace kindle {

class FakeHardwareProbe : public HardwareProbe {
public:
    FakeHardwareProbe(TargetModel model, uint64_t total_ram, uint64_t free_ram)
        : model_(model), total_ram_(total_ram), free_ram_(free_ram) {}

    HardwareProfile detect() const override {
        HardwareProfile prof;
        prof.model = model_;
        prof.cpu_arch = "armv6";

        if (model_ == TargetModel::KindleKeyboard) {
            prof.soc = SocType::Imx353;
            prof.screen_width = 600;
            prof.screen_height = 800;
            prof.total_ram_bytes = 256 * 1024 * 1024;
        } else if (model_ == TargetModel::KindleDX) {
            prof.soc = SocType::Imx31;
            prof.screen_width = 824;
            prof.screen_height = 1200;
            prof.total_ram_bytes = 128 * 1024 * 1024;
        } else {
            prof.soc = SocType::Unknown;
            prof.screen_width = 0;
            prof.screen_height = 0;
            prof.total_ram_bytes = 0;
        }
        return prof;
    }

    MemoryInfo query_memory() const override {
        MemoryInfo mem;
        mem.total_ram_bytes = total_ram_;
        mem.free_ram_bytes = free_ram_;
        mem.available_ram_bytes = free_ram_;
        return mem;
    }

private:
    TargetModel model_;
    uint64_t total_ram_;
    uint64_t free_ram_;
};

} // namespace kindle
