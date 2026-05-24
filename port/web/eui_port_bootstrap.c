#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include "eui/driver/eui_drv_web.h"
#include <emscripten.h>
#include <stdio.h>

#define POOL_SIZE EUI_MEM_POOL_SIZE
static uint8_t mem_pool[POOL_SIZE];

static eui_display_drv_t *g_display = NULL;
static eui_input_drv_t   *g_input   = NULL;

static uint32_t get_tick_ms(void) {
    return (uint32_t)emscripten_get_now();
}

static void main_loop(void) {
    eui_tick();
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, sizeof(mem_pool));

    eui_example_config_t cfg = {
        .display_width  = EUI_DISPLAY_WIDTH,
        .display_height = EUI_DISPLAY_HEIGHT,
    };

    g_display = eui_drv_web_create_display(
        cfg.display_width, cfg.display_height, EUI_COLOR_DEPTH);
    if (!g_display) {
        fprintf(stderr, "Failed to create web display\n");
        return 1;
    }

    g_input = eui_drv_web_create_input();
    if (!g_input) {
        fprintf(stderr, "Failed to create web input\n");
        eui_drv_web_destroy_display(g_display);
        return 1;
    }

    eui_config_t eui_cfg = { .display = g_display, .input = g_input };

    if (eui_init(&eui_cfg) != 0) {
        fprintf(stderr, "eui_init failed\n");
        eui_drv_web_destroy_input(g_input);
        eui_drv_web_destroy_display(g_display);
        return 1;
    }

    eui_set_tick_callback(get_tick_ms);
    g_display->init(g_display->user_data);
    eui_example_setup(&cfg);

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
