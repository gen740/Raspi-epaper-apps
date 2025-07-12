#pragma once

#include <fcntl.h>
#include <print>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

class Mailbox final {
public:
  Mailbox() {
    fd_ = ::open("/dev/vcio", O_RDWR);
    if (fd_ < 0) {
      std::println(stderr, "Failed to open /dev/vcio: {}", ::strerror(errno));
      std::exit(EXIT_FAILURE);
    }
  }

  [[nodiscard]] auto request(uint32_t tag_id, std::span<uint32_t> request,
                             size_t response_size) -> std::vector<uint32_t> {
    auto buffer_size =
        std::max(request.size() * sizeof(uint32_t), response_size);
    if (buffer_size > 16) {
      std::println(stderr, "Request size exceeds maximum of 16 elements.");
      std::exit(EXIT_FAILURE);
    }
    if (buffer_size % 4 != 0) {
      std::println(stderr, "Request size must be a multiple of 4.");
      std::exit(EXIT_FAILURE);
    }

    std::array<uint32_t, 16> buf{};
    buf.fill(0);

    size_t total_size = 0;
    buf.at(total_size++) = 0x00;
    buf.at(total_size++) = 0x00;   // request
    buf.at(total_size++) = tag_id; // Tag ID
    buf.at(total_size++) = buffer_size;
    buf.at(total_size++) = 0;
    for (size_t i = 0; i < buffer_size / 4; ++i) {
      if (i < request.size()) {
        buf.at(total_size++) = request[i];
      } else {
        buf.at(total_size++) = 0x00; // Fill with zeros if not enough values
      }
    }
    buf.at(total_size++) = 0x00; // End tag (0x0)
    buf.at(0) =
        static_cast<uint32_t>(total_size * sizeof(uint32_t)); // total size
    auto ret_code = ::ioctl(fd_, IOCTL_MBOX_PROPERTY, buf.data());
    if (ret_code < 0) {
      std::println(stderr, "Failed to send mailbox request: {}",
                   ::strerror(errno));
      std::exit(EXIT_FAILURE);
    }
    return {buf.begin() + 5, buf.begin() + 5 + response_size / 4};
  }

  ~Mailbox() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

private:
  static constexpr auto IOCTL_MBOX_PROPERTY = _IOWR(100, 0, char *);
  int fd_{-1};
};

inline auto allocate_memory(size_t size, uint32_t flags) -> uint32_t {
  Mailbox mbox;
  std::array<uint32_t, 3> request{};
  request[0] = size;
  request[1] = 4096;  // alignment
  request[2] = flags; // flags
  auto response = mbox.request(0x0003000c, request, 12);
  if (response.size() < 1 || response[0] == 0) {
    std::println(stderr, "Failed to allocate memory.");
    std::exit(EXIT_FAILURE);
  }
  return response[0];
};

inline auto lock_memory(uint32_t handle) -> uint32_t {
  Mailbox mbox;
  std::array<uint32_t, 1> request{};
  request[0] = handle;
  auto response = mbox.request(0x0003000d, request, 4);
  if (response.size() < 1 || response[0] == 0) {
    std::println(stderr, "Failed to lock memory.");
    std::exit(EXIT_FAILURE);
  }
  return response[0];
}

inline void unlock_memory(uint32_t handle) {
  Mailbox mbox;
  std::array<uint32_t, 1> request{};
  request[0] = handle;
  auto response = mbox.request(0x0003000e, request, 4);
  if (response.size() < 1 || response[0] != 0) {
    std::println(stderr, "Failed to unlock memory.");
    std::exit(EXIT_FAILURE);
  }
}

inline void free_memory(uint32_t handle) {
  Mailbox mbox;
  std::array<uint32_t, 1> request{};
  request[0] = handle;
  auto response = mbox.request(0x0003000f, request, 4);
  if (response.size() < 1 || response[0] != 0) {
    std::println(stderr, "Failed to free memory.");
    std::exit(EXIT_FAILURE);
  }
}
