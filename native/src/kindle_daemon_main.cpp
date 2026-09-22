#include "kindle/ipc.hpp"
#include <iostream>

int main() {
    kindle::IpcMessage in_msg;
    // Read IPC message from Java Kindlet bridge
    while (kindle::IpcChannel::read_message(std::cin, in_msg)) {
        if (in_msg.type == kindle::MessageType::Shutdown) {
            break;
        }

        kindle::IpcMessage out_msg;
        out_msg.flags = 0;
        out_msg.request_id = in_msg.request_id;

        if (in_msg.type == kindle::MessageType::Ping) {
            out_msg.type = kindle::MessageType::Pong;
            out_msg.set_payload("PONG_FROM_ARMV6_CPP");
        } else if (in_msg.type == kindle::MessageType::Command) {
            out_msg.type = kindle::MessageType::Response;
            out_msg.set_payload("{\"status\":\"ok\",\"device\":\"kindle_armv6\",\"echo\":" + in_msg.payload_as_string() + "}");
        } else {
            out_msg.type = kindle::MessageType::Response;
            out_msg.set_payload("{\"status\":\"acknowledged\"}");
        }

        kindle::IpcChannel::write_message(std::cout, out_msg);
    }

    return 0;
}
