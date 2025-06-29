#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <print>
#include <random>
#include <thread>
#include <vector>

#include "Utility.hh"
#include "epd_7in3e.hh"

namespace fs = std::filesystem;

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
      i = static_cast<uint8_t>(color << 4 | color); // Fill with some pattern
    }
    epd7in3e_.display(image_buffer_.data());
  }

  auto draw_image(const std::string &image_path) -> void {
    int width, height;
    auto data = Apps::Common::load_bmp(image_path, width, height);
    std::println("BMP file size: {}", data.size());
    std::println("BMP file dimensions: {}x{}", width, height);

    if (width != 800 || height != 480) {
      throw std::runtime_error(
          "Image dimensions do not match e-Paper display size.");
    }

    epd7in3e_.display(convert_to_buffer(data).data());
  }
};

auto main() -> int {
  while (true) {
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
      std::cerr << "HOME environment variable not set\n";
      return 1;
    }
    const fs::path images_root = fs::path(home) / "images";

    std::random_device rd;
    std::mt19937_64 rng(rd());

    for (auto const &dir_entry : fs::directory_iterator(images_root)) {
      if (!dir_entry.is_directory()) {
        continue;
      }
      std::vector<fs::directory_entry> files;
      for (auto const &file_entry : fs::directory_iterator(dir_entry.path())) {
        if (file_entry.is_regular_file()) {
          files.push_back(file_entry);
        }
      }
      if (files.empty()) {
        continue;
      }
      std::uniform_int_distribution<std::size_t> dist(0, files.size() - 1);
      const auto &chosen = files[dist(rng)];
      try {
        auto &epaper = EpaperDevice::get();
        epaper.draw_image(chosen.path().string());
      } catch (const std::exception &e) {
        std::cerr << "Error processing file " << chosen.path() << ": "
                  << e.what() << '\n';
      }
    }
    // Sleep for 300 seconds before the next iteration
    std::this_thread::sleep_for(std::chrono::seconds(300));
  }
  return 0;
}
