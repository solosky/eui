# configs/nrf52/st7735_spi_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "nrf5")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "st7735")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    262144)

# SPI pins
set(EUI_PIN_SPI_HOST     0)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         24)
set(EUI_PIN_CS           22)
set(EUI_PIN_DC           25)
set(EUI_PIN_RST          19)
set(EUI_PIN_BL           20)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       13)
set(EUI_PIN_BTN_DOWN     14)
set(EUI_PIN_BTN_OK       11)
set(EUI_PIN_BTN_BACK     12)
