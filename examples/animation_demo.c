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

static uint32_t fake_tick = 0;
static uint32_t get_tick(void) { return fake_tick; }

static void stub_draw(eui_widget_t *w, eui_canvas_t *c)
{
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool stub_input(eui_widget_t *w, const eui_event_t *e) { (void)w; (void)e; return false; }
static eui_widget_vtable_t square_vt = { .draw = stub_draw, .input = stub_input };

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t square;
    eui_widget_init(&square, &square_vt, 0, 20, 16, 16);
    square.focus_policy = EUI_FOCUS_NONE;

    eui_view_dispatcher_add(&vd, 1, &square.view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    eui_anim_init();

    eui_anim_handle_t h = eui_anim_start(&square, EUI_ANIM_TARGET_X,
                                          0, 80, 500, mc_ease_linear, NULL, NULL);

    for (int i = 0; i < 30; i++) {
        fake_tick += 33;
        eui_anim_update(33);
        eui_view_dispatcher_tick(&vd);
    }

    int final_x = square.area.x;
    eui_canvas_destroy(canvas);

    if (final_x < 10) { printf("[FAIL] Animation did not move (x=%d)\n", final_x); return 1; }
    printf("[PASS] animation_demo (square moved from 0 to %d)\n", final_x);
    (void)h;
    return 0;
}
