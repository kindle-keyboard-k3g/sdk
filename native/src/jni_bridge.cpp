#include "kindle/jni_bridge.hpp"

namespace kindle {

int32_t JniBridge::handle_transaction(int32_t command_id, const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    if (command_id == 1) { // Ping
        output = {'P', 'O', 'N', 'G'};
        return 0;
    }
    // Echo
    output = input;
    return 0;
}

} // namespace kindle
