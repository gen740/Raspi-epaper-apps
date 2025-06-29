#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stb/stb_image.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace Apps::Common {

struct RGB {
  uint8_t r, g, b;
};

inline auto load_bmp(const std::string &filename) -> std::vector<RGB> {
  int w, h, channels;
  auto *data = stbi_load(filename.c_str(), &w, &h, &channels, 3);
  if (!data) {
    throw std::runtime_error("Failed to load image: " + filename);
  }

  const bool rotate_ccw = (w == 480 && h == 800);
  const int out_w = rotate_ccw ? h : w;
  const int out_h = rotate_ccw ? w : h;

  std::vector<RGB> image;
  image.reserve(static_cast<size_t>(out_w) * out_h);

  if (!rotate_ccw) {
    for (int i = 0; i < w * h; ++i) {
      image.push_back({data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2]});
    }
  } else {
    for (int y = 0; y < out_h; ++y) {
      for (int x = 0; x < out_w; ++x) {
        const int src_x = y;
        const int src_y = h - 1 - x;
        const int idx = (src_y * w + src_x) * 3;
        image.push_back({data[idx + 0], data[idx + 1], data[idx + 2]});
      }
    }
  }

  stbi_image_free(data);
  return image;
}

constexpr std::array<std::pair<uint8_t, RGB>, 6> color_palette = {{
    {0x00, {.r = 0x00, .g = 0x00, .b = 0x00}}, // Black
    {0x01, {.r = 0xFF, .g = 0xFF, .b = 0xFF}}, // White
    {0x02, {.r = 0xFF, .g = 0xFF, .b = 0x00}}, // Yellow
    {0x03, {.r = 0xFF, .g = 0x00, .b = 0x00}}, // Red
    {0x05, {.r = 0x00, .g = 0x00, .b = 0xFF}}, // Blue
    {0x06, {.r = 0x00, .g = 0xFF, .b = 0x00}}, // Green
}};

inline auto convert_to_buffer(const std::vector<RGB> &pixels)
    -> std::vector<uint8_t> {
  std::vector<uint8_t> ids;
  ids.reserve(pixels.size());

  for (const auto &px : pixels) {
    int best_id = -1;
    int min_dist_sq = std::numeric_limits<int>::max();

    for (const auto &[id, ref] : color_palette) {
      int dr = static_cast<int>(px.r) - ref.r;
      int dg = static_cast<int>(px.g) - ref.g;
      int db = static_cast<int>(px.b) - ref.b;
      int dist_sq = dr * dr + dg * dg + db * db;

      if (dist_sq < min_dist_sq) {
        min_dist_sq = dist_sq;
        best_id = id;
      }
    }

    ids.push_back(best_id);
  }
  std::vector<uint8_t> buffer;

  buffer.reserve(pixels.size() / 2);
  for (size_t i = 0; i < ids.size(); i += 2) {
    uint8_t first = ids[i];
    uint8_t second = (i + 1 < ids.size())
                         ? ids[i + 1]
                         : 0x00; // Default to black if odd count
    buffer.push_back(static_cast<uint8_t>((first << 4) | second));
  }
  return buffer;
}

}; // namespace Apps::Common
