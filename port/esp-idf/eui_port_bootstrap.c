/* port/esp-idf/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/eui_port_bootstrap.h"
#include "eui_port_esp_idf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>

#define POOL_SIZE 8192
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

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_main(void) {
    eui_allocator_init_tlsf(mem_pool, sizeof(mem_pool));

    eui_example_config_t cfg = {
        .display_width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH,
        .display_height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT,
    };

    /* Create I2C transport */
    esp_idf_i2c_config_t i2c_cfg = {
        .port       = CONFIG_EUI_EXAMPLE_I2C_PORT,
        .sda        = CONFIG_EUI_EXAMPLE_I2C_SDA,
        .scl        = CONFIG_EUI_EXAMPLE_I2C_SCL,
        .freq       = CONFIG_EUI_EXAMPLE_I2C_FREQ,
        .addr       = CONFIG_EUI_EXAMPLE_I2C_ADDR,
        .timeout_ms = CONFIG_EUI_EXAMPLE_I2C_TIMEOUT,
    };
    eui_hal_i2c_t *i2c = eui_port_esp_idf_i2c_create(&i2c_cfg);

    /* Create SSD1306 display */
    eui_drv_ssd1306_config_t dcfg = {
        .i2c      = *i2c,
        .width    = cfg.display_width,
        .height   = cfg.display_height,
        .i2c_addr = CONFIG_EUI_EXAMPLE_I2C_ADDR,
    };
    eui_display_hal_t *display = eui_drv_ssd1306_create(&dcfg);

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
