#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static uint8_t mock_buf[W * H / 8];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud) {
    (void)ud;
    int bpr = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (W / 8) + r->x / 8),
               b + row * bpr, bpr);
}

static eui_display_hal_t mock_display = {
    .caps = { .width = W, .height = H, .color_depth = 1, .buffer_mode = EUI_BUFFER_FULL },
    .init = NULL, .deinit = NULL, .write_buffer = mock_write_buffer,
    .user_data = NULL
};

static int mock_poll(eui_event_t *e, void *d) { (void)e; (void)d; return 0; }
static eui_input_hal_t mock_input = {
    .poll = mock_poll, .user_data = NULL
};

typedef struct {
    eui_widget_t widget;
    int count;
} counter_widget_t;

static void counter_draw(eui_widget_t *w, eui_canvas_t *c) {
    counter_widget_t *cw = (counter_widget_t*)w;
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_draw_round_rect(c, w->area.x, w->area.y, w->area.w, w->area.h, 4);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", cw->count);
    eui_canvas_draw_str_aligned(c, w->area.x + (int16_t)(w->area.w / 2),
                                  w->area.y + (int16_t)(w->area.h / 2),
                                  EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE, buf);
}

static bool counter_input(eui_widget_t *w, const eui_event_t *evt) {
    counter_widget_t *cw = (counter_widget_t*)w;
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        cw->count++;
        return true;
    }
    return false;
}

static eui_widget_vtable_t counter_vt = {
    .draw = counter_draw,
    .input = counter_input
};

static void counter_init(counter_widget_t *cw, int16_t x, int16_t y, uint16_t ww, uint16_t hh) {
    eui_widget_init(&cw->widget, &counter_vt, x, y, ww, hh);
    cw->count = 0;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    counter_widget_t counter;
    counter_init(&counter, 40, 20, 48, 24);
    counter.widget.focus_policy = EUI_FOCUS_STRONG;

    eui_view_dispatcher_add(&vd, 1, &counter.widget.view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);
    eui_view_dispatcher_tick(&vd);

    eui_event_t evt;
    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    if (counter.count != 3) { printf("[FAIL] Count=%d expected=3\n", counter.count); return 1; }
    printf("[PASS] custom_widget (count=%d)\n", counter.count);
    eui_canvas_destroy(canvas);
    return 0;
}
