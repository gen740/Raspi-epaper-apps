
#include "epd_7in3e.hh"

#include <bcm2835.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdint>
#include <print>
#include <thread>

namespace Epaper {

EPD7IN3E::EPD7IN3E()
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
  request.set_value(EPD_PWR_PIN, gpiod::line::value::ACTIVE);
  if (bcm2835_init() != 1) {
    throw std::runtime_error("Failed to initialize SPI");
  }
  bcm2835_spi_begin();
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  bcm2835_spi_set_speed_hz(20000000);       // Default speed
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);  // Set chip select to CS0
  bcm2835_spi_set_speed_hz(10000000);
  device_init_();
}

EPD7IN3E::~EPD7IN3E() {
  clear(EPDColor::WHITE);
  bcm2835_spi_end();
  request.set_value(EPD_PWR_PIN, gpiod::line::value::INACTIVE)
      .set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE)
      .set_value(EPD_RST_PIN, gpiod::line::value::INACTIVE);
  device_sleep_();
}

auto EPD7IN3E::clear(EPDColor color) -> void {
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

auto EPD7IN3E::display(uint8_t *Image) -> void {
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

auto EPD7IN3E::device_send_command_(uint8_t command) -> void {
  request.set_value(EPD_DC_PIN, gpiod::line::value::INACTIVE);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);      // Set chip select to CS0
  bcm2835_spi_transfer(command);                // Send command via SPI
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);  // Set chip select to none
}

auto EPD7IN3E::device_send_data_(uint8_t data) -> void {
  request.set_value(EPD_DC_PIN, gpiod::line::value::ACTIVE);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);      // Set chip select to CS0
  bcm2835_spi_transfer(data);                   // Send data via SPI
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);  // Set chip select to none
}

auto EPD7IN3E::device_read_busy_() -> void {
  while (request.get_value(EPD_BUSY_PIN) == gpiod::line::value::INACTIVE) {
    std::this_thread::sleep_for(1ms);
  }
}

auto EPD7IN3E::device_init_() -> void {
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

auto EPD7IN3E::device_turn_on_display_() -> void {
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

auto EPD7IN3E::device_sleep_() -> void {
  device_send_command_(0x02);  // POWER_OFF
  device_send_data_(0X00);
  device_read_busy_();

  device_send_command_(0x07);  // DEEP_SLEEP
  device_send_data_(0xA5);
}

};  // namespace Epaper
