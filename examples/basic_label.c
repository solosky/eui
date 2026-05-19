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

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t *label = eui_label_create("Hello EUI!", 20, 20);
    eui_widget_set_visible(label, true);
    eui_view_dispatcher_add(&vd, 1, &label->view);

    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    for (int i = 0; i < 30; i++)
        eui_view_dispatcher_tick(&vd);

    eui_canvas_destroy(canvas);
    printf("[PASS] basic_label\n");
    return 0;
}
