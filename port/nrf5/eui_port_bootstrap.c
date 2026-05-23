/* port/nrf5/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/eui_port_bootstrap.h"
#include "eui_port_nrf5.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include <string.h>

#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

/* Default pin config — override via Kconfig or sdk_config */
#ifndef CONFIG_EUI_EXAMPLE_I2C_SDA
#define CONFIG_EUI_EXAMPLE_I2C_SDA    26
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SCL
#define CONFIG_EUI_EXAMPLE_I2C_SCL    27
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_FREQ
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_ADDR
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
#endif

static uint32_t get_tick_ms(void) {
    return app_timer_cnt_get() * (1000 / APP_TIMER_CLOCK_FREQ);
}

int main(void) {
    eui_example_config_t cfg = {
        .display_width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH,
        .display_height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT,
    };

    /* Create TWI (I2C) transport */
    nrf5_i2c_config_t i2c_cfg = {
        .sda         = CONFIG_EUI_EXAMPLE_I2C_SDA,
        .scl         = CONFIG_EUI_EXAMPLE_I2C_SCL,
        .addr        = CONFIG_EUI_EXAMPLE_I2C_ADDR,
        .freq        = CONFIG_EUI_EXAMPLE_I2C_FREQ,
        .instance_id = 0,
    };
    eui_hal_i2c_t *i2c = eui_port_nrf5_i2c_create(&i2c_cfg);

    /* Create SSD1306 display */
    eui_drv_ssd1306_config_t dcfg = {
        .i2c      = *i2c,
        .width    = cfg.display_width,
        .height   = cfg.display_height,
        .i2c_addr = CONFIG_EUI_EXAMPLE_I2C_ADDR,
    };
    eui_display_hal_t *display = eui_drv_ssd1306_create(&dcfg);

    eui_config_t eui_cfg = {
        .mem_pool_buffer = mem_pool,
        .mem_pool_size   = sizeof(mem_pool),
        .display         = display,
        .input           = NULL,
    };

    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (1) {
        eui_tick();
        nrf_delay_ms(16);
    }
}
