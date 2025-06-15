#include <chrono>
#include <cstddef>
#include <print>
#include <thread>
#include <vector>

#include "epd_7in3e.hh"

class EpaperDevice {
 private:
  EpaperDevice() : epd7in3e_() {
    epd7in3e_.clear(Epaper::EPDColor::BLUE);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    image_buffer_.resize(static_cast<std::size_t>(480 * 800));
    // Paint_NewImage(image_buffer_.data(), EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
  }
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
};

auto main() -> int {
  try {
    std::println("Simple test Start!!");
    auto &epaper = EpaperDevice::get();
    epaper.test();
    std::this_thread::sleep_for(std::chrono::seconds(10));
  } catch (const std::exception &e) {
    std::println("Exception occur: {}", e.what());
  }
}
