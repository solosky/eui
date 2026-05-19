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

static int selected_idx = -1;

static void on_item_select(uint8_t index, void *ctx) {
    selected_idx = (int)index;
    (void)ctx;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t *list = eui_list_create(0, 0, 128, 64);
    list->focus_policy = EUI_FOCUS_STRONG;
    eui_list_set_callback(list, on_item_select, NULL);

    for (int i = 0; i < 10; i++) {
        char item[16];
        snprintf(item, sizeof(item), "Item %d", i);
        eui_list_add_item(list, item, NULL);
    }

    eui_view_dispatcher_add(&vd, 1, &list->view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    eui_event_t evt;
    for (int i = 0; i < 5; i++) {
        evt.type = EUI_EVT_KEY_PRESS;
        evt.data.key = EUI_KEY_DOWN;
        eui_view_dispatcher_send_input(&vd, &evt);
    }

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    if (selected_idx < 0) { printf("[FAIL] No item selected\n"); return 1; }
    printf("[PASS] list_nav (selected item %d)\n", selected_idx);
    eui_canvas_destroy(canvas);
    return 0;
}
