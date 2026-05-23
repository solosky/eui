/* port/raylib/eui_port_bootstrap.c */
#include "eui/eui.h"
#include "eui/driver/eui_drv_raylib.h"
#include "eui/eui_port_bootstrap.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE (128 * 64 * 2 + 8192)
static uint8_t mem_pool[POOL_SIZE];

static uint32_t get_tick_ms(void) {
    return (uint32_t)(GetTime() * 1000.0);
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, sizeof(mem_pool));

    eui_example_config_t cfg = {
        .display_width  = 128,
        .display_height = 64,
    };

    eui_display_drv_t *display = eui_drv_raylib_create_display(
        cfg.display_width, cfg.display_height, EUI_COLOR_DEPTH);
    if (!display) {
        fprintf(stderr, "Failed to create raylib display\n");
        return 1;
    }

    eui_input_drv_t *input = eui_drv_raylib_create_input();
    if (!input) {
        fprintf(stderr, "Failed to create raylib input\n");
        eui_drv_raylib_destroy_display(display);
        return 1;
    }

    eui_config_t eui_cfg = {
        .display         = display,
        .input           = input,
    };

    if (eui_init(&eui_cfg) != 0) {
        fprintf(stderr, "eui_init failed\n");
        eui_drv_raylib_destroy_input(input);
        eui_drv_raylib_destroy_display(display);
        return 1;
    }

    eui_set_tick_callback(get_tick_ms);
    display->init(display->user_data);

    eui_example_setup(&cfg);

    while (!eui_drv_raylib_window_should_close()) {
        eui_tick();
        eui_drv_raylib_refresh();
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(input);
    eui_drv_raylib_destroy_display(display);
    return 0;
}
