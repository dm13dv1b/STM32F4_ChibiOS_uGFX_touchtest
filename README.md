STM32F4_ChibiOS_uGFX_touchtest
==============================

Copy the board_SSD1289.h and gdisp_lld_SSD1289.c into:
	ugfx/drivers/gdisp/SSD1289
Copy the ginput_lld_mouse_board.h into:
	ugfx/drivers/ginput/touch/ADS7843

Modify the Makefile for your settings.

Touchscreen connection:

  SCK:             GPIOB, 13
  MISO:            GPIOB, 14
  MOSI:            GPIOB, 15
  CS:              GPIOC, 4
  IRQ:             GPIOC, 5

SSD1289 board connection:

  CS		PD7
  RST		PD10
  RS		PD11
  WR		PD8
  RD		PD9

  DB[0:3]	PD[0:3]
  DB[4:15]	PE[4:15]


