#include "kindle/ipc.hpp"
#include <cassert>
#include <sstream>
#include <iostream>

int main() {
    std::cout << "Running test_ipc..." << std::endl;

    std::stringstream stream;

    kindle::IpcMessage send_msg;
    send_msg.type = kindle::MessageType::Command;
    send_msg.request_id = 42;
    send_msg.flags = 0;
    send_msg.set_payload("{\"action\":\"refresh\",\"mode\":\"full\"}");

    assert(kindle::IpcChannel::write_message(stream, send_msg));

    kindle::IpcMessage recv_msg;
    assert(kindle::IpcChannel::read_message(stream, recv_msg));

    assert(recv_msg.type == kindle::MessageType::Command);
    assert(recv_msg.request_id == 42);
    assert(recv_msg.flags == 0);
    assert(recv_msg.payload_as_string() == "{\"action\":\"refresh\",\"mode\":\"full\"}");

    // Test rejection of bad magic
    std::stringstream bad_stream;
    uint32_t bad_magic = 0x12345678;
    bad_stream.write(reinterpret_cast<const char*>(&bad_magic), sizeof(bad_magic));
    kindle::IpcMessage bad_msg;
    assert(!kindle::IpcChannel::read_message(bad_stream, bad_msg));

    std::cout << "PASS: test_ipc verified successfully!" << std::endl;
    return 0;
}
