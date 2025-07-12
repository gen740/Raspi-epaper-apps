// spi_ws281x_20mhz.cpp
// Build: g++ -std=c++20 -O2 -Wall -Wextra spi_ws281x_20mhz.cpp -lbcm2835
// Run  : sudo ./a.out    (root 必須)

#include <bcm2835.h>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

consteval auto my_round(float value) -> int32_t {
  return static_cast<uint32_t>(value + 0.5f); // NOLINT
}

constexpr uint32_t SPI_HZ = 20'000'000;
constexpr auto LED_BITS = 24;
constexpr auto BITS_PER_CODE = my_round(1.20 / (1e6 / SPI_HZ));
constexpr auto HIGHBIT_CODE0 = my_round(0.30 / (1e6 / SPI_HZ));
constexpr auto HIGHBIT_CODE1 = my_round(0.65 / (1e6 / SPI_HZ));
constexpr auto RET_CODE_SIZE = static_cast<int>(100 / (1e6 / SPI_HZ)) / 8;

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

[[nodiscard]] auto build_buffer(RGB rgb) -> std::string {
  std::bitset<static_cast<size_t>(24 * BITS_PER_CODE)> codes;
  uint32_t rgb_value = (rgb.r << 16) | (rgb.g << 8) | rgb.b;
  // uint32_t rgb_value = (rgb.g << 16) | (rgb.r << 8) | rgb.b;

  for (auto i = 0; i < LED_BITS; ++i) {
    bool bit = (rgb_value >> (23 - i)) & 1;
    if (bit) {
      for (auto j = 0; j < HIGHBIT_CODE1; ++j) {
        codes.set(BITS_PER_CODE * i + j, true);
      }
      for (auto j = HIGHBIT_CODE1; j < BITS_PER_CODE; ++j) {
        codes.set(BITS_PER_CODE * i + j, false);
      }
    } else {
      for (auto j = 0; j < HIGHBIT_CODE0; ++j) {
        codes.set(BITS_PER_CODE * i + j, true);
      }
      for (auto j = HIGHBIT_CODE0; j < BITS_PER_CODE; ++j) {
        codes.set(BITS_PER_CODE * i + j, false);
      }
    }
  }

  std::vector<uint8_t> buf;
  buf.resize(LED_BITS * BITS_PER_CODE / 8);
  for (size_t i = 0; i < codes.size(); i++) {
    if (codes.test(i)) {
      buf[i / 8] |= (1 << (7 - (i % 8)));
    } else {
      buf[i / 8] &= ~(1 << (7 - (i % 8)));
    }
  }
  return std::string{buf.begin(), buf.end()};
}

[[nodiscard]] auto ret_code() -> std::string {
  std::string ret;
  for (auto i = 0; i < RET_CODE_SIZE; ++i) {
    ret.append("\x00");
  }
  return ret;
}

auto main() -> int {
  if (!bcm2835_init()) {
    return 1;
  }

  bcm2835_spi_begin();
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_set_speed_hz(SPI_HZ);

  {
    auto buf = ret_code() + build_buffer({.r = 0x00, .g = 0x00, .b = 0x00}) +
               build_buffer({.r = 0x00, .g = 0x00, .b = 0x00}) +
               build_buffer({.r = 0x00, .g = 0x00, .b = 0x00}) + ret_code();
    bcm2835_spi_writenb(
        const_cast<char *>(reinterpret_cast<const char *>(buf.data())),
        buf.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  {
    auto buf = ret_code() + build_buffer({.r = 0xff, .g = 0x00, .b = 0x00}) +
               build_buffer({.r = 0x00, .g = 0xff, .b = 0x00}) +
               build_buffer({.r = 0x00, .g = 0x00, .b = 0xff}) + ret_code();
    bcm2835_spi_writenb(
        const_cast<char *>(reinterpret_cast<const char *>(buf.data())),
        buf.size());
  }
  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}
