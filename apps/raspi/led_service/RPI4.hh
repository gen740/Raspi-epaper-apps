#pragma once

#include <cstddef>
#include <cstdint>

namespace RPI4 {

/*! -------------------------------------------------
 * @brief DMA
 * -------------------------------------------------- */
struct DMA {
  struct alignas(256) Channel {
    alignas(4) uint32_t CS;
    alignas(4) uint32_t CONBLK_AD;
    alignas(4) uint32_t TI;
    alignas(4) uint32_t SOURCE_AD;
    alignas(4) uint32_t DEST_AD;
    alignas(4) uint32_t TXFR_LEN;
    alignas(4) uint32_t STRIDE;
    alignas(4) uint32_t NEXTCONBK;
    alignas(4) uint32_t DEBUG;
  };
  Channel chan[16]; // NOLINT
};
static_assert(sizeof(DMA) == 0x1000, "DMA size mismatch");

static constexpr uint32_t DMA_CS_INT = 1 << 2;
static constexpr uint32_t DMA_CS_END = 1 << 1;
static constexpr uint32_t DMA_CS_RESET = 1 << 31;
static constexpr uint32_t DMA_CS_WAIT_OUTSTANDING_WRITES = 1 << 28;
static constexpr uint32_t DMA_CS_ACTIVE = 1 << 0;
static consteval auto DMA_CS_PRIORITY(auto val) -> uint32_t {
  return (val & 0b1111) << 16;
}
static consteval auto DMA_CS_PANIC_PRIORITY(auto val) -> uint32_t {
  return (val & 0b1111) << 20;
}

/*! -------------------------------------------------
 * @brief DMA Control Block
 * -------------------------------------------------- */
template <size_t N> struct DMACB {
  uint32_t ti;
  uint32_t src;
  uint32_t dst;
  uint32_t len;
  uint32_t stride;
  uint32_t next;
  uint32_t pad[2];   // NOLINT
  uint32_t data[N]; // NOLINT
};

static constexpr uint32_t DMACB_TI_NO_WIDE_BURSTS = 1 << 26;
static constexpr uint32_t DMACB_TI_SRC_INC = 1 << 8;
static constexpr uint32_t DMACB_TI_DEST_DREQ = 1 << 6;
static constexpr uint32_t DMACB_TI_PERMAP_PWM = 5 << 16;
static constexpr uint32_t DMACB_TI_WAIT_RESP = 1 << 3;

/*! -------------------------------------------------
 * @brief GPIO
 * -------------------------------------------------- */
struct alignas(32) GPIO {
  uint32_t GPFSEL0;
  uint32_t GPFSEL1;
  uint32_t GPFSEL2;
  uint32_t GPFSEL3;
  uint32_t GPFSEL4;
  uint32_t GPFSEL5;
};

static constexpr uint32_t RPI_GPIO_ALT0 = 0b100;
static constexpr uint32_t RPI_GPIO_ALT1 = 0b101;
static constexpr uint32_t RPI_GPIO_ALT2 = 0b110;
static constexpr uint32_t RPI_GPIO_ALT3 = 0b111;
static constexpr uint32_t RPI_GPIO_ALT4 = 0b111;
static constexpr auto GPIO_FSEL1(auto val) -> uint32_t {
  return (val & 0b111) << 6;
}

/*! -------------------------------------------------
 * @brief PWM
 * -------------------------------------------------- */
struct PWM {
  alignas(4) uint32_t CTL;  // Control
  alignas(4) uint32_t STA;  // Status
  alignas(4) uint32_t DMAC; // DMA Configuration
  alignas(4) uint32_t RNG1; // Range for Channel 1
  alignas(4) uint32_t DAT1; // Data for Channel 1
  alignas(4) uint32_t FIF1; // FIFO for Channel 1
  alignas(4) uint32_t RNG2; // Range for Channel 2
  alignas(4) uint32_t DAT2; // Data for Channel 2
};

static constexpr uint32_t PWM_DMAC_ENABLE = 1 << 31;
static constexpr uint32_t PWM_DMAC_PANIC = 1 << 8;
static constexpr uint32_t PWM_DMAC_DREQ = 1 << 0;

static constexpr uint32_t PWM_CTL_CLRF = 1 << 6;
static constexpr uint32_t PWM_CTL_PWEN1 = 1 << 0;
static constexpr uint32_t PWM_CTL_SBIT1 = 1 << 3;
static constexpr uint32_t PWM_CTL_MODE1 = 1 << 1;
static constexpr uint32_t PWM_CTL_USEF1 = 1 << 5;
static constexpr uint32_t PWM_CTL_MSEN1 = 1 << 7;

/*! -------------------------------------------------
 * @brief CLK
 * -------------------------------------------------- */
struct CLK {
  alignas(4) uint32_t CTL; // Clock Control
  alignas(4) uint32_t DIV; // Clock Control
};

static constexpr uint32_t CLK_CTL_PASSWD = 0x5A000000;
static constexpr uint32_t CLK_CTL_KILL = 1 << 5;
static constexpr uint32_t CLK_CTL_BUSY = 1 << 7;
static constexpr uint32_t CLK_CTL_ENAB = 1 << 4;
static consteval auto CLK_CTL_SRC(auto val) -> uint32_t {
  return (val & 0b1111) << 0;
}
static consteval auto CLK_DIV_DIVI(auto val) -> uint32_t {
  return (static_cast<uint32_t>(val) & 0xFFF) << 12;
}
static consteval auto CLK_DIV_DIVF(auto val) -> uint32_t {
  return (static_cast<uint32_t>(val) & 0xFFF) << 0;
}

}; // namespace RPI4
