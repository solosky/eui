# configs/esp32/ili9341_spi_320x240_16bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "ili9341")
set(EUI_DISPLAY_WIDTH    320)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    262144)

# SPI pins
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)
set(EUI_PIN_BL           4)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
