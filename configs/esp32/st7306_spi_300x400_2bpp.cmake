# configs/esp32/st7306_spi_300x400_2bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      2)
set(EUI_DISPLAY_DRIVER   "st7306")
set(EUI_DISPLAY_WIDTH    300)
set(EUI_DISPLAY_HEIGHT   400)
set(EUI_BUFFER_MODE      "full")
set(EUI_MEM_POOL_SIZE    65536)
set(EUI_DRV_ST7306       1)   # enables ST7306 path in bootstrap

# SPI pins
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
