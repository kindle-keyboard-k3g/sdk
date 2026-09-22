#include "kindle/ipc.hpp"
#include <arpa/inet.h>

namespace kindle {

static uint32_t to_be32(uint32_t val) { return htonl(val); }
static uint16_t to_be16(uint16_t val) { return htons(val); }
static uint32_t from_be32(uint32_t val) { return ntohl(val); }
static uint16_t from_be16(uint16_t val) { return ntohs(val); }

bool IpcChannel::write_message(std::ostream& os, const IpcMessage& msg) {
    if (msg.payload.size() > MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint32_t magic_be = to_be32(IPC_MAGIC);
    uint8_t ver = IPC_VERSION;
    uint8_t type = static_cast<uint8_t>(msg.type);
    uint16_t flags_be = to_be16(msg.flags);
    uint32_t req_id_be = to_be32(msg.request_id);
    uint32_t len_be = to_be32(static_cast<uint32_t>(msg.payload.size()));

    os.write(reinterpret_cast<const char*>(&magic_be), sizeof(magic_be));
    os.write(reinterpret_cast<const char*>(&ver), sizeof(ver));
    os.write(reinterpret_cast<const char*>(&type), sizeof(type));
    os.write(reinterpret_cast<const char*>(&flags_be), sizeof(flags_be));
    os.write(reinterpret_cast<const char*>(&req_id_be), sizeof(req_id_be));
    os.write(reinterpret_cast<const char*>(&len_be), sizeof(len_be));

    if (!msg.payload.empty()) {
        os.write(reinterpret_cast<const char*>(msg.payload.data()), msg.payload.size());
    }

    os.flush();
    return os.good();
}

bool IpcChannel::read_message(std::istream& is, IpcMessage& out_msg) {
    uint32_t magic_be = 0;
    if (!is.read(reinterpret_cast<char*>(&magic_be), sizeof(magic_be))) return false;
    if (from_be32(magic_be) != IPC_MAGIC) return false;

    uint8_t ver = 0;
    if (!is.read(reinterpret_cast<char*>(&ver), sizeof(ver))) return false;
    if (ver != IPC_VERSION) return false;

    uint8_t type = 0;
    if (!is.read(reinterpret_cast<char*>(&type), sizeof(type))) return false;
    out_msg.type = static_cast<MessageType>(type);

    uint16_t flags_be = 0;
    if (!is.read(reinterpret_cast<char*>(&flags_be), sizeof(flags_be))) return false;
    out_msg.flags = from_be16(flags_be);

    uint32_t req_id_be = 0;
    if (!is.read(reinterpret_cast<char*>(&req_id_be), sizeof(req_id_be))) return false;
    out_msg.request_id = from_be32(req_id_be);

    uint32_t len_be = 0;
    if (!is.read(reinterpret_cast<char*>(&len_be), sizeof(len_be))) return false;
    uint32_t len = from_be32(len_be);

    if (len > MAX_PAYLOAD_SIZE) return false;

    out_msg.payload.resize(len);
    if (len > 0) {
        if (!is.read(reinterpret_cast<char*>(out_msg.payload.data()), len)) return false;
    }

    return true;
}

} // namespace kindle
