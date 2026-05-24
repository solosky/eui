# configs/nrf52/ssd1306_i2c_128x64_1bpp.cmake
set(EUI_TARGET_PORT      "nrf5")
set(EUI_COLOR_DEPTH      1)
set(EUI_DISPLAY_DRIVER   "ssd1306")
set(EUI_DISPLAY_WIDTH    128)
set(EUI_DISPLAY_HEIGHT   64)
set(EUI_BUFFER_MODE      "page")
set(EUI_MEM_POOL_SIZE    32768)

# I2C pins (nRF52 DK defaults)
set(EUI_PIN_I2C_PORT     0)
set(EUI_PIN_SDA          26)
set(EUI_PIN_SCL          27)
set(EUI_PIN_I2C_FREQ     400000)
set(EUI_PIN_I2C_ADDR     0x3C)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       13)
set(EUI_PIN_BTN_DOWN     14)
set(EUI_PIN_BTN_OK       11)
set(EUI_PIN_BTN_BACK     12)
