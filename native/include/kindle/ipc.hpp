#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>

namespace kindle {

/** Magic marker identifying Kindle SDK IPC stream frames ('KIND'). */
constexpr uint32_t IPC_MAGIC = 0x4B494E44;

/** Supported protocol wire format version. */
constexpr uint8_t IPC_VERSION = 0x01;

/** Maximum allowed payload size per frame (1 MiB) to guard against buffer exhaustion. */
constexpr uint32_t MAX_PAYLOAD_SIZE = 1024 * 1024;

/**
 * IPC message types exchanged between the Java supervisor and native process.
 */
enum class MessageType : uint8_t {
    Ping = 0x01,         ///< Health check ping from supervisor
    Pong = 0x02,         ///< Response to ping
    Command = 0x10,      ///< Request/command carrying action payload
    Response = 0x11,     ///< Reply to command with status and results
    Notification = 0x20, ///< Asynchronous unsolicited event notification
    Shutdown = 0xFF      ///< Clean termination signal
};

/**
 * In-memory representation of an IPC framed message.
 */
struct IpcMessage {
    MessageType type;             ///< Message classification code
    uint16_t flags = 0;           ///< Bitmask flags reserved for compression or serialization
    uint32_t request_id = 0;      ///< Correlation ID matching responses to requests
    std::vector<uint8_t> payload; ///< Message payload body (JSON, binary, or plain text)

    /**
     * Converts payload byte buffer into a UTF-8 std::string.
     *
     * @return Decoded payload as string.
     */
    std::string payload_as_string() const {
        return std::string(payload.begin(), payload.end());
    }

    /**
     * Copies string content into message payload buffer.
     *
     * @param str UTF-8 string to assign as payload.
     */
    void set_payload(const std::string& str) {
        payload.assign(str.begin(), str.end());
    }
};

/**
 * Wire serializer and deserializer for standard I/O IPC channels.
 */
class IpcChannel {
public:
    /**
     * Serializes and writes a framed message to the output stream.
     *
     * @param os Target output stream (e.g. std::cout or socket).
     * @param msg Message structure to serialize and write.
     * @return true if successfully written and flushed, false on write error.
     */
    static bool write_message(std::ostream& os, const IpcMessage& msg);

    /**
     * Reads and deserializes a single framed message from the input stream.
     *
     * @param is Target input stream (e.g. std::cin).
     * @param out_msg Message structure to populate.
     * @return true if valid frame was parsed, false on EOF or framing error.
     */
    static bool read_message(std::istream& is, IpcMessage& out_msg);
};

} // namespace kindle
