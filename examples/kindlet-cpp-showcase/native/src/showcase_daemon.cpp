#include "system_telemetry.hpp"
#include "kindle/ipc.hpp"
#include <iostream>
#include <sstream>
#include <string>

int main() {
    kindle::IpcMessage in_msg;

    // Loop reading messages from stdin using the standard Kindle IPC channel
    while (kindle::IpcChannel::read_message(std::cin, in_msg)) {
        if (in_msg.type == kindle::MessageType::Shutdown) {
            break;
        }

        kindle::IpcMessage out_msg;
        out_msg.flags = 0;
        out_msg.request_id = in_msg.request_id;

        if (in_msg.type == kindle::MessageType::Ping) {
            out_msg.type = kindle::MessageType::Pong;
            out_msg.set_payload("PONG_FROM_SHOWCASE_DAEMON");
        } else if (in_msg.type == kindle::MessageType::Command) {
            std::string cmd = in_msg.payload_as_string();
            out_msg.type = kindle::MessageType::Response;

            if (cmd.find("get_telemetry") != std::string::npos) {
                out_msg.set_payload(showcase::SystemTelemetry::query_telemetry_json());
            } else if (cmd.find("generate_pattern") != std::string::npos) {
                // Parse optional pattern index
                int ptype = 0;
                if (cmd.find("pattern=1") != std::string::npos) ptype = 1;
                if (cmd.find("pattern=2") != std::string::npos) ptype = 2;

                showcase::EinkPattern pat = showcase::SystemTelemetry::generate_pattern(200, 120, ptype);
                std::string hex_data = showcase::SystemTelemetry::encode_pattern_hex(pat);

                std::ostringstream resp;
                resp << "{"
                     << "\"status\":\"ok\","
                     << "\"width\":" << pat.width << ","
                     << "\"height\":" << pat.height << ","
                     << "\"pattern_type\":" << ptype << ","
                     << "\"hex_pixels\":\"" << hex_data << "\""
                     << "}";
                out_msg.set_payload(resp.str());
            } else {
                // Echo / default response
                out_msg.set_payload("{\"status\":\"ok\",\"echo\":\"" + cmd + "\"}");
            }
        } else {
            out_msg.type = kindle::MessageType::Response;
            out_msg.set_payload("{\"status\":\"acknowledged\"}");
        }

        kindle::IpcChannel::write_message(std::cout, out_msg);
    }

    return 0;
}
