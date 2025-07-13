#pragma once

#include "LED.hh"
#include <vector>

namespace LED {

template <size_t BITNUM = 2048> class DataGenerator {
public:
  // 3 tick = 1.25 us
  // 50 us = 3 * 50 / 1.25 = 120 ticks
  //
  // 100 us = 3 * 100 / 1.25 = 240 ticks
  //
  // 1 LED = 24 bits = 3 * 24 = 72 ticks
  //
  // 100us -> N LEDs -> 100us
  //  ---> 480 + N * 72 ticks
  //

  [[nodiscard]] auto generate(std::span<RGB> rgbs) {
    if (rgbs.size() * 72 + 480 > BITNUM) {
      throw std::runtime_error("Too many LEDs for the given BITNUM.");
    }
    std::array<uint32_t, BITNUM> data{};
    data.fill(0x00000000);
    size_t head_in_bits = 0;

    auto push_back = [&](bool bit) {
      if (bit) {
        data.at(head_in_bits / 32) |= (1 << (31 - head_in_bits % 32));
      }
      head_in_bits++;
    };
    head_in_bits = 240;
    for (const auto &rgb : rgbs) {
      uint32_t data = rgb.r << 8 | rgb.g << 16 | rgb.b;
      for (int i = 0; i < 24; ++i) {
        bool flag = (data & (1 << (23 - i))) != 0;
        if (flag) {
          for (size_t j = 0; j < 2; ++j) {
            push_back(true);
          }
          for (size_t j = 0; j < 1; ++j) {
            push_back(false);
          }
        } else {
          for (size_t j = 0; j < 1; ++j) {
            push_back(true);
          }
          for (size_t j = 0; j < 2; ++j) {
            push_back(false);
          }
        }
      }
    }
    return data;
  }

  [[nodiscard]] auto generate(std::initializer_list<RGB> rgbs) {
    std::vector<RGB> rgb_vector(rgbs);
    return generate(std::span<RGB>(rgb_vector));
  }
};

}; // namespace LED
