#include <chrono>
#include <cstddef>
#include <print>
#include <thread>
#include <vector>

#include "Epaper.hh"

class EpaperDevice {
 private:
  EpaperDevice() {
    if (Epaper::DEV_Module_Init() != 0) {
      throw std::runtime_error("Could not initialize the device");
    }
    std::println("e-Paper Init and Clear...");
    Epaper::EPD_7IN3E_Init();
    Epaper::EPD_7IN3E_Clear(0x01);  // WHITE
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    image_buffer_.resize(static_cast<std::size_t>(2 * 480 * 800));
    // Paint_NewImage(image_buffer_.data(), EPD_7IN3E_WIDTH, EPD_7IN3E_HEIGHT, 0, EPD_7IN3E_WHITE);
  }

  ~EpaperDevice() {
    std::println("Clear...");
    Epaper::EPD_7IN3E_Clear(0x01);
    Epaper::EPD_7IN3E_Sleep();
    std::println("e-Paper Sleep...");
    Epaper::DEV_Module_Exit();
  }

  std::vector<uint8_t> image_buffer_;

 public:
  EpaperDevice(const EpaperDevice &) = delete;
  auto operator=(const EpaperDevice &) -> EpaperDevice & = delete;

  static auto get() -> EpaperDevice & {
    static EpaperDevice instance_;
    return instance_;
  }

  auto test() -> void {
    // std::println("show bmp1-----------------");
    // Paint_SetScale(6);
    // Paint_SelectImage(image_buffer_.data());
    // Paint_Clear(EPD_7IN3E_WHITE);
    // GUI_ReadBmp_RGB_6Color("./pic/output.bmp", 0, 0);
    Epaper::EPD_7IN3E_Display(image_buffer_.data());
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
