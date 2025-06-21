#pragma once

#include <vector>

namespace Processing {

enum class EPDColor {
  BLACK = 0x00,
  WHITE = 0x01,
  YELLOW = 0x02,
  RED = 0x03,
  BLUE = 0x05,
  GREEN = 0x06,
};

[[nodiscard]] auto closest_epd_color(const std::uint8_t *rgb) -> EPDColor;

auto nearest_color(const std::array<uint8_t, 3> &c) -> std::array<uint8_t, 3>;

[[nodiscard]]
auto rotate90(const std::vector<uint8_t> &src, std::size_t width,
              std::size_t height) -> std::vector<uint8_t>;

struct ImageAdjustParams {
  float exposure = 0.f;    // EV (±N → 2^N 倍)
  float contrast = 1.f;    // 1 = 無変更
  float highlight = 0.f;   // 0–1
  float shadow = 0.f;      // 0–1
  float saturation = 1.f;  // 1 = 無変更
  float temperature = 0.f; // −1 … +1
  float tint = 0.f;        // −1 … +1
};

void adjustImage(const std::vector<uint8_t> &src, std::vector<uint8_t> &dst,
                 int w, int h, const ImageAdjustParams &p);

void Atkinson(std::vector<uint8_t> &img, int w, int h);

} // namespace Processing
