#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui_port_esp_idf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define I2C_PORT    I2C_NUM_0
#define PIN_SDA     GPIO_NUM_21
#define PIN_SCL     GPIO_NUM_22
#define I2C_ADDR    0x3C
#define I2C_FREQ    400000
#define I2C_TIMEOUT 100

#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void app_main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    esp_idf_i2c_config_t i2c_cfg = {
        .port = I2C_PORT,
        .sda = PIN_SDA,
        .scl = PIN_SCL,
        .freq = I2C_FREQ,
        .addr = I2C_ADDR,
        .timeout_ms = I2C_TIMEOUT,
    };
    eui_hal_i2c_t *i2c = eui_port_esp_idf_i2c_create(&i2c_cfg);

    eui_drv_ssd1306_config_t dcfg = {
        .i2c = *i2c,
        .width = 128,
        .height = 64,
        .i2c_addr = I2C_ADDR,
    };
    eui_display_hal_t *display = eui_drv_ssd1306_create(&dcfg);

    eui_config_t eui_cfg = { .display = display, .input = NULL };
    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);

    display->init(display->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("Hello EUI!", 10, 5);
    eui_view_dispatcher_add(vd, 1, &label->view);

    eui_widget_t *battery = eui_progress_create(10, 35, 108, 12);
    eui_progress_set_value(battery, 75);
    eui_view_dispatcher_add(vd, 1, &battery->view);

    eui_widget_t *pct = eui_label_create("Battery: 75%", 10, 50);
    eui_view_dispatcher_add(vd, 1, &pct->view);

    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    while (true) {
        eui_tick();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
