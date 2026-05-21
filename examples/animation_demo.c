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

static void square_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}
static bool square_input(eui_widget_t *w, const eui_event_t *e) { (void)w;(void)e; return false; }
static eui_widget_vtable_t square_vt = { .draw=square_draw, .input=square_input };

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t square;
    eui_widget_init(&square, &square_vt, 0, 30, 16, 16);

    eui_view_dispatcher_add(vd, 1, &square.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    eui_anim_init();
    eui_anim_start(&square, EUI_ANIM_TARGET_X, 0, 80, 500, mc_ease_linear, NULL, NULL);

    while (!eui_hal_raylib_window_should_close()) {
        eui_tick();
        eui_hal_raylib_refresh();
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_hal_raylib_destroy_input(input);
    eui_hal_raylib_destroy_display(display);
    return 0;
}
