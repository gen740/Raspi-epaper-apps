#pragma once

#include <bcm2835.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdint>
#include <gpiod.hpp>
#include <print>
#include <thread>

namespace Epaper {

enum class SPIChipSelect : uint8_t {
  CS_LOW = 0,  /*!< Chip Select 0 */
  CS_HIGH = 1, /*!< Chip Select 1 */
  CS_NONE = 3  /*!< No CS, control it yourself */
};

enum class SPIBitOrder {
  LSBFIRST = 0, /*!< LSB First */
  MSBFIRST = 1  /*!< MSB First */
};

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
  int spi_fd_ = 0;
  uint64_t spi_mode_ = 0;
  spi_ioc_transfer spi_tr_ = {.tx_buf = 0,
                              .rx_buf = 0,
                              .len = 0,
                              .speed_hz = 20000000,
                              .delay_usecs = 5,
                              .bits_per_word = 8,
                              .cs_change = 0,
                              .tx_nbits = 0,
                              .rx_nbits = 0,
                              .word_delay_usecs = 0,
                              .pad = 0};

 public:
  EPD7IN3E()
      : request(::gpiod::chip(chip_path)
                    .prepare_request()
                    .set_consumer("get-line-value")
                    .add_line_settings(EPD_BUSY_PIN, ::gpiod::line_settings().set_direction(
                                                         ::gpiod::line::direction::INPUT))
                    .add_line_settings(EPD_RST_PIN, ::gpiod::line_settings().set_direction(
                                                        ::gpiod::line::direction::OUTPUT))
                    .add_line_settings(EPD_DC_PIN, ::gpiod::line_settings().set_direction(
                                                       ::gpiod::line::direction::OUTPUT))
                    .add_line_settings(EPD_PWR_PIN, ::gpiod::line_settings().set_direction(
                                                        ::gpiod::line::direction::OUTPUT))
                    .do_request()) {
    set_spi_chipselect_(SPIChipSelect::CS_HIGH);
    request.set_value(EPD_PWR_PIN, gpiod::line::value::ACTIVE);
    spi_begin_("/dev/spidev0.0");
    set_spi_speed_(10000000);
    device_init_();
  }

