#include "eui/eui.h"
#include "eui/hal/eui_hal_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 16384
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, 1);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);

    eui_canvas_t *canvas = eui_canvas_create(display);

    eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
    eui_canvas_clear(canvas);
    eui_canvas_fill_rect(canvas, 10, 10, 40, 30);
    eui_canvas_draw_rect(canvas, 60, 15, 50, 30);
    eui_canvas_draw_circle(canvas, 30, 45, 10);
    eui_canvas_commit(canvas);

    while (!eui_hal_raylib_window_should_close()) {
        eui_hal_raylib_refresh();
    }

    eui_canvas_destroy(canvas);
    display->deinit(display->user_data);
    eui_deinit();
    eui_hal_raylib_destroy_input(input);
    eui_hal_raylib_destroy_display(display);
    return 0;
}
