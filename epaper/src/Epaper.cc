#include "Epaper.hh"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <gpiod.hpp>
#include <print>
#include <thread>

namespace Epaper {

using namespace std::chrono_literals;

constexpr int EPD_RST_PIN = 17;
constexpr int EPD_DC_PIN = 25;
constexpr int EPD_PWR_PIN = 18;
constexpr int EPD_BUSY_PIN = 24;
constexpr int EPD_7IN3E_WIDTH = 800;
constexpr int EPD_7IN3E_HEIGHT = 480;

using gpiod::line::direction;

const ::std::filesystem::path chip_path("/dev/gpiochip0");
std::unique_ptr<gpiod::line_request> request = {};

void GPIOD_Export() {
  ::gpiod::chip chip(chip_path);
  auto info = chip.get_info();

  ::std::cout << info.name() << " [" << info.label() << "] (" << info.num_lines() << " lines)"
              << ::std::endl;

  std::println("GPIO chip opening");

  static auto request_ =
      ::gpiod::chip(chip_path)
          .prepare_request()
          .set_consumer("get-line-value")
          .add_line_settings(EPD_BUSY_PIN, gpiod::line_settings().set_direction(direction::INPUT))
          .add_line_settings(EPD_RST_PIN, gpiod::line_settings().set_direction(direction::OUTPUT))
          .add_line_settings(EPD_DC_PIN, gpiod::line_settings().set_direction(direction::OUTPUT))
          .add_line_settings(EPD_PWR_PIN, gpiod::line_settings().set_direction(direction::OUTPUT))
          .do_request();
  request = std::unique_ptr<gpiod::line_request>(&request_);
  std::println("Request prepared");
}

void DEV_GPIO_Init() {
  DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_HIGH);
  std::println("EPD_CS_PIN set to ACTIVE");

  request->set_value(EPD_PWR_PIN, gpiod::line::value::ACTIVE);
  std::println("EPD_PWR_PIN set to ACTIVE");
}

// SPI

struct HARDWARE_SPI {
  // GPIO
  uint16_t SCLK_PIN;
  uint16_t MOSI_PIN;
  uint16_t MISO_PIN;

  uint16_t CS0_PIN;
  uint16_t CS1_PIN;

  uint32_t speed;
  uint16_t mode;
  uint16_t delay;
  int fd;  //
};

HARDWARE_SPI hardware_SPI;

static uint8_t bits = 8;
struct spi_ioc_transfer tr;

auto DEV_HARDWARE_SPI_Mode(int mode) -> int {
  hardware_SPI.mode &= 0xfC;  // Clear low 2 digits
  hardware_SPI.mode |= mode;  // Setting mode
                              //
  if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
    std::println("can't set spi mode");
    return -1;
  }
  return 1;
}

auto DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect CS_Mode) -> int {
  switch (CS_Mode) {
    case SPIChipSelect::SPI_CS_Mode_HIGH:
      hardware_SPI.mode |= SPI_CS_HIGH;
      hardware_SPI.mode &= ~SPI_NO_CS;
      break;
    case SPIChipSelect::SPI_CS_Mode_LOW:
      hardware_SPI.mode &= ~SPI_CS_HIGH;
      hardware_SPI.mode &= ~SPI_NO_CS;
      break;
    case SPIChipSelect::SPI_CS_Mode_NONE:
      hardware_SPI.mode |= SPI_NO_CS;
      break;
  }

  if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
    std::println("can't set spi mode");
    return -1;
  }
  return 1;
}

auto DEV_HARDWARE_SPI_SetBitOrder(SPIBitOrder Order) -> int {
  switch (Order) {
    case SPIBitOrder::SPI_BIT_ORDER_LSBFIRST:
      hardware_SPI.mode |= SPI_LSB_FIRST;
      std::println("SPI_LSB_FIRST");
      break;
    case SPIBitOrder::SPI_BIT_ORDER_MSBFIRST:
      hardware_SPI.mode &= ~SPI_LSB_FIRST;
      std::println("SPI_MSB_FIRST");
      break;
  }
  int fd = ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode);
  std::println("fd = {}", fd);
  if (fd == -1) {
    std::println("can't set spi SPI_LSB_FIRST");
    return -1;
  }
  return 1;
}

auto DEV_HARDWARE_SPI_setSpeed(uint32_t speed) -> int {
  uint32_t speed1 = hardware_SPI.speed;
  hardware_SPI.speed = speed;
  if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
    std::println("can't set max speed hz");
    hardware_SPI.speed = speed1;  // Setting failure rate unchanged
    return -1;
  }
  if (ioctl(hardware_SPI.fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
    std::println("can't get max speed hz");
    hardware_SPI.speed = speed1;  // Setting failure rate unchanged
    return -1;
  }
  hardware_SPI.speed = speed;
  tr.speed_hz = hardware_SPI.speed;
  return 1;
}

