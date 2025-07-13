#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>
#include <vector>


#include "LED.hh"

volatile bool running = true;

auto main() -> int {
  try {
    LED::LED led;
    led.initialize();
    std::vector<LED::RGB> off = {
        {.r = 0, .g = 0, .b = 0}, // Red
        {.r = 0, .g = 0, .b = 0}, // Green
        {.r = 0, .g = 0, .b = 0}, // Blue
        {.r = 0, .g = 0, .b = 0}, // Blue
        {.r = 0, .g = 0, .b = 0}, // Blue
        {.r = 0, .g = 0, .b = 0}, // Blue
        {.r = 0, .g = 0, .b = 0}, // Blue
    };
    std::vector<LED::RGB> colors = {
        {.r = 255, .g = 0, .b = 0},   // Red
        {.r = 127, .g = 127, .b = 0}, // Red
        {.r = 0, .g = 255, .b = 0},   // Red
        {.r = 0, .g = 127, .b = 127}, // Red
        {.r = 0, .g = 0, .b = 255},   // Red
        {.r = 127, .g = 0, .b = 127}, // Red
    };
    led.set(off);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (int i = 0; i < 1000; ++i) {
      size_t num_leds = 7;
      size_t offset = i / colors.size();
      std::vector<LED::RGB> data;
      for (size_t j = 0; j < num_leds; ++j) {
        LED::RGB color = colors[(j + offset) % colors.size()];
        data.push_back(color);
      }
      led.set(data);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    led.set(colors);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    return 1;
  }
  return 0;
}
