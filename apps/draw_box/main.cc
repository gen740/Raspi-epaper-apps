#include <cassert>
#include <cstdint>
#include <print>
#include <thread>
#include <vector>

#include "epd_7in3e.hh"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 480;
constexpr int PIXELS_PER_BYTE = 2;
constexpr int BUFFER_SIZE = (SCREEN_WIDTH * SCREEN_HEIGHT) / PIXELS_PER_BYTE;

using Buffer = std::vector<uint8_t>;

// pixel 単位の位置 (x, y) に value を描く
void set_pixel(Buffer &buf, int x, int y, uint8_t value) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  int index = (y * SCREEN_WIDTH + x) / 2;
  bool high = (x % 2 == 0);
  uint8_t byte = static_cast<uint8_t>(buf[index]);
  if (high) {
    byte = (byte & 0x0F) | (value << 4);
  } else {
    byte = (byte & 0xF0) | (value & 0x0F);
  }
  buf[index] = static_cast<int8_t>(byte);
}

Buffer draw_box(int margin, int width, uint8_t value) {
  assert(margin >= 0 && width > 0 && margin + width <= SCREEN_WIDTH / 2 &&
         margin + width <= SCREEN_HEIGHT / 2);

  Buffer buf(BUFFER_SIZE, 0);  // 初期化 (全ピクセル0)

  int x0 = margin;
  int x1 = SCREEN_WIDTH - margin - 1;
  int y0 = margin;
  int y1 = SCREEN_HEIGHT - margin - 1;

  // 上側と下側
  for (int y = y0; y < y0 + width; ++y) {
    for (int x = x0; x <= x1; ++x) {
      set_pixel(buf, x, y, value);              // 上辺
      set_pixel(buf, x, y1 - (y - y0), value);  // 下辺
    }
  }

  // 左側と右側
  for (int y = y0 + width; y <= y1 - width; ++y) {
    for (int x = x0; x < x0 + width; ++x) {
      set_pixel(buf, x, y, value);              // 左辺
      set_pixel(buf, x1 - (x - x0), y, value);  // 右辺
    }
  }

  return buf;
}

class EpaperDevice {
 private:
  EpaperDevice() : epd7in3e_() {}
  ~EpaperDevice() = default;

  Epaper::EPD7IN3E epd7in3e_;

 public:
  EpaperDevice(const EpaperDevice &) = delete;
  auto operator=(const EpaperDevice &) -> EpaperDevice & = delete;

  static auto get() -> EpaperDevice & {
    static EpaperDevice instance_;
    return instance_;
  }

  auto draw() -> void {
    epd7in3e_.display(draw_box(20, 3, 0x01)
                          .data()  // マージン10、幅20、色は白(0x0F)
    );
  }
};

auto main() -> int {
  try {
    std::println("Simple test Start!!");
    auto &epaper = EpaperDevice::get();
    epaper.draw();
    std::this_thread::sleep_for(std::chrono::seconds(100));
  } catch (const std::exception &e) {
    std::println("Exception occur: {}", e.what());
  }
}
