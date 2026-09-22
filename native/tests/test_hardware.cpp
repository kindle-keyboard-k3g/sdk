#include "kindle/hardware.hpp"
#include <cassert>
#include <iostream>

namespace kindle {
    class FakeHardwareProbe;
}

#include "../platform/fake/fake_procfs.cpp"

int main() {
    std::cout << "Running test_hardware..." << std::endl;

    kindle::FakeHardwareProbe k3_probe(kindle::TargetModel::KindleKeyboard, 256 * 1024 * 1024, 180 * 1024 * 1024);
    auto k3_prof = k3_probe.detect();
    assert(k3_prof.model == kindle::TargetModel::KindleKeyboard);
    assert(k3_prof.soc == kindle::SocType::Imx353);
    assert(k3_prof.screen_width == 600);
    assert(k3_prof.screen_height == 800);
    assert(k3_prof.total_ram_bytes == 268435456);

    auto k3_mem = k3_probe.query_memory();
    assert(k3_mem.total_ram_bytes == 268435456);
    assert(k3_mem.free_ram_bytes == 180 * 1024 * 1024);

    kindle::FakeHardwareProbe dx_probe(kindle::TargetModel::KindleDX, 128 * 1024 * 1024, 70 * 1024 * 1024);
    auto dx_prof = dx_probe.detect();
    assert(dx_prof.model == kindle::TargetModel::KindleDX);
    assert(dx_prof.soc == kindle::SocType::Imx31);
    assert(dx_prof.screen_width == 824);
    assert(dx_prof.screen_height == 1200);
    assert(dx_prof.total_ram_bytes == 134217728);

    std::cout << "PASS: test_hardware verified!" << std::endl;
    return 0;
}
