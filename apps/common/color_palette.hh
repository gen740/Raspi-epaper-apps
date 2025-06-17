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

    // {EPDColor::BLACK, {25, 15, 40}},    //
    // {EPDColor::WHITE, {160, 175, 185}}, //
    // {EPDColor::YELLOW, {170, 170, 0}},  //
    // {EPDColor::RED, {140, 0, 30}},      //
    // {EPDColor::BLUE, {20, 70, 150}},    //
    // {EPDColor::GREEN, {60, 100, 70}},   //

    //                  // {35, 35, 35},     //
    //                  // {160, 165, 165},  //
    //                  // {160, 160, 30},   //
    //                  // {145, 40, 40},    //
    //                  // {45, 90, 180},    //
    //                  // {65, 110, 65},
};

} // namespace Apps::Common
