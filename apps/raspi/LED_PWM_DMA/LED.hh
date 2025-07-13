#pragma once

#include "Mailbox.hh"
#include "MemoryMap.hh"
#include "RPI4.hh"

#include <span>

namespace LED {

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class LED {

  RPI4::MemoryMap<RPI4::PWM> pwm_;
  RPI4::MemoryMap<RPI4::CLK> clk_;
  RPI4::MemoryMap<RPI4::DMA> dma_;
  RPI4::MemoryMap<RPI4::GPIO> gpio_;
  RPI4::GPUMemoryAllocator<RPI4::DMACB<2048>> dma_cb_;

private:
  auto set_gpio_() -> void;

  auto set_pwm_() -> void;

  auto set_pwm_clk_() -> void;

  auto start_dma_() -> void;

public:
  explicit LED();
  ~LED();

  auto initialize() -> void;

  auto set(std::span<RGB> rgbs) -> void;

  auto start() -> void { start_dma_(); }
};

} // namespace LED
