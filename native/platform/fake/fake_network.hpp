#pragma once
#include "kindle/network.hpp"
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

/**
 * Scripted fake socket backend for unit-testing the network layer.
 * Enqueue responses before each test; inspect captured outgoing bytes after.
 */
class FakeSocketBackend : public kindle::network::detail::SocketBackend {
public:
    FakeSocketBackend() = default;

    /** Enqueue a scripted ASCII/binary response returned by the next receive() calls. */
    void enqueue_response(const std::string& response);
    void enqueue_response_bytes(const std::vector<uint8_t>& bytes);

    /** Optionally split the next enqueued response into fragments of this size. */
    void set_fragment_size(size_t fragment);

    /** Make connect() return -1 (simulates connection refused). */
    void set_connect_fails(bool fails);

    /** Make send_all() return false (simulates broken write). */
    void set_send_fails(bool fails);

    /** Number of connect() calls made so far. */
    int connect_call_count() const;

    /** Return captured outgoing bytes as a string for the given fd. */
    std::string captured_request(int fd) const;

    /** Return captured outgoing bytes as raw bytes for the given fd. */
    std::vector<uint8_t> captured_bytes(int fd) const;

    // SocketBackend overrides
    int     connect(const std::string& host, uint16_t port) override;
    bool    send_all(int fd, const uint8_t* data, size_t size) override;
    int64_t receive(int fd, uint8_t* data, size_t size) override;
    void    close(int fd) override;

private:
    int  next_fd_           = 1;
    bool connect_fails_     = false;
    bool send_fails_        = false;
    int  connect_calls_     = 0;
    size_t fragment_size_   = 0; // 0 = no fragmentation

    // Per-fd receive buffer (deque of byte chunks)
    std::map<int, std::deque<std::vector<uint8_t>>> recv_queues_;
    // Per-fd capture of sent bytes
    std::map<int, std::vector<uint8_t>> sent_bytes_;
    // Global queue of scripted responses (assigned to the next connect fd)
    std::deque<std::vector<uint8_t>> pending_responses_;
};
