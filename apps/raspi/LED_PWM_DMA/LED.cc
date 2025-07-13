#include "LED.hh"

#include <algorithm>
#include <thread>

#include "DataGenerator.hh"
#include "Mailbox.hh"

using namespace std::chrono_literals;

namespace LED {

[[maybe_unused]] constexpr uint32_t PERI_BASE = 0xFE00'0000;
[[maybe_unused]] constexpr uint32_t GPIO_BASE = PERI_BASE + 0x200000;
[[maybe_unused]] constexpr uint32_t DMA_BASE = PERI_BASE + 0x007000;
[[maybe_unused]] constexpr uint32_t PWM_BASE = PERI_BASE + 0x20C000;
[[maybe_unused]] constexpr uint32_t PWM_CLK_BASE = PERI_BASE + 0x101000 + 0xA0;

[[maybe_unused]] constexpr uint32_t PERI_BUS_BASE = 0x7E000000;
[[maybe_unused]] constexpr uint32_t GPIO_BUS_BASE = PERI_BUS_BASE + 0x200000;
[[maybe_unused]] constexpr uint32_t PWM_BUS_BASE = PERI_BUS_BASE + 0x20C000;

LED::LED()
    : pwm_(PWM_BASE), clk_(0xFE1010A0), dma_(DMA_BASE), gpio_(GPIO_BASE) {}

LED::~LED() {
  dma_->chan[5].CS = RPI4::DMA_CS_RESET;
  pwm_->CTL ^= RPI4::PWM_CTL_PWEN1; // Disable PWM
}

auto LED::initialize() -> void {
  set_gpio_();
  set_pwm_clk_();
  set_pwm_();
};

auto LED::set(std::span<RGB> rgbs) -> void {
  dma_->chan[5].CS = 0;
  dma_->chan[5].TXFR_LEN = 0;
  auto pattern = DataGenerator<2048>().generate(rgbs);
  std::ranges::copy(pattern, const_cast<volatile uint32_t *>(dma_cb_->data));
  dma_cb_->ti = (RPI4::DMACB_TI_NO_WIDE_BURSTS | RPI4::DMACB_TI_SRC_INC |
                 RPI4::DMACB_TI_DEST_DREQ | RPI4::DMACB_TI_PERMAP_PWM |
                 RPI4::DMACB_TI_WAIT_RESP);
  dma_cb_->src = dma_cb_.get_bus_addr() + sizeof(uint32_t);
  dma_cb_->dst = PWM_BUS_BASE + 0x18;
  dma_cb_->len = pattern.size() * sizeof(uint32_t);
  dma_cb_->stride = 0;
  dma_cb_->next = 0;
  start_dma_();
}

auto LED::set_gpio_() -> void {
  gpio_->GPFSEL1 &= ~RPI4::GPIO_FSEL1(0b111);
  gpio_->GPFSEL1 |= RPI4::GPIO_FSEL1(RPI4::RPI_GPIO_ALT0);
}

auto LED::set_pwm_() -> void {
  pwm_->RNG1 = 32; // 32 bit per word
  std::this_thread::sleep_for(10us);
  pwm_->CTL = RPI4::PWM_CTL_CLRF;
  std::this_thread::sleep_for(10us);
  pwm_->DMAC = RPI4::PWM_DMAC_ENABLE | (7 * RPI4::PWM_DMAC_PANIC) |
               (3 * RPI4::PWM_DMAC_DREQ);
  std::this_thread::sleep_for(10us);
  pwm_->CTL = RPI4::PWM_CTL_PWEN1 | RPI4::PWM_CTL_MODE1 | RPI4::PWM_CTL_USEF1;
}

auto LED::set_pwm_clk_() -> void {
  pwm_->CTL = 0; // Clear PWM control
  std::this_thread::sleep_for(10us);

  clk_->CTL = RPI4::CLK_CTL_PASSWD | RPI4::CLK_CTL_KILL;
  std::this_thread::sleep_for(10us);
  while (clk_->CTL & RPI4::CLK_CTL_BUSY) {
    ;
  }
  clk_->DIV = RPI4::CLK_CTL_PASSWD | RPI4::CLK_DIV_DIVI(23) |
              RPI4::CLK_DIV_DIVF(0x1000 * 5 / 10);
  ;
  clk_->CTL = RPI4::CLK_CTL_PASSWD | RPI4::CLK_CTL_SRC(1) | RPI4::CLK_CTL_ENAB;
  std::this_thread::sleep_for(10us);
  while (!(clk_->CTL & RPI4::CLK_CTL_BUSY)) {
    ;
  }
}

auto LED::start_dma_() -> void {
  dma_->chan[5].CS = RPI4::DMA_CS_RESET;
  std::this_thread::sleep_for(10us);
  dma_->chan[5].CS =     //
      RPI4::DMA_CS_INT | //
      RPI4::DMA_CS_END;
  dma_->chan[5].CONBLK_AD = dma_cb_.get_phys_addr();
  dma_->chan[5].DEBUG = 7;
  std::this_thread::sleep_for(10us);
  dma_->chan[5].CS =                         //
      RPI4::DMA_CS_ACTIVE |                  //
      RPI4::DMA_CS_WAIT_OUTSTANDING_WRITES | //
      RPI4::DMA_CS_PRIORITY(15) |            //
      RPI4::DMA_CS_PANIC_PRIORITY(15);
  std::this_thread::sleep_for(10us);
}

}; // namespace LED
