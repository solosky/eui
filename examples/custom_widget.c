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

typedef struct {
    eui_widget_t widget;
    int count;
} counter_t;

static void counter_draw(eui_widget_t *w, eui_canvas_t *c) {
    counter_t *cw = (counter_t*)w;
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_draw_round_rect(c, w->area.x, w->area.y, w->area.w, w->area.h, 4);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", cw->count);
    eui_canvas_draw_str_aligned(c, w->area.x + (int16_t)(w->area.w / 2),
                                  w->area.y + (int16_t)(w->area.h / 2),
                                  EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE, buf);
}

static bool counter_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        ((counter_t*)w)->count++;
        return true;
    }
    return false;
}

static eui_widget_vtable_t counter_vt = {
    .draw = counter_draw,
    .input = counter_input
};

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    counter_t cw;
    memset(&cw, 0, sizeof(cw));
    eui_widget_init(&cw.widget, &counter_vt, 40, 20, 48, 24);
    cw.widget.focus_policy = EUI_FOCUS_STRONG;
    eui_view_dispatcher_add(vd, 1, &cw.widget.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

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
