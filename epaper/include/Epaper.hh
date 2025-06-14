#pragma once

#include <cstdint>

namespace Epaper {

void GPIOD_Export();

void DEV_GPIO_Init();

auto DEV_HARDWARE_SPI_Mode(int mode) -> int;

enum class SPIChipSelect : uint8_t {
  SPI_CS_Mode_LOW = 0,  /*!< Chip Select 0 */
  SPI_CS_Mode_HIGH = 1, /*!< Chip Select 1 */
  SPI_CS_Mode_NONE = 3  /*!< No CS, control it yourself */
};

auto DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect CS_Mode) -> int;

enum class SPIBitOrder {
  SPI_BIT_ORDER_LSBFIRST = 0, /*!< LSB First */
  SPI_BIT_ORDER_MSBFIRST = 1  /*!< MSB First */
};

auto DEV_HARDWARE_SPI_SetBitOrder(SPIBitOrder Order) -> int;

auto DEV_HARDWARE_SPI_setSpeed(uint32_t speed) -> int;

void DEV_HARDWARE_SPI_SetDataInterval(uint16_t us);

void DEV_HARDWARE_SPI_begin(char *SPI_device);

auto DEV_Module_Init() -> uint8_t;

// Epaper

auto DEV_HARDWARE_SPI_TransferByte(uint8_t buf) -> uint8_t;

void EPD_7IN3E_Init();

void EPD_7IN3E_Clear(uint8_t color);

void EPD_7IN3E_Sleep();

void DEV_HARDWARE_SPI_end();

void DEV_Module_Exit();

void EPD_7IN3E_Display(uint8_t *Image);

};  // namespace Epaper
