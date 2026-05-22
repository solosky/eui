#include "eui/eui.h"
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
    eui_display_hal_t *display = eui_drv_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_drv_raylib_create_input();

    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("Hello EUI!", 20, 20);
    eui_view_dispatcher_add(vd, 1, &label->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

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
