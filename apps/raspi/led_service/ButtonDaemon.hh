#pragma once

#include "gpiod.hpp"
#include <functional>
#include <thread>

namespace RPI4 {

class ButtonDaemon {
private:
  ::gpiod::line_request request_;

  enum class Button : uint8_t {
    Button1 = 16,
    Button2 = 26,
  };

  bool button1_pressed_ = false;
  bool button2_pressed_ = false;

  std::function<void()> callback_;

public:
  ButtonDaemon()
      : request_(::gpiod::chip("/dev/gpiochip0")
                     .prepare_request()
                     .set_consumer("raspi-led-pwm-dma")
                     .add_line_settings(
                         static_cast<int>(Button::Button1),
                         ::gpiod::line_settings()
                             .set_bias(::gpiod::line::bias::PULL_UP)
                             .set_direction(::gpiod::line::direction::INPUT)
                             .set_active_low(true))
                     .add_line_settings(
                         static_cast<int>(Button::Button2),
                         ::gpiod::line_settings()
                             .set_bias(::gpiod::line::bias::PULL_UP)
                             .set_direction(::gpiod::line::direction::INPUT)
                             .set_active_low(true))
                     .do_request()) {};
  virtual ~ButtonDaemon() = default;

  virtual auto onButton1Pressed() -> void = 0;

  virtual auto onButton2Pressed() -> void = 0;

  auto setCallback(std::function<void()> callback) -> void {
    callback_ = std::move(callback);
  }

  auto clearCallback() -> void { callback_ = nullptr; }

  auto run() -> void {
    while (true) {
      if (request_.get_value(static_cast<int>(Button::Button1)) ==
          ::gpiod::line::value::ACTIVE) {
        if (!button1_pressed_) {
          button1_pressed_ = true;
          onButton1Pressed();
        }
      } else {
        button1_pressed_ = false;
      }

      if (request_.get_value(static_cast<int>(Button::Button2)) ==
          ::gpiod::line::value::ACTIVE) {
        if (!button2_pressed_) {
          button2_pressed_ = true;
          onButton2Pressed();
        }
      } else {
        button2_pressed_ = false;
      }
      if (callback_) {
        callback_();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
};

}; // namespace RPI4
