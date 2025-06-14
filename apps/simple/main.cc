#include <chrono>
#include <cstddef>
#include <print>
#include <thread>
#include <vector>

#include "DEV_Config.h"
#include "EPD_7in3e.h"
#include "GUI_BMPfile.h"
#include "GUI_Paint.h"

class Epaper {
 private:
  Epaper() {
    if (DEV_Module_Init() != 0) {
      throw std::runtime_error("Could not initialize the device");
    }
    std::println("e-Paper Init and Clear...");
    EPD_7IN3E_Init();
    EPD_7IN3E_Clear(EPD_7IN3E_WHITE);  // WHITE
    DEV_Delay_ms(1000);
    image_cache_.resize(static_cast<std::size_t>(4 * 480 * 800));
  }

  ~Epaper() {
    std::println("Clear...");
    EPD_7IN3E_Clear(EPD_7IN3E_WHITE);
    EPD_7IN3E_Sleep();
    DEV_Delay_ms(2000);  // important, at least 2s
    DEV_Module_Exit();
  }

  std::vector<uint8_t> image_cache_;

 public:
  Epaper(const Epaper &) = delete;
  auto operator=(const Epaper &) -> Epaper & = delete;

  static auto get() -> Epaper & {
    static Epaper instance_;
    return instance_;
  }

  auto test() -> void {
    std::println("show bmp1-----------------");
    Paint_SelectImage(image_cache_.data());
    Paint_Clear(EPD_7IN3E_WHITE);
    GUI_ReadBmp_RGB_6Color("./pic/7in3e.bmp", 0, 0);
    EPD_7IN3E_Display(image_cache_.data());
  }
};

auto main() -> int {
  try {
    std::println("Simple test Start!!");
    auto &epaper = Epaper::get();
    epaper.test();
    std::this_thread::sleep_for(std::chrono::seconds(3));
  } catch (const std::exception &e) {
    std::println("Exception occur: {}", e.what());
  }
}