  ~EPD7IN3E() {
    clear(EPDColor::WHITE);
    if (::close(spi_fd_) != 0) {
      std::println("Failed to close SPI device");
    }
    request.set_value(EPD_PWR_PIN, gpiod::line::value::INACTIVE)
        .set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE)
        .set_value(EPD_RST_PIN, gpiod::line::value::INACTIVE);
    device_sleep_();
  }

  auto clear(EPDColor color) -> void {
    uint16_t Width = (EPD_7IN3E_WIDTH / 2);
    uint16_t Height = EPD_7IN3E_HEIGHT;

    device_send_command_(0x10);  // Write RAM
    for (uint16_t j = 0; j < Height; j++) {
      for (uint16_t i = 0; i < Width; i++) {
        device_send_data_(
            static_cast<uint8_t>((static_cast<uint8_t>(color) << 4) | static_cast<uint8_t>(color)));
      }
    }
    device_turn_on_display_();
  }

  auto display(uint8_t *Image) -> void {
    uint16_t Width = (EPD_7IN3E_WIDTH / 2);
    uint16_t Height = EPD_7IN3E_HEIGHT;

    device_send_command_(0x10);  // Write RAM
    for (uint16_t j = 0; j < Height; j++) {
      for (uint16_t i = 0; i < Width; i++) {
        device_send_data_(Image[i + j * Width]);
      }
    }
    device_turn_on_display_();
  }

 private:
  auto set_spi_chipselect_(SPIChipSelect CS_Mode) -> int {
    switch (CS_Mode) {
      case SPIChipSelect::CS_HIGH:
        spi_mode_ |= SPI_CS_HIGH;
        break;

      case SPIChipSelect::CS_LOW:
        spi_mode_ &= ~SPI_CS_HIGH;
        break;

      case SPIChipSelect::CS_NONE:
        spi_mode_ |= SPI_NO_CS;
        break;
    }
    if (ioctl(spi_fd_, SPI_IOC_WR_MODE, &spi_mode_) == -1) {
      std::println("can't set spi mode (chip select)");
      return -1;
    }
    return 1;
  }

  auto set_spi_mode_(uint8_t mode) -> int {
    spi_mode_ &= 0xFC;
    spi_mode_ |= mode;
    if (ioctl(spi_fd_, SPI_IOC_WR_MODE, &spi_mode_) == -1) {
      std::println("can't set spi mode");
      return -1;
    }
    return 1;
  }

  auto set_spi_data_interval_(uint16_t us) -> void { spi_tr_.delay_usecs = us; }

  auto set_spi_bit_order_(SPIBitOrder order) -> int {
    switch (order) {
      case SPIBitOrder::LSBFIRST:
        bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_LSBFIRST);
        break;
      case SPIBitOrder::MSBFIRST:
        bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
        break;
    }
    return 1;
  }

  auto set_spi_speed_(uint32_t speed) -> void { bcm2835_spi_set_speed_hz(speed); }

  auto spi_begin_(const char *spi_device) -> int {
    spi_fd_ = ::open(spi_device, O_RDWR);
    if (spi_fd_ < 0) {
      throw std::runtime_error("Failed to open SPI device");
    }
    if (ioctl(spi_fd_, SPI_IOC_WR_BITS_PER_WORD, &spi_tr_.bits_per_word) == -1) {
      throw std::runtime_error("Failed to set bits per word");
    }
    if (ioctl(spi_fd_, SPI_IOC_RD_BITS_PER_WORD, &spi_tr_.bits_per_word) == -1) {
      throw std::runtime_error("Failed to read bits per word");
    }
    // set_spi_mode_(SPI_MODE_0);

    if (bcm2835_init() != 1) {
      throw std::runtime_error("Failed to initialize SPI");
    }
    bcm2835_spi_begin();
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    // set_spi_bit_order_(SPIBitOrder::LSBFIRST);
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_LSBFIRST);
    set_spi_speed_(20000000);   // Default speed
    set_spi_data_interval_(5);  // Default delay in microseconds
    set_spi_chipselect_(SPIChipSelect::CS_LOW);
    return 0;
  }

  auto device_send_command_(uint8_t command) -> void {
    request.set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE);
    set_spi_chipselect_(SPIChipSelect::CS_LOW);
    spi_tr_.tx_buf = reinterpret_cast<uint64_t>(&command);
    spi_tr_.len = 1;
    if (ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &spi_tr_) < 1) {
      std::println("Failed to send SPI command");
    }
    set_spi_chipselect_(SPIChipSelect::CS_HIGH);
  }

  auto device_send_data_(uint8_t data) -> void {
    request.set_value(EPD_DC_PIN, gpiod::line::value::ACTIVE);
    set_spi_chipselect_(SPIChipSelect::CS_LOW);
    spi_tr_.tx_buf = reinterpret_cast<uint64_t>(&data);
    spi_tr_.len = 1;
    if (ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &spi_tr_) < 1) {
      std::println("Failed to send SPI data");
    }
    set_spi_chipselect_(SPIChipSelect::CS_HIGH);
  }

  auto device_read_busy_() -> void {
    while (request.get_value(EPD_BUSY_PIN) == gpiod::line::value::INACTIVE) {
      std::this_thread::sleep_for(1ms);
    }
  }

  auto device_init_() -> void {
    request.set_value(EPD_RST_PIN, gpiod::line::value::ACTIVE)
        .wait_edge_events(20ms);  // Wait for the line to be active
    request.set_value(EPD_RST_PIN, gpiod::line::value::INACTIVE)
        .wait_edge_events(20ms);  // Wait for the line to be inactive
    request.set_value(EPD_RST_PIN, gpiod::line::value::ACTIVE)
        .wait_edge_events(20ms);  // Wait for the line to be active again

    device_read_busy_();
    std::println("e-Paper Init and Clear...");

    device_send_command_(0xAA);  // CMDH
    device_send_data_(0x49);
    device_send_data_(0x55);
    device_send_data_(0x20);
    device_send_data_(0x08);
    device_send_data_(0x09);
    device_send_data_(0x18);

    device_send_command_(0x01);  //
    device_send_data_(0x3F);

    device_send_command_(0x00);
    device_send_data_(0x5F);
    device_send_data_(0x69);

    device_send_command_(0x03);
    device_send_data_(0x00);
    device_send_data_(0x54);
    device_send_data_(0x00);
    device_send_data_(0x44);

    device_send_command_(0x05);
    device_send_data_(0x40);
    device_send_data_(0x1F);
    device_send_data_(0x1F);
    device_send_data_(0x2C);

    device_send_command_(0x06);
    device_send_data_(0x6F);
    device_send_data_(0x1F);
    device_send_data_(0x17);
    device_send_data_(0x49);

    device_send_command_(0x08);
    device_send_data_(0x6F);
    device_send_data_(0x1F);
    device_send_data_(0x1F);
    device_send_data_(0x22);

    device_send_command_(0x30);
    device_send_data_(0x03);

    device_send_command_(0x50);
    device_send_data_(0x3F);

    device_send_command_(0x60);
    device_send_data_(0x02);
    device_send_data_(0x00);

    device_send_command_(0x61);
    device_send_data_(0x03);
    device_send_data_(0x20);
    device_send_data_(0x01);
    device_send_data_(0xE0);

    device_send_command_(0x84);
    device_send_data_(0x01);

    device_send_command_(0xE3);
    device_send_data_(0x2F);

    device_send_command_(0x04);  // PWR on
    device_read_busy_();         // waiting for the electronic paper IC to release the idle signal
  }

  auto device_turn_on_display_() -> void {
    device_send_command_(0x04);  // POWER_ON
    device_read_busy_();

    // Second setting
    device_send_command_(0x06);
    device_send_data_(0x6F);
    device_send_data_(0x1F);
    device_send_data_(0x17);
    device_send_data_(0x49);

    device_send_command_(0x12);  // DISPLAY_REFRESH
    device_send_data_(0x00);
    device_read_busy_();

    device_send_command_(0x02);  // POWER_OFF
    device_send_data_(0X00);
    device_read_busy_();
  }

  auto device_sleep_() -> void {
    device_send_command_(0x02);  // POWER_OFF
    device_send_data_(0X00);
    device_read_busy_();

    device_send_command_(0x07);  // DEEP_SLEEP
    device_send_data_(0xA5);
  }
};

};  // namespace Epaper
