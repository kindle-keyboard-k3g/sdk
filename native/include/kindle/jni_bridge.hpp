#pragma once
#include <cstdint>
#include <vector>

namespace kindle {

class JniBridge {
public:
    static int32_t handle_transaction(int32_t command_id, const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
};

} // namespace kindle
