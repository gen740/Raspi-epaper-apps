#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <print>
#include <thread>
#include <vector>

#include "epd_7in3e.hh"

struct RGB {
  uint8_t r, g, b;
};

std::array<std::pair<uint8_t, RGB>, 6> color_palette = {{
    {0x00, {.r = 0x00, .g = 0x00, .b = 0x00}},  // Black
    {0x01, {.r = 0xFF, .g = 0xFF, .b = 0xFF}},  // White
    {0x02, {.r = 0xFF, .g = 0xFF, .b = 0x00}},  // Yellow
    {0x03, {.r = 0xFF, .g = 0x00, .b = 0x00}},  // Red
    {0x05, {.r = 0x00, .g = 0x00, .b = 0xFF}},  // Blue
    {0x06, {.r = 0x00, .g = 0xFF, .b = 0x00}},  // Green
}};

auto load_bmp(const std::string &filename, int &width, int &height) -> std::vector<RGB> {
  int channels;
  unsigned char *data =
      stbi_load(filename.c_str(), &width, &height, &channels, 3);  // 強制的にRGB3チャンネル

  if (!data) {
    throw std::runtime_error("Failed to load image: " + filename);
  }

  std::vector<RGB> image;
  image.reserve(static_cast<size_t>(width * height));
  for (int i = 0; i < width * height; ++i) {
    RGB pixel;
    pixel.r = data[i * 3 + 0];
    pixel.g = data[i * 3 + 1];
    pixel.b = data[i * 3 + 2];
    image.push_back(pixel);
  }

  stbi_image_free(data);
  return image;
}

auto convert_to_buffer(const std::vector<RGB> &pixels) -> std::vector<uint8_t> {
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
    uint8_t second = (i + 1 < ids.size()) ? ids[i + 1] : 0x00;  // Default to black if odd count
    buffer.push_back(static_cast<uint8_t>((first << 4) | second));
  }
  return buffer;
}

constexpr int WIDTH = 800;
constexpr int HEIGHT = 480;

std::vector<uint8_t> fill_segmented_screen() {
  const int pixels_per_byte = 2;
  const int total_pixels = WIDTH * HEIGHT;
  const int total_bytes = total_pixels / pixels_per_byte;

  std::vector<uint8_t> buffer(total_bytes, 0);

  const int num_segments = 6;
  const int segment_width = WIDTH / num_segments;
  const uint8_t colors[num_segments] = {0x0, 0x1, 0x2, 0x3, 0x5, 0x6};

  for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      int segment = x / segment_width;
      if (segment >= num_segments) segment = num_segments - 1;

      uint8_t color = colors[segment] & 0xF;
      int pixel_index = y * WIDTH + x;
      int byte_index = pixel_index / 2;

      if (pixel_index % 2 == 0) {
        buffer[byte_index] &= 0x0F;          // 下位4bitは残す
        buffer[byte_index] |= (color << 4);  // 上位4bitに書き込み
      } else {
        buffer[byte_index] &= 0xF0;   // 上位4bitは残す
        buffer[byte_index] |= color;  // 下位4bitに書き込み
      }
    }
  }

  return buffer;
}

class EpaperDevice {
 private:
  EpaperDevice() : epd7in3e_() {}
  ~EpaperDevice() = default;

  std::vector<uint8_t> image_buffer_;
  Epaper::EPD7IN3E epd7in3e_;

 public:
  EpaperDevice(const EpaperDevice &) = delete;
  auto operator=(const EpaperDevice &) -> EpaperDevice & = delete;

  static auto get() -> EpaperDevice & {
    static EpaperDevice instance_;
    return instance_;
  }

  auto test() -> void {
    int counter = 0;
    for (auto &&i : image_buffer_) {
      auto color = static_cast<uint8_t>(counter++ % 7);
      if (color > 3) {
        color += 1;
      }
      i = static_cast<uint8_t>(color << 4 | color);  // Fill with some pattern
    }
    epd7in3e_.display(image_buffer_.data());
  }

  auto draw_image() -> void {
    int width, height;
    auto data = load_bmp("./pic/output.bmp", width, height);
    std::println("BMP file size: {}", data.size());
    std::println("BMP file dimensions: {}x{}", width, height);

    if (width != 800 || height != 480) {
      throw std::runtime_error("Image dimensions do not match e-Paper display size.");
    }

    epd7in3e_.display(convert_to_buffer(data).data());

    // epd7in3e_.display(fill_segmented_screen().data());
  }
};

auto main() -> int {
  try {
    std::println("Simple test Start!!");
    auto &epaper = EpaperDevice::get();
    epaper.draw_image();
    std::this_thread::sleep_for(std::chrono::seconds(100));
  } catch (const std::exception &e) {
    std::println("Exception occur: {}", e.what());
  }
}
