#include <algorithm>
#include <array>
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

#include "ButtonDaemon.hh"
#include "LED.hh"

// volatile bool running = true;

class Service final : public RPI4::ButtonDaemon {
  LED::LED led;

  enum class State : uint8_t {
    Gaming,
    Red,
    Green,
    Blue,
    White,
    DimWhite,
    FaintWhite,
    Daylight,
    DimDaylight,
    FaintDaylight,
    Candle,
    DimCandle,
    FaintCandle,
    Last
  } state_ = State::Gaming;

  bool led_is_lit_ = false;

  [[nodiscard]] auto getStateString(State state) const -> std::string {
    switch (state) {
    case State::Red:
      return "Red";
    case State::Green:
      return "Green";
    case State::Blue:
      return "Blue";
    case State::White:
      return "White";
    case State::DimWhite:
      return "Dim White";
    case State::FaintWhite:
      return "Faint White";
    case State::Daylight:
      return "Daylight";
    case State::DimDaylight:
      return "Dim Daylight";
    case State::FaintDaylight:
      return "Faint Daylight";
    case State::Candle:
      return "Candle";
    case State::DimCandle:
      return "Dim Candle";
    case State::FaintCandle:
      return "Faint Candle";
    case State::Gaming:
      return "Gaming";
    default:
      return "Unknown";
    }
  }

public:
  Service() : RPI4::ButtonDaemon(), led() { led.initialize(); }

  auto onButton1Pressed() -> void final {
    state_ = static_cast<State>((static_cast<uint8_t>(state_) + 1) %
                                static_cast<uint8_t>(State::Last));
    std::println("Button 1 pressed, changing state to: {}",
                 getStateString(state_));
    clearCallback();
    setLEDBasedonState();
    led_is_lit_ = true;
  }

  auto setLEDBasedonState() -> void {
    std::array<LED::RGB, 34> colors; // All LEDs off
    switch (state_) {
    case State::Red:
      colors.fill({255, 0, 0});
      break;
    case State::Green:
      colors.fill({0, 255, 0});
      break;
    case State::Blue:
      colors.fill({0, 0, 255});
      break;
    case State::White:
      colors.fill({255, 255, 255});
      break;
    case State::DimWhite:
      colors.fill({128, 128, 128}); // Dim white
      break;
    case State::FaintWhite:
      colors.fill({32, 32, 32}); // Faint white
      break;
    case State::Daylight:
      colors.fill({255, 223, 186}); // Daylight color
      break;
    case State::DimDaylight:
      colors.fill({128, 112, 93}); // Dim daylight color
      break;
    case State::FaintDaylight:
      colors.fill({32, 28, 23}); // Faint daylight color
      break;
    case State::Candle:
      colors.fill({255, 153, 51}); // Candle color
      break;
    case State::DimCandle:
      colors.fill({128, 76, 25}); // Dim candle color
      break;
    case State::FaintCandle:
      colors.fill({32, 19, 6}); // Faint candle color
      break;
    case State::Gaming: {
      std::array<LED::RGB, 6> game_colors;
      game_colors[0] = {.r = 255, .g = 0, .b = 0};   // Red
      game_colors[1] = {.r = 125, .g = 125, .b = 0}; // Red
      game_colors[2] = {.r = 0, .g = 255, .b = 0};   // Green
      game_colors[3] = {.r = 0, .g = 125, .b = 125}; // Green
      game_colors[4] = {.r = 0, .g = 0, .b = 255};   // Blue
      game_colors[5] = {.r = 125, .g = 0, .b = 125}; // Blue
      for (size_t i = 0; i < colors.size(); ++i) {
        colors[i] = game_colors[i % game_colors.size()];
      }
      setCallback([this, colors]() {
        static std::array<LED::RGB, 34> colors_ = colors;
        static auto counter = 0ULL;
        counter++;
        if (counter % 8 == 0) { // Rotate every 10 calls
          std::ranges::rotate(colors_, colors_.begin() + 1);
          led.set(colors_);
        }
      });
      break;
    }
    default:
      break; // Off state does nothing
    }
    led.set(colors);
  }

  auto onButton2Pressed() -> void final {
    clearCallback();
    if (led_is_lit_) {
      std::println("Button 2 pressed, turning off LED.");
      std::array<LED::RGB, 34> off_colors;
      off_colors.fill({0, 0, 0}); // All LEDs off
      led.set(off_colors);
      led_is_lit_ = false;
    } else {
      std::println("Button 2 pressed, turning on LED.");
      setLEDBasedonState(); // Reapply the current state color
      led_is_lit_ = true;
    }
  }
};

auto main() -> int {
  try {
    Service().run();
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    return 1;
  }
  return 0;
}
