#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <ranges>
#include <vector>

enum class EPDColor {
  BLACK = 0x00,
  WHITE = 0x01,
  YELLOW = 0x02,
  RED = 0x03,
  BLUE = 0x05,
  GREEN = 0x06,
};

const std::map<EPDColor, std::array<uint8_t, 3>> PALETTE = {
    {EPDColor::BLACK, {0, 0, 0}},       //
    {EPDColor::WHITE, {255, 255, 255}}, //
    {EPDColor::YELLOW, {255, 255, 0}},  //
    {EPDColor::RED, {255, 0, 0}},       //
    {EPDColor::BLUE, {0, 0, 255}},      //
    {EPDColor::GREEN, {0, 255, 0}},     //
};

[[nodiscard]] inline auto closest_epd_color(const std::uint8_t *rgb)
    -> EPDColor {
  double min_dist = std::numeric_limits<double>::max();
  EPDColor closest = EPDColor::BLACK;
  for (const auto &[color, val] : PALETTE) {
    double dist = 0.0;
    for (int i = 0; i < 3; ++i) {
      dist += std::pow(rgb[i] - val[i], 2);
    }
    if (dist < min_dist) {
      min_dist = dist;
      closest = color;
    }
  }
  return closest;
}

inline auto nearest_color(const std::array<uint8_t, 3> &c)
    -> std::array<uint8_t, 3> {
  int best_d = 1 << 30;
  std::array<uint8_t, 3> best = {};
  for (const auto &p : PALETTE | std::views::values) {
    int dr = int(c[0]) - p[0], dg = int(c[1]) - p[1], db = int(c[2]) - p[2];
    int d = dr * dr + dg * dg + db * db;
    if (d < best_d) {
      best_d = d, best = p;
    }
  }
  return best;
}

[[nodiscard]]
inline auto rotate90(const std::vector<uint8_t> &src, std::size_t width,
                     std::size_t height) -> std::vector<uint8_t> {
  constexpr std::size_t BYTES_PER_PX = 3; // r g b
  if (src.size() != width * height * BYTES_PER_PX) {
    throw std::invalid_argument("buffer size mismatch");
  }

  const std::size_t dstW = height; // 480 ← 800, etc.
  const std::size_t dstH = width;
  std::vector<uint8_t> dst(dstW * dstH * BYTES_PER_PX);

  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      const std::size_t src_idx = (y * width + x) * BYTES_PER_PX;

      const std::size_t dstX = y;             // 0‥(height-1)
      const std::size_t dstY = width - 1 - x; // (width-1)‥0
      const std::size_t dst_idx = (dstY * dstW + dstX) * BYTES_PER_PX;

      dst[dst_idx + 0] = src[src_idx + 0]; // R
      dst[dst_idx + 1] = src[src_idx + 1]; // G
      dst[dst_idx + 2] = src[src_idx + 2]; // B
    }
  }
  return dst;
}

inline void Atkinson(std::vector<uint8_t> &img, int w, int h) {
  auto idx = [&](int x, int y) { return 3 * (y * w + x); };
  for (int y = 0; y < h - 2; ++y) {
    for (int x = 0; x < w - 2; ++x) {
      int i = idx(x, y);
      std::array<uint8_t, 3> old = {img[i], img[i + 1], img[i + 2]};
      std::array<uint8_t, 3> newc = nearest_color(old);
      std::array<int, 3> err = {(int(old[0]) - newc[0]) / 8,
                                (int(old[1]) - newc[1]) / 8,
                                (int(old[2]) - newc[2]) / 8};
      for (int c = 0; c < 3; ++c) {
        img[i + c] = newc[c];
      }

      const std::array<std::pair<int, int>, 6> offset = {
          {{1, 0}, {2, 0}, {-1, 1}, {0, 1}, {1, 1}, {0, 2}}};
      for (auto [dx, dy] : offset) {
        int nx = x + dx, ny = y + dy;
        if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
          continue;
        }
        int j = idx(nx, ny);
        for (int c = 0; c < 3; ++c) {
          int val = int(img[j + c]) + err[c];
          img[j + c] = std::clamp(val, 0, 255);
        }
      }
    }
  }
}
