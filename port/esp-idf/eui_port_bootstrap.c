/* port/esp-idf/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/driver/eui_drv_st7306.h"
#include "eui/eui_port_bootstrap.h"
#include "eui_port_esp_idf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>

#if __has_include("eui/eui_profile_config.h")
#include "eui/eui_profile_config.h"
#endif

#ifdef EUI_DRV_ST7306
#define POOL_SIZE 65536
#else
#define POOL_SIZE 8192
#endif
static uint8_t mem_pool[POOL_SIZE];

/* Default pin config — override via Kconfig menuconfig */
#ifndef CONFIG_EUI_EXAMPLE_I2C_PORT
#define CONFIG_EUI_EXAMPLE_I2C_PORT   0
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SDA
#define CONFIG_EUI_EXAMPLE_I2C_SDA    21
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SCL
#define CONFIG_EUI_EXAMPLE_I2C_SCL    22
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_FREQ
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_ADDR
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_TIMEOUT
#define CONFIG_EUI_EXAMPLE_I2C_TIMEOUT 100
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
#endif

#ifdef EUI_DRV_ST7306
#ifndef CONFIG_EUI_EXAMPLE_SPI_HOST
#define CONFIG_EUI_EXAMPLE_SPI_HOST   1
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_MOSI
#define CONFIG_EUI_EXAMPLE_SPI_MOSI   23
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_SCLK
#define CONFIG_EUI_EXAMPLE_SPI_SCLK   18
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_CS
#define CONFIG_EUI_EXAMPLE_SPI_CS     5
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_DC
#define CONFIG_EUI_EXAMPLE_SPI_DC     16
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_RST
#define CONFIG_EUI_EXAMPLE_SPI_RST    17
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_FREQ
#define CONFIG_EUI_EXAMPLE_SPI_FREQ   20000000
#endif
#endif /* EUI_DRV_ST7306 */

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_main(void) {
    eui_allocator_init_tlsf(mem_pool, sizeof(mem_pool));

    eui_example_config_t cfg = {
        .display_width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH,
        .display_height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT,
    };

    eui_display_drv_t *display = NULL;

#ifdef EUI_DRV_ST7306
    /* --- ST7306 / SPI path --- */
    {
        esp_idf_spi_config_t spi_cfg = {
            .host       = CONFIG_EUI_EXAMPLE_SPI_HOST,
            .mosi       = CONFIG_EUI_EXAMPLE_SPI_MOSI,
            .sclk       = CONFIG_EUI_EXAMPLE_SPI_SCLK,
            .cs         = CONFIG_EUI_EXAMPLE_SPI_CS,
            .dc         = CONFIG_EUI_EXAMPLE_SPI_DC,
            .rst        = CONFIG_EUI_EXAMPLE_SPI_RST,
            .freq       = CONFIG_EUI_EXAMPLE_SPI_FREQ,
            .queue_size = 1,
        };
        eui_hal_spi_t *spi = eui_port_esp_idf_spi_create(&spi_cfg);

        eui_drv_st7306_config_t dcfg = {
            .spi    = *spi,
            .width  = cfg.display_width,
            .height = cfg.display_height,
        };
        display = eui_drv_st7306_create(&dcfg);
    }
#else
    /* --- SSD1306 / I2C path (default) --- */
    {
        esp_idf_i2c_config_t i2c_cfg = {
            .port       = CONFIG_EUI_EXAMPLE_I2C_PORT,
            .sda        = CONFIG_EUI_EXAMPLE_I2C_SDA,
            .scl        = CONFIG_EUI_EXAMPLE_I2C_SCL,
            .freq       = CONFIG_EUI_EXAMPLE_I2C_FREQ,
            .addr       = CONFIG_EUI_EXAMPLE_I2C_ADDR,
            .timeout_ms = CONFIG_EUI_EXAMPLE_I2C_TIMEOUT,
        };
        eui_hal_i2c_t *i2c = eui_port_esp_idf_i2c_create(&i2c_cfg);

        eui_drv_ssd1306_config_t dcfg = {
            .i2c      = *i2c,
            .width    = cfg.display_width,
            .height   = cfg.display_height,
            .i2c_addr = CONFIG_EUI_EXAMPLE_I2C_ADDR,
        };
        display = eui_drv_ssd1306_create(&dcfg);
    }
#endif

    eui_config_t eui_cfg = {
        .display         = display,
        .input           = NULL,
    };

    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (1) {
        eui_tick();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
