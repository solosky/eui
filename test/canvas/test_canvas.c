#include "eui/eui_canvas.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define MOCK_W 128
#define MOCK_H 64
#if EUI_COLOR_DEPTH == 1
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H / 8)
#elif EUI_COLOR_DEPTH == 8
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H)
#else
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H * 2)
#endif

static uint8_t mock_buf[MOCK_BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
#if EUI_COLOR_DEPTH == 1
    int bytes_per_row = r->w / 8;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 8) + r->x / 8),
               b + row * bytes_per_row, bytes_per_row);
    }
#elif EUI_COLOR_DEPTH == 8
    int bytes_per_row = r->w;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * MOCK_W + r->x),
               b + row * bytes_per_row, bytes_per_row);
    }
#else
    int bytes_per_row = r->w * 2;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * MOCK_W * 2 + r->x * 2),
               b + row * bytes_per_row, bytes_per_row);
    }
#endif
}

static eui_display_drv_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int count_pixels(void)
{
#if EUI_COLOR_DEPTH == 1
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        uint8_t b = mock_buf[i];
        while (b) { count += (b & 1); b >>= 1; }
    }
    return count;
#elif EUI_COLOR_DEPTH == 16
    int count = 0;
    uint16_t *p = (uint16_t*)mock_buf;
    int num = (int)sizeof(mock_buf) / 2;
    for (int i = 0; i < num; i++) {
        if (p[i] != 0) count++;
    }
    return count;
#else
    /* For 8bpp, count non-zero bytes as pixels */
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        if (mock_buf[i] != 0) count++;
    }
    return count;
#endif
}

static void test_clear(void)
{
    TEST("clear fills with background");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0xFF, sizeof(mock_buf));
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    eui_canvas_commit(c);
    if (count_pixels() != 0) FAIL("clear should produce 0 pixels");
    eui_canvas_destroy(c);
    PASS();
}

static void test_fill_rect(void)
{
    TEST("fill_rect fills correct area");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    if (count_pixels() != 200) FAIL("fill_rect 20x10 should produce 200 pixels");
    eui_canvas_destroy(c);
    PASS();
}

static void test_clip(void)
{
    TEST("clip restricts drawing");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_rect_t clip = {0, 0, 50, 50};
    eui_canvas_set_clip(c, &clip);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, 0, 0, 100, 100);
    eui_canvas_commit(c);
    if (count_pixels() != 2500) FAIL("clipped fill should be 50x50=2500 pixels");
    eui_canvas_destroy(c);
    PASS();
}

static void test_save_restore(void)
{
    TEST("save/restore preserves state");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_color(c, EUI_COLOR_BLACK);
    eui_canvas_save(c);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_restore(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_fill_rect(c, 0, 0, 50, 50);
    eui_canvas_commit(c);
    if (count_pixels() != 0) FAIL("restored black color should produce 0 pixels");
    eui_canvas_destroy(c);
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Canvas Tests ===\n");
    test_clear();
    test_fill_rect();
    test_clip();
    test_save_restore();
    return eui_test_summary();
}
