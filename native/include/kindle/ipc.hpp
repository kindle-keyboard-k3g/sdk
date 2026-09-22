#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

namespace kindle {

constexpr uint32_t IPC_MAGIC = 0x4B494E44; // 'KIND'
constexpr uint8_t IPC_VERSION = 0x01;
constexpr uint32_t MAX_PAYLOAD_SIZE = 1024 * 1024; // 1 MiB max

enum class MessageType : uint8_t {
    Ping = 0x01,
    Pong = 0x02,
    Command = 0x10,
    Response = 0x11,
    Notification = 0x20,
    Shutdown = 0xFF
};

struct IpcMessage {
    MessageType type;
    uint16_t flags = 0;
    uint32_t request_id = 0;
    std::vector<uint8_t> payload;

    std::string payload_as_string() const {
        return std::string(payload.begin(), payload.end());
    }

    void set_payload(const std::string& str) {
        payload.assign(str.begin(), str.end());
    }
};

class IpcChannel {
public:
    static bool write_message(std::ostream& os, const IpcMessage& msg);
    static bool read_message(std::istream& is, IpcMessage& out_msg);
};

} // namespace kindle