void DEV_HARDWARE_SPI_SetDataInterval(uint16_t us) {
  hardware_SPI.delay = us;
  tr.delay_usecs = hardware_SPI.delay;
}

void DEV_HARDWARE_SPI_begin(const char *SPI_device) {
  hardware_SPI.fd = open(SPI_device, O_RDWR);
  hardware_SPI.mode = 0;

  int ret;
  ret = ioctl(hardware_SPI.fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1) {
    std::println("can't set bits per word");
  }

  ret = ioctl(hardware_SPI.fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1) {
    std::println("can't set bits per word");
  }
  tr.bits_per_word = bits;

  DEV_HARDWARE_SPI_Mode(SPI_MODE_0);
  DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_LOW);
  DEV_HARDWARE_SPI_SetBitOrder(SPIBitOrder::SPI_BIT_ORDER_LSBFIRST);
  DEV_HARDWARE_SPI_setSpeed(20000000);
  DEV_HARDWARE_SPI_SetDataInterval(5);
}

auto DEV_Module_Init() -> uint8_t {
  std::println("/***********************************/");
  // if (DEV_Equipment_Testing() < 0) {
  //   return 1;
  // }
  // printf("Write and read /dev/spidev0.0 \r\n");
  GPIOD_Export();
  std::println("GPIOD_Exported");
  DEV_GPIO_Init();
  std::println("DEV_GPIO_Init");
  DEV_HARDWARE_SPI_begin("/dev/spidev0.0");
  std::println("DEV_HARDWARE_SPI_begin");
  DEV_HARDWARE_SPI_setSpeed(10000000);
  std::println("DEV_HARDWARE_SPI_setSpeed");
  printf("/***********************************/ \r\n");
  return 0;
}

// Epaper

static void EPD_7IN3E_Reset() {
  request->set_value(EPD_RST_PIN, gpiod::line::value::ACTIVE);
  std::this_thread::sleep_for(20ms);
  request->set_value(EPD_RST_PIN, gpiod::line::value::INACTIVE);
  // .wait_edge_events(-1ns);
  std::this_thread::sleep_for(20ms);
  request->set_value(EPD_RST_PIN, gpiod::line::value::ACTIVE);
  std::this_thread::sleep_for(20ms);
}

static void EPD_7IN3E_ReadBusyH() {
  auto value = request->get_value(EPD_BUSY_PIN);
  while (value == gpiod::line::value::INACTIVE) {  // LOW: busy, HIGH: idle
    value = request->get_value(EPD_BUSY_PIN);
    std::this_thread::sleep_for(1ms);
  }
}

auto DEV_HARDWARE_SPI_TransferByte(uint8_t buf) -> uint8_t {
  uint8_t rbuf[1];
  tr.len = 1;
  tr.tx_buf = (unsigned long)&buf;
  tr.rx_buf = (unsigned long)rbuf;

  // ioctl Operation, transmission of data
  if (ioctl(hardware_SPI.fd, SPI_IOC_MESSAGE(1), &tr) < 1) std::println("can't send spi message");
  return rbuf[0];
}

static void EPD_7IN3E_SendCommand(uint8_t Reg) {
  // DEV_Digital_Write(EPD_DC_PIN, 0);
  // DEV_Digital_Write(EPD_CS_PIN, 0);
  // DEV_SPI_WriteByte(Reg);
  // DEV_Digital_Write(EPD_CS_PIN, 1);
  request->set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE);
  // DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_LOW);
  DEV_HARDWARE_SPI_TransferByte(Reg);
  // DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_HIGH);
}

static void EPD_7IN3E_SendData(uint8_t Data) {
  // DEV_Digital_Write(EPD_DC_PIN, 1);
  // DEV_Digital_Write(EPD_CS_PIN, 0);
  // DEV_SPI_WriteByte(Data);
  // DEV_Digital_Write(EPD_CS_PIN, 1);
  request->set_value(EPD_DC_PIN, gpiod::line::value::ACTIVE);
  // DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_LOW);
  DEV_HARDWARE_SPI_TransferByte(Data);
  // DEV_HARDWARE_SPI_ChipSelect(SPIChipSelect::SPI_CS_Mode_HIGH);
}

