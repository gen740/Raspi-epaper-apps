#pragma once

#include <array>
#include <cstdint>
#include <stb/stb_image.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace Apps::Common {

struct RGB {
  uint8_t r, g, b;
};

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

inline auto load_bmp(const std::string &filename) -> std::vector<RGB> {
  int w, h, channels;
  auto *data = stbi_load(filename.c_str(), &w, &h, &channels, 3);
  if (!data) {
    throw std::runtime_error("Failed to load image: " + filename);
  }

  std::vector<RGB> image;
  image.resize(800 * 480);

  if (w == 480 && h == 800) {
    const int dstW = h;
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const int src_idx_base = (y * w + x) * 3;
        const int dstX = y;
        const int dstY = w - 1 - x;
        const int dst_idx = dstY * dstW + dstX;
        image[dst_idx] = {data[src_idx_base], data[src_idx_base + 1],
                          data[src_idx_base + 2]};
      }
    }
  } else if (w == 800 && h == 480) {
    for (int i = 0; i < w * h; ++i) {
      image[i] = {.r = data[i * 3], .g = data[i * 3 + 1], .b = data[i * 3 + 2]};
    }
  } else {
    stbi_image_free(data);
    throw std::invalid_argument("Image must be 480x800 or 800x480");
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
