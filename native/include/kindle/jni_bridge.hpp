#pragma once
#include <cstdint>
#include <vector>

namespace kindle {

/**
 * Direct JNI transaction handler for in-process CVM native bindings.
 */
class JniBridge {
public:
    /**
     * Dispatches an in-process native transaction invoked from the Java runtime via JNI.
     *
     * @param command_id Integer action code specifying the operation.
     * @param input Input byte buffer containing serialized request data.
     * @param output Output byte buffer to populate with the operation result.
     * @return 0 on success, or non-zero error code on failure.
     */
    static int32_t handle_transaction(int32_t command_id, const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
};

} // namespace kindle
