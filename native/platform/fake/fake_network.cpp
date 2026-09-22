#include "fake_network.hpp"
#include <cstring>
#include <stdexcept>

void FakeSocketBackend::enqueue_response(const std::string& response) {
    pending_responses_.push_back(
        std::vector<uint8_t>(response.begin(), response.end()));
}

void FakeSocketBackend::enqueue_response_bytes(const std::vector<uint8_t>& bytes) {
    pending_responses_.push_back(bytes);
}

void FakeSocketBackend::set_fragment_size(size_t fragment) {
    fragment_size_ = fragment;
}

void FakeSocketBackend::set_connect_fails(bool fails) {
    connect_fails_ = fails;
}

void FakeSocketBackend::set_send_fails(bool fails) {
    send_fails_ = fails;
}

int FakeSocketBackend::connect_call_count() const {
    return connect_calls_;
}

std::string FakeSocketBackend::captured_request(int fd) const {
    auto it = sent_bytes_.find(fd);
    if (it == sent_bytes_.end()) return {};
    return std::string(it->second.begin(), it->second.end());
}

std::vector<uint8_t> FakeSocketBackend::captured_bytes(int fd) const {
    auto it = sent_bytes_.find(fd);
    if (it == sent_bytes_.end()) return {};
    return it->second;
}

int FakeSocketBackend::connect(const std::string& /*host*/, uint16_t /*port*/) {
    ++connect_calls_;
    if (connect_fails_) return -1;

    int fd = next_fd_++;
    sent_bytes_[fd] = {};

    // Assign the next pending response to this fd
    if (!pending_responses_.empty()) {
        auto& response = pending_responses_.front();
        if (fragment_size_ > 0 && response.size() > fragment_size_) {
            // Split into fragments
            auto& q = recv_queues_[fd];
            size_t offset = 0;
            while (offset < response.size()) {
                size_t chunk = std::min(fragment_size_, response.size() - offset);
                q.push_back(std::vector<uint8_t>(
                    response.begin() + offset,
                    response.begin() + offset + chunk));
                offset += chunk;
            }
        } else {
            recv_queues_[fd].push_back(response);
        }
        pending_responses_.pop_front();
    }

    return fd;
}

bool FakeSocketBackend::send_all(int fd, const uint8_t* data, size_t size) {
    if (send_fails_) return false;
    auto& buf = sent_bytes_[fd];
    buf.insert(buf.end(), data, data + size);
    return true;
}

int64_t FakeSocketBackend::receive(int fd, uint8_t* data, size_t size) {
    auto it = recv_queues_.find(fd);
    if (it == recv_queues_.end() || it->second.empty()) return 0; // EOF

    auto& chunks = it->second;
    auto& front  = chunks.front();
    size_t to_copy = std::min(size, front.size());
    std::memcpy(data, front.data(), to_copy);

    if (to_copy == front.size()) {
        chunks.pop_front();
    } else {
        front.erase(front.begin(), front.begin() + to_copy);
    }

    return static_cast<int64_t>(to_copy);
}

void FakeSocketBackend::close(int /*fd*/) {
    // no-op
}
