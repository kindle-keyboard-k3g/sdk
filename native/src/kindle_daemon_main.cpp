#include "kindle/ipc.hpp"
#include "kindle/network.hpp"
#include "kindle/network_ipc.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using kindle::network::ProxyConfig;

bool parse_proxy_port(const std::string& text, uint16_t& port) {
    int value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end || value < 1 || value > 65535)
        return false;
    port = static_cast<uint16_t>(value);
    return true;
}

ProxyConfig load_proxy_config(int argc, char** argv) {
    std::string host;
    std::string port_text;
    bool has_host_argument = false;
    bool has_port_argument = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument(argv[index]);
        if (argument == "--proxy-host") {
            if (index + 1 >= argc) {
                std::cerr << "Malformed --proxy-host argument: missing value\n";
                return ProxyConfig{};
            }
            host = argv[++index];
            has_host_argument = true;
            continue;
        }
        if (argument == "--proxy-port") {
            if (index + 1 >= argc) {
                std::cerr << "Malformed --proxy-port argument: missing value\n";
                return ProxyConfig{};
            }
            port_text = argv[++index];
            has_port_argument = true;
            continue;
        }
        std::cerr << "Ignoring unknown argument: " << argument << "\n";
    }

    if (has_host_argument || has_port_argument) {
        if (!has_host_argument || !has_port_argument || host.empty()) {
            std::cerr << "Malformed proxy arguments: both --proxy-host and --proxy-port are required\n";
            return ProxyConfig{};
        }
        uint16_t port = 0;
        if (!parse_proxy_port(port_text, port)) {
            std::cerr << "Malformed --proxy-port value: " << port_text << "\n";
            return ProxyConfig{};
        }
        return ProxyConfig{host, port};
    }

    try {
        return ProxyConfig::from_environment();
    } catch (const std::invalid_argument& error) {
        std::cerr << "Malformed Whispernet proxy environment: " << error.what() << "\n";
        return ProxyConfig{};
    }
}

void write_http_error(const kindle::IpcMessage& in_msg, const std::string& message) {
    kindle::network::HttpResponse error;
    error.status_code = 0;
    error.error = message;
    error.reason = message;

    kindle::IpcMessage out_msg;
    out_msg.flags = 0;
    out_msg.request_id = in_msg.request_id;
    out_msg.type = kindle::MessageType::HttpResponse;
    out_msg.payload = kindle::network::ipc::encode_response(error);
    kindle::IpcChannel::write_message(std::cout, out_msg);
}

void handle_http_request(const kindle::IpcMessage& in_msg,
                         const ProxyConfig& proxy_config) {
    try {
        kindle::network::HttpRequest request;
        if (!kindle::network::ipc::decode_request(in_msg.payload, request)) {
            write_http_error(in_msg, "daemon: malformed HttpRequest payload");
            return;
        }

        kindle::network::HttpClient client(proxy_config);
        kindle::network::HttpResponse response = client.execute(request);
        if (response.status_code == 0 && response.reason.empty()) {
            response.reason = response.error;
        }

        kindle::IpcMessage out_msg;
        out_msg.flags = 0;
        out_msg.request_id = in_msg.request_id;
        out_msg.type = kindle::MessageType::HttpResponse;
        out_msg.payload = kindle::network::ipc::encode_response(response);
        if (out_msg.payload.empty()) {
            write_http_error(in_msg, "daemon: failed to encode HttpResponse");
            return;
        }
        kindle::IpcChannel::write_message(std::cout, out_msg);
    } catch (const std::exception& error) {
        write_http_error(in_msg, std::string("daemon: request failure: ") + error.what());
    } catch (...) {
        write_http_error(in_msg, "daemon: request failure: unknown exception");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const ProxyConfig proxy_config = load_proxy_config(argc, argv);

        kindle::IpcMessage in_msg;
        while (kindle::IpcChannel::read_message(std::cin, in_msg)) {
            if (in_msg.type == kindle::MessageType::Shutdown) break;

            if (in_msg.type == kindle::MessageType::HttpRequest) {
                handle_http_request(in_msg, proxy_config);
                continue;
            }

            kindle::IpcMessage out_msg;
            out_msg.flags = 0;
            out_msg.request_id = in_msg.request_id;

            if (in_msg.type == kindle::MessageType::Ping) {
                out_msg.type = kindle::MessageType::Pong;
                out_msg.set_payload("PONG_FROM_ARMV6_CPP");
            } else if (in_msg.type == kindle::MessageType::Command) {
                out_msg.type = kindle::MessageType::Response;
                out_msg.set_payload(
                    "{\"status\":\"ok\",\"device\":\"kindle_armv6\",\"echo\":" +
                    in_msg.payload_as_string() + "}");
            } else {
                out_msg.type = kindle::MessageType::Response;
                out_msg.set_payload("{\"status\":\"acknowledged\"}");
            }

            kindle::IpcChannel::write_message(std::cout, out_msg);
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "kindle_daemon fatal error: " << error.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "kindle_daemon fatal error: unknown exception\n";
        return 1;
    }
}
