#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cstddef>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <vector>

struct RGB {
  uint8_t r, g, b;
};

auto load_bmp(const std::string& filename, int& width, int& height) -> std::vector<RGB> {
  int channels;
  unsigned char* data =
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

auto main() -> int {
  std::println("Reading BMP file...");
  int width, height;
  auto data = load_bmp("./pic/output.bmp", width, height);
  std::println("BMP file size: {}", data.size());
  std::println("BMP file dimensions: {}x{}", width, height);
}
