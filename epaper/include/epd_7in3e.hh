#pragma once

#include <gpiod.hpp>

namespace Epaper {

enum class EPDColor : uint8_t {
  BLACK = 0x00,  /*!< Black */
  WHITE = 0x01,  /*!< White */
  YELLOW = 0x02, /*!< Yellow */
  RED = 0x03,    /*!< Red */
  BLUE = 0x05,   /*!< Blue */
  GREEN = 0x06,  /*!< Green */
};

using namespace std::chrono_literals;

class EPD7IN3E {
 private:
  const ::std::filesystem::path chip_path = "/dev/gpiochip0";

  static constexpr int EPD_RST_PIN = 17;
  static constexpr int EPD_DC_PIN = 25;
  static constexpr int EPD_PWR_PIN = 18;
  static constexpr int EPD_BUSY_PIN = 24;
  static constexpr int EPD_7IN3E_WIDTH = 800;
  static constexpr int EPD_7IN3E_HEIGHT = 480;
  ::gpiod::line_request request;

 public:
  EPD7IN3E();
  ~EPD7IN3E();

  auto clear(EPDColor color) -> void;
  auto display(uint8_t *Image) -> void;

 private:
  auto device_send_command_(uint8_t command) -> void;

  auto device_send_data_(uint8_t data) -> void;

  auto device_read_busy_() -> void;

  auto device_init_() -> void;

  auto device_turn_on_display_() -> void;

  auto device_sleep_() -> void;
};

};  // namespace Epaper
