#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <print>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

template <class T> class MemoryMapCast {
  void set_(uint32_t phys) {
    phys_ = phys & ~(getpagesize() - 1);
    offset_ = phys - phys_;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
      perror("open /dev/mem");
      exit(1);
    }
    void *map = mmap(nullptr, offset_ + sizeof(T), PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, phys_);
    if (map == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }
    close(fd);
    base_ = static_cast<volatile uint32_t *>(map);
  }

  void cleanup_() {
    if (base_) {
      munmap((void *)base_, sizeof(T));
      base_ = nullptr;
    }
  }

public:
  MemoryMapCast() = default;
  MemoryMapCast(uint32_t phys) { set_(phys); }
  ~MemoryMapCast() { cleanup_(); }

  void reset(uint32_t phys) {
    cleanup_();
    set_(phys);
  }

  auto operator->() -> volatile T * {
    return reinterpret_cast<volatile T *>(base_ + offset_ / 4);
  }

private:
  volatile uint32_t *base_;
  uint32_t phys_{};
  uint32_t offset_{};
};