void EPD_7IN3E_Init() {
  EPD_7IN3E_Reset();
  EPD_7IN3E_ReadBusyH();
  std::println("e-Paper Init and Clear...");

  EPD_7IN3E_SendCommand(0xAA);  // CMDH
  EPD_7IN3E_SendData(0x49);
  EPD_7IN3E_SendData(0x55);
  EPD_7IN3E_SendData(0x20);
  EPD_7IN3E_SendData(0x08);
  EPD_7IN3E_SendData(0x09);
  EPD_7IN3E_SendData(0x18);

  EPD_7IN3E_SendCommand(0x01);  //
  EPD_7IN3E_SendData(0x3F);

  EPD_7IN3E_SendCommand(0x00);
  EPD_7IN3E_SendData(0x5F);
  EPD_7IN3E_SendData(0x69);

  EPD_7IN3E_SendCommand(0x03);
  EPD_7IN3E_SendData(0x00);
  EPD_7IN3E_SendData(0x54);
  EPD_7IN3E_SendData(0x00);
  EPD_7IN3E_SendData(0x44);

  EPD_7IN3E_SendCommand(0x05);
  EPD_7IN3E_SendData(0x40);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x2C);

  EPD_7IN3E_SendCommand(0x06);
  EPD_7IN3E_SendData(0x6F);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x17);
  EPD_7IN3E_SendData(0x49);

  EPD_7IN3E_SendCommand(0x08);
  EPD_7IN3E_SendData(0x6F);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x22);

  EPD_7IN3E_SendCommand(0x30);
  EPD_7IN3E_SendData(0x03);

  EPD_7IN3E_SendCommand(0x50);
  EPD_7IN3E_SendData(0x3F);

  EPD_7IN3E_SendCommand(0x60);
  EPD_7IN3E_SendData(0x02);
  EPD_7IN3E_SendData(0x00);

  EPD_7IN3E_SendCommand(0x61);
  EPD_7IN3E_SendData(0x03);
  EPD_7IN3E_SendData(0x20);
  EPD_7IN3E_SendData(0x01);
  EPD_7IN3E_SendData(0xE0);

  EPD_7IN3E_SendCommand(0x84);
  EPD_7IN3E_SendData(0x01);

  EPD_7IN3E_SendCommand(0xE3);
  EPD_7IN3E_SendData(0x2F);

  EPD_7IN3E_SendCommand(0x04);  // PWR on
  EPD_7IN3E_ReadBusyH();        // waiting for the electronic paper IC to release the
                                // idle signal
}

static void EPD_7IN3E_TurnOnDisplay() {
  EPD_7IN3E_SendCommand(0x04);  // POWER_ON
  EPD_7IN3E_ReadBusyH();

  // Second setting
  EPD_7IN3E_SendCommand(0x06);
  EPD_7IN3E_SendData(0x6F);
  EPD_7IN3E_SendData(0x1F);
  EPD_7IN3E_SendData(0x17);
  EPD_7IN3E_SendData(0x49);

  EPD_7IN3E_SendCommand(0x12);  // DISPLAY_REFRESH
  EPD_7IN3E_SendData(0x00);
  EPD_7IN3E_ReadBusyH();

  EPD_7IN3E_SendCommand(0x02);  // POWER_OFF
  EPD_7IN3E_SendData(0X00);
  EPD_7IN3E_ReadBusyH();
}

void EPD_7IN3E_Clear(uint8_t color) {
  uint16_t Width, Height;
  Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
  Height = EPD_7IN3E_HEIGHT;

  EPD_7IN3E_SendCommand(0x10);
  for (uint16_t j = 0; j < Height; j++) {
    for (uint16_t i = 0; i < Width; i++) {
      EPD_7IN3E_SendData((color << 4) | color);
    }
  }

  EPD_7IN3E_TurnOnDisplay();
}

void EPD_7IN3E_Sleep() {
  EPD_7IN3E_SendCommand(0X02);  // DEEP_SLEEP
  EPD_7IN3E_SendData(0x00);
  EPD_7IN3E_ReadBusyH();

  EPD_7IN3E_SendCommand(0x07);  // DEEP_SLEEP
  EPD_7IN3E_SendData(0XA5);
}

void DEV_HARDWARE_SPI_end() {
  hardware_SPI.mode = 0;
  std::println("DEV_HARDWARE_SPI_end");
  if (::close(hardware_SPI.fd) != 0) {
    std::println("Failed to close SPI device");
    perror("Failed to close SPI device.\n");
  }
}

void DEV_Module_Exit() {
  DEV_HARDWARE_SPI_end();
  request->set_value(EPD_PWR_PIN, gpiod::line::value::INACTIVE)
      .set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE)
      .set_value(EPD_RST_PIN, gpiod::line::value::INACTIVE);
  std::println("GPIOD lines set to INACTIVE");
}

void EPD_7IN3E_Display(uint8_t *Image) {
  uint16_t Width, Height;
  Width = (EPD_7IN3E_WIDTH % 2 == 0) ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1);
  Height = EPD_7IN3E_HEIGHT;

  EPD_7IN3E_SendCommand(0x10);
  for (uint16_t j = 0; j < Height; j++) {
    for (uint16_t i = 0; i < Width; i++) {
      EPD_7IN3E_SendData(Image[i + j * Width]);
    }
  }
  EPD_7IN3E_TurnOnDisplay();
}

}  // namespace Epaper
