/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V3.0
* | Date        :   2019-07-31
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "DEV_Config.h"

/**
 * GPIO
 **/
int EPD_RST_PIN;
int EPD_DC_PIN;
int EPD_CS_PIN;
int EPD_BUSY_PIN;
int EPD_PWR_PIN;
int EPD_MOSI_PIN;
int EPD_SCLK_PIN;

/**
 * GPIO read and write
 **/
void DEV_Digital_Write(UWORD Pin, UBYTE Value) { GPIOD_Write(Pin, Value); }

UBYTE DEV_Digital_Read(UWORD Pin) {
  UBYTE Read_value = 0;
  Read_value = GPIOD_Read(Pin);
  return Read_value;
}

/**
 * SPI
 **/
void DEV_SPI_WriteByte(uint8_t Value) { DEV_HARDWARE_SPI_TransferByte(Value); }

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len) {
  DEV_HARDWARE_SPI_Transfer(pData, Len);
}

/**
 * GPIO Mode
 **/
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode) {
  if (Mode == 0 || Mode == GPIOD_IN) {
    GPIOD_Direction(Pin, GPIOD_IN);
    // Debug("IN Pin = %d\r\n",Pin);
  } else {
    GPIOD_Direction(Pin, GPIOD_OUT);
    // Debug("OUT Pin = %d\r\n",Pin);
  }
}

/**
 * delay x ms
 **/
void DEV_Delay_ms(UDOUBLE xms) {
  UDOUBLE i;
  for (i = 0; i < xms; i++) {
    usleep(1000);
  }
}

static int DEV_Equipment_Testing(void) {
  FILE *fp;
  char issue_str[64];

  fp = fopen("/etc/issue", "r");
  if (fp == NULL) {
    Debug("Unable to open /etc/issue");
    return -1;
  }
  if (fread(issue_str, 1, sizeof(issue_str), fp) <= 0) {
    Debug("Unable to read from /etc/issue");
    return -1;
  }
  issue_str[sizeof(issue_str) - 1] = '\0';
  fclose(fp);

  printf("Current environment: ");
  char systems[][9] = {"Raspbian", "Debian", "NixOS"};
  int detected = 0;
  for (int i = 0; i < 3; i++) {
    if (strstr(issue_str, systems[i]) != NULL) {
      printf("%s\n", systems[i]);
      detected = 1;
    }
  }
  if (!detected) {
    printf("not recognized\n");
    printf("Built for Raspberry Pi, but unable to detect environment.\n");
    printf("Perhaps you meant to 'make JETSON' instead?\n");
    return -1;
  }
  return 0;
}

void DEV_GPIO_Init(void) {
  EPD_RST_PIN = 17;
  EPD_DC_PIN = 25;
  EPD_CS_PIN = 8;
  EPD_PWR_PIN = 18;
  EPD_BUSY_PIN = 24;
  EPD_MOSI_PIN = 10;
  EPD_SCLK_PIN = 11;

  DEV_GPIO_Mode(EPD_BUSY_PIN, 0);
  DEV_GPIO_Mode(EPD_RST_PIN, 1);
  DEV_GPIO_Mode(EPD_DC_PIN, 1);
  DEV_GPIO_Mode(EPD_CS_PIN, 1);
  DEV_GPIO_Mode(EPD_PWR_PIN, 1);
  // DEV_GPIO_Mode(EPD_MOSI_PIN, 0);
  // DEV_GPIO_Mode(EPD_SCLK_PIN, 1);

  DEV_Digital_Write(EPD_CS_PIN, 1);
  DEV_Digital_Write(EPD_PWR_PIN, 1);
}

void DEV_SPI_SendnData(UBYTE *Reg) {
  UDOUBLE size;
  size = sizeof(Reg);
  for (UDOUBLE i = 0; i < size; i++) {
    DEV_SPI_SendData(Reg[i]);
  }
}

void DEV_SPI_SendData(UBYTE Reg) {
  UBYTE i, j = Reg;
  DEV_GPIO_Mode(EPD_MOSI_PIN, 1);
  DEV_Digital_Write(EPD_CS_PIN, 0);
  for (i = 0; i < 8; i++) {
    DEV_Digital_Write(EPD_SCLK_PIN, 0);
    if (j & 0x80) {
      DEV_Digital_Write(EPD_MOSI_PIN, 1);
    } else {
      DEV_Digital_Write(EPD_MOSI_PIN, 0);
    }

    DEV_Digital_Write(EPD_SCLK_PIN, 1);
    j = j << 1;
  }
  DEV_Digital_Write(EPD_SCLK_PIN, 0);
  DEV_Digital_Write(EPD_CS_PIN, 1);
}

UBYTE DEV_SPI_ReadData() {
  UBYTE i, j = 0xff;
  DEV_GPIO_Mode(EPD_MOSI_PIN, 0);
  DEV_Digital_Write(EPD_CS_PIN, 0);
  for (i = 0; i < 8; i++) {
    DEV_Digital_Write(EPD_SCLK_PIN, 0);
    j = j << 1;
    if (DEV_Digital_Read(EPD_MOSI_PIN)) {
      j = j | 0x01;
    } else {
      j = j & 0xfe;
    }
    DEV_Digital_Write(EPD_SCLK_PIN, 1);
  }
  DEV_Digital_Write(EPD_SCLK_PIN, 0);
  DEV_Digital_Write(EPD_CS_PIN, 1);
  return j;
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI
protocol parameter: Info:
******************************************************************************/
UBYTE DEV_Module_Init(void) {
  printf("/***********************************/ \r\n");
  if (DEV_Equipment_Testing() < 0) {
    return 1;
  }
  printf("Write and read /dev/spidev0.0 \r\n");
  GPIOD_Export();
  DEV_GPIO_Init();
  DEV_HARDWARE_SPI_begin("/dev/spidev0.0");
  DEV_HARDWARE_SPI_setSpeed(10000000);
  printf("/***********************************/ \r\n");
  return 0;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_Module_Exit(void) {
  DEV_HARDWARE_SPI_end();
  DEV_Digital_Write(EPD_CS_PIN, 0);
  DEV_Digital_Write(EPD_PWR_PIN, 0);
  DEV_Digital_Write(EPD_DC_PIN, 0);
  DEV_Digital_Write(EPD_RST_PIN, 0);
  GPIOD_Unexport(EPD_PWR_PIN);
  GPIOD_Unexport(EPD_DC_PIN);
  GPIOD_Unexport(EPD_RST_PIN);
  GPIOD_Unexport(EPD_BUSY_PIN);
  GPIOD_Unexport_GPIO();
}
