#pragma once

#include <array>
#include <map>

namespace Apps::Common {

enum class EPDColor {
  BLACK = 0x00,
  WHITE = 0x01,
  YELLOW = 0x02,
  RED = 0x03,
  BLUE = 0x05,
  GREEN = 0x06,
};

using Color = std::array<uint8_t, 3>;
const std::map<EPDColor, Color> PALETTE = {
    {EPDColor::BLACK, {0, 0, 0}},       //
    {EPDColor::WHITE, {255, 255, 255}}, //
    {EPDColor::YELLOW, {255, 255, 0}},  //
    {EPDColor::RED, {255, 0, 0}},       //
    {EPDColor::BLUE, {0, 0, 255}},      //
    {EPDColor::GREEN, {0, 255, 0}},     //
};

} // namespace Apps::Common
