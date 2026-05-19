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

static int count_pixels(const uint8_t *buf, int sz) {
    int cnt = 0;
    for (int i = 0; i < sz; i++) {
        uint8_t b = buf[i];
        while (b) { cnt += (b & 1); b >>= 1; }
    }
    return cnt;
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    memset(mock_buf, 0, sizeof(mock_buf));

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);

    eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(canvas, 10, 10, 20, 10);
    eui_canvas_commit(canvas);

    int px1 = count_pixels(mock_buf, (int)sizeof(mock_buf));

    eui_canvas_set_color(canvas, EUI_COLOR_BLACK);
    eui_canvas_clear(canvas);
    eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(canvas, 30, 15, 40, 20);
    eui_canvas_commit(canvas);

    int px2 = count_pixels(mock_buf, (int)sizeof(mock_buf));

    eui_canvas_destroy(canvas);

    if (px1 != 200) { printf("[FAIL] First fill: %d px, expected 200\n", px1); return 1; }
    if (px2 != 800) { printf("[FAIL] Second fill: %d px, expected 800\n", px2); return 1; }

    printf("[PASS] page_buffer (%d, %d pixels drawn)\n", px1, px2);
    return 0;
}
