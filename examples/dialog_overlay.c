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

static eui_dialog_result_t last_result = -1;

static void on_dialog_result(eui_dialog_result_t result, void *ctx) {
    last_result = result;
    (void)ctx;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t *dialog = eui_dialog_create("Confirm", "Are you sure?");
    eui_dialog_add_button(dialog, "Yes", EUI_DIALOG_YES);
    eui_dialog_add_button(dialog, "No",  EUI_DIALOG_NO);
    eui_dialog_show(dialog, &vd, on_dialog_result);

    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    eui_event_t evt;
    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    if ((int)last_result < 0) { printf("[FAIL] No dialog result\n"); return 1; }
    printf("[PASS] dialog_overlay (result=%d)\n", (int)last_result);
    eui_canvas_destroy(canvas);
    return 0;
}
