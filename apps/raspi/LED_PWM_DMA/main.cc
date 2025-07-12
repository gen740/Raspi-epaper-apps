#include <fcntl.h>
#include <functional>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>
#include <vector>

#include "Mailbox.hh"
#include "MemoryMap.hh"
#include "RPI4.hh"

[[maybe_unused]] constexpr uint32_t PERI_BASE = 0xFE00'0000;
[[maybe_unused]] constexpr uint32_t GPIO_BASE = PERI_BASE + 0x200000;
[[maybe_unused]] constexpr uint32_t DMA_BASE = PERI_BASE + 0x007000;
[[maybe_unused]] constexpr uint32_t PWM_BASE = PERI_BASE + 0x20C000;
[[maybe_unused]] constexpr uint32_t PWM_CLK_BASE = PERI_BASE + 0x101000 + 0xA0;

[[maybe_unused]] constexpr uint32_t PERI_BUS_BASE = 0x7E000000;
[[maybe_unused]] constexpr uint32_t GPIO_BUS_BASE = PERI_BUS_BASE + 0x200000;
[[maybe_unused]] constexpr uint32_t PWM_BUS_BASE = PERI_BUS_BASE + 0x20C000;

// auto GPIO_FSEL1 = [](auto fsel) consteval -> auto { return fsel << 6; };

volatile bool running = true;

class LED {

  MemoryMapCast<RPI4::PWM> pwm_;
  MemoryMapCast<RPI4::CLK> clk_;
  MemoryMapCast<RPI4::DMA> dma_;
  MemoryMapCast<RPI4::GPIO> gpio_;
  MemoryMapCast<RPI4::DMACB<780>> dma_cb_;

public:
  LED() : pwm_(PWM_BASE), clk_(0xFE1010A0), dma_(DMA_BASE), gpio_(GPIO_BASE) {

    std::println("\nStep 1: Allocate DMA memory");
    std::vector<uint32_t> pattern;
    for (int i = 0; i < 390; i++) {
      pattern.push_back(0xFFFFFFFF);
    }
    for (int i = 0; i < 390; i++) {
      pattern.push_back(0x00000000);
    }
    size_t pattern_size = pattern.size() * sizeof(uint32_t);
    total_size = sizeof(RPI4::DMACB<780>);

    std::println("Total size for DMA memory: {} bytes", total_size);
    handle_ = allocate_memory(total_size, 0xC);
    bus_addr_ = lock_memory(handle_);
    uint32_t phys_addr = bus_addr_ & 0x3FFFFFFF;

    // PWM設定
    // pwm->RNG1 = 32; // 32 bit per word

    pwm_->CTL = 0;                     // Stop PWM
    clk_->CTL = 0x5A000000 | (1 << 5); // Kill clock
    usleep(10);
    while (clk_->CTL & (1 << 7)) {
      usleep(1000);
    }

    pwm_->RNG1 = 32; // 32 bit per word
    usleep(10);
    pwm_->CTL = RPI4::PWM_CTL_CLRF;
    usleep(10);
    pwm_->DMAC = RPI4::PWM_DMAC_ENABLE | (7 * RPI4::PWM_DMAC_PANIC) |
                 (3 * RPI4::PWM_DMAC_DREQ);
    usleep(10);
    pwm_->CTL = RPI4::PWM_CTL_PWEN1 | RPI4::PWM_CTL_MODE1 | RPI4::PWM_CTL_USEF1;

    dma_->chan[5].CS = 0;
    dma_->chan[5].TXFR_LEN = 0;

    // // DMAメモリをマップ
    // int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    // dma_mem = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
    //                mem_fd, phys_addr);
    // if (mem_fd >= 0) {
    //   close(mem_fd);
    // }
    // if (dma_mem == MAP_FAILED) {
    //   perror("mmap DMA memory");
    //   throw std::runtime_error("Failed to mmap DMA memory");
    // }
    //
    // // Control Blockとデータの設定
    // auto *cb = (RPI4::DMACB *)dma_mem;
    // auto *data = (uint32_t *)((char *)dma_mem + sizeof(RPI4::DMACB));
    dma_cb_.reset(phys_addr);

    // パターンデータをコピー
    memcpy((uint32_t *)dma_cb_->data, pattern.data(), pattern_size);

    std::println("\nStep 3: Setup DMA Control Blocks");

    // Control Block 0: GPIO SET（0.5秒分）
    dma_cb_->ti = (RPI4::DMACB_TI_NO_WIDE_BURSTS | RPI4::DMACB_TI_SRC_INC |
                   RPI4::DMACB_TI_DEST_DREQ | RPI4::DMACB_TI_PERMAP_PWM |
                   RPI4::DMACB_TI_WAIT_RESP);
    dma_cb_->src = bus_addr_ + 4;
    dma_cb_->dst = PWM_BUS_BASE + 0x18;
    dma_cb_->len = pattern.size() * sizeof(uint32_t);
    dma_cb_->stride = 0;
    dma_cb_->next = bus_addr_;

    // GPIO Setting
    std::println("\nStep 4: Setup GPIO for PWM");
    gpio_->GPFSEL1 &= ~RPI4::GPIO_FSEL1(0b111);
    gpio_->GPFSEL1 |= RPI4::GPIO_FSEL1(RPI4::RPI_GPIO_ALT0);

    // PWM Clock Setting
    std::println("\nStep 5: Setup PWM Clock");
    clk_->CTL = RPI4::CLK_CTL_PASSWD | RPI4::CLK_CTL_KILL;
    while (clk_->CTL & RPI4::CLK_CTL_BUSY) {
      usleep(1000);
    }
    clk_->DIV = RPI4::CLK_CTL_PASSWD | ((540 * 4) << 12);
    clk_->CTL =
        RPI4::CLK_CTL_PASSWD | RPI4::CLK_CTL_SRC(1) | RPI4::CLK_CTL_ENAB;
    while (clk_->CTL & RPI4::CLK_CTL_BUSY) {
      usleep(1000);
    }

    // DMA
    std::println("\nStep 5: Start DMA");
    dma_->chan[5].CS = RPI4::DMA_CS_RESET;
    usleep(10);

    dma_->chan[5].CS =     //
        RPI4::DMA_CS_INT | //
        RPI4::DMA_CS_END;
    usleep(10);

    dma_->chan[5].CONBLK_AD = phys_addr;
    dma_->chan[5].DEBUG = 7;
    usleep(10);

    dma_->chan[5].CS =                         //
        RPI4::DMA_CS_ACTIVE |                  //
        RPI4::DMA_CS_WAIT_OUTSTANDING_WRITES | //
        RPI4::DMA_CS_PRIORITY(15) |            //
        RPI4::DMA_CS_PANIC_PRIORITY(15);
  }

  ~LED() {

    if (handle_ != 0) {
      free_memory(handle_);
    }
    if (bus_addr_ != 0) {
      unlock_memory(handle_);
    }
    if (dma_mem != MAP_FAILED) {
      munmap(dma_mem, total_size);
    }

    dma_->chan[5].CS = RPI4::DMA_CS_RESET;
    pwm_->CTL ^= RPI4::PWM_CTL_PWEN1; // Disable PWM
    std::println("Cleanup complete.");
  }

private:
  uint32_t handle_ = 0;
  uint32_t bus_addr_ = 0;
  size_t total_size = 0;
  void *dma_mem = nullptr;
};

auto main() -> int {

  try {
    LED led;
    std::this_thread::sleep_for(std::chrono::seconds(10));
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    return 1;
  }

  return 0;
}
