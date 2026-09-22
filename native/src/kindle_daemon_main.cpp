#include "kindle/ipc.hpp"
#include "kindle/network.hpp"
#include "kindle/network_ipc.hpp"
#include <iostream>

static void handle_http_request(const kindle::IpcMessage& in_msg,
                                const kindle::network::ProxyConfig& proxy_cfg) {
    kindle::IpcMessage out_msg;
    out_msg.flags      = 0;
    out_msg.request_id = in_msg.request_id;
    out_msg.type       = kindle::MessageType::HttpResponse;

    kindle::network::HttpRequest request;
    bool decoded = kindle::network::ipc::decode_request(in_msg.payload, request);
    if (!decoded) {
        kindle::network::HttpResponse err;
        err.status_code = 0;
        err.error       = "daemon: malformed HttpRequest payload";
        out_msg.payload = kindle::network::ipc::encode_response(err);
        kindle::IpcChannel::write_message(std::cout, out_msg);
        return;
    }

    kindle::network::HttpClient client(proxy_cfg);
    kindle::network::HttpResponse response = client.execute(request);

    auto payload = kindle::network::ipc::encode_response(response);
    if (payload.empty()) {
        kindle::network::HttpResponse enc_err;
        enc_err.status_code = 0;
        enc_err.error       = "daemon: failed to encode HttpResponse";
        out_msg.payload = kindle::network::ipc::encode_response(enc_err);
    } else {
        out_msg.payload = std::move(payload);
    }
    kindle::IpcChannel::write_message(std::cout, out_msg);
}

int main() {
    kindle::network::ProxyConfig proxy_cfg = kindle::network::ProxyConfig::from_environment();

    kindle::IpcMessage in_msg;
    while (kindle::IpcChannel::read_message(std::cin, in_msg)) {
        if (in_msg.type == kindle::MessageType::Shutdown) {
            break;
        }

        if (in_msg.type == kindle::MessageType::HttpRequest) {
            handle_http_request(in_msg, proxy_cfg);
            continue;
        }

        kindle::IpcMessage out_msg;
        out_msg.flags      = 0;
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

