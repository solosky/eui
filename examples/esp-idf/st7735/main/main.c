#include "eui/eui.h"
#include "eui/driver/eui_drv_st7735.h"
#include "eui_port_esp_idf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <stdio.h>

#define SPI_HOST    SPI2_HOST
#define PIN_MOSI    GPIO_NUM_23
#define PIN_SCLK    GPIO_NUM_18
#define PIN_CS      GPIO_NUM_5
#define PIN_DC      GPIO_NUM_16
#define PIN_RST     GPIO_NUM_17
#define SPI_FREQ    40000000
#define SPI_QUEUE   1

#define SCREEN_W    240
#define SCREEN_H    240

#if defined(EUI_ST7735_BUFFER_FULL)
    #define POOL_SIZE (SCREEN_W * SCREEN_H * 2 + 32768)
    static uint8_t *mem_pool = NULL;
#else
    #define POOL_SIZE 24576
    static uint8_t mem_pool[POOL_SIZE];
#endif

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static int counter = 0;
static char counter_text[32];

static void on_button_click(void *ctx) {
    eui_widget_t *label = (eui_widget_t *)ctx;
    counter++;
    snprintf(counter_text, sizeof(counter_text), "Clicks: %d", counter);
    eui_label_set_text(label, counter_text);
}

void app_main(void) {
#if defined(EUI_ST7735_BUFFER_FULL)
    mem_pool = (uint8_t *)heap_caps_malloc(POOL_SIZE, MALLOC_CAP_SPIRAM);
#endif
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    esp_idf_spi_config_t spi_cfg = {
        .host = SPI_HOST,
        .mosi = PIN_MOSI,
        .sclk = PIN_SCLK,
        .cs = PIN_CS,
        .dc = PIN_DC,
        .rst = PIN_RST,
        .freq = SPI_FREQ,
        .queue_size = SPI_QUEUE,
    };
    eui_hal_spi_t *spi = eui_port_esp_idf_spi_create(&spi_cfg);

    eui_drv_st7735_config_t dcfg = {
        .spi = *spi,
        .width = SCREEN_W,
        .height = SCREEN_H,
        .variant = 1,
    };
    eui_display_hal_t *display = eui_drv_st7735_create(&dcfg);

    eui_config_t eui_cfg = { .display = display, .input = NULL };
    eui_init(&eui_cfg);
    eui_set_tick_callback(get_tick_ms);

    display->init(display->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *title = eui_label_create("EUI ST7735", 60, 10);
    eui_view_dispatcher_add(vd, 1, &title->view);

    eui_widget_t *btn = eui_button_create("Click Me!", 60, 120, 120, 40);
    eui_view_dispatcher_add(vd, 1, &btn->view);

    eui_widget_t *cnt = eui_label_create("Clicks: 0", 80, 180);
    eui_view_dispatcher_add(vd, 1, &cnt->view);
    eui_button_set_callback(btn, on_button_click, cnt);

    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    while (true) {
        eui_tick();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
