#include "eui/eui.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include "eui/eui_font_wqy13.h"

#define W 128
#define H 64
/* W*H*2 for 16bpp framebuffer + overhead for TLSF + structs */
#define POOL_SIZE (128 * 64 * 2 + 8192)
static uint8_t mem_pool[POOL_SIZE];

static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_drv_t *display = eui_drv_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_drv_t *input = eui_drv_raylib_create_input();

    eui_config_t cfg = { .display=display, .input=input };
    if (eui_init(&cfg) != 0) { fprintf(stderr, "eui_init failed\n"); return 1; }
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *label = eui_label_create("应用列表Amiibo", 20, 20);
    eui_label_set_font(label, &eui_font_wqy13);
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
