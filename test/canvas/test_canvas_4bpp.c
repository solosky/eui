#include "eui/eui_canvas.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define MOCK_W 128
#define MOCK_H 64
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H / 2)  /* 2 pixels per byte */

static uint8_t mock_buf[MOCK_BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int bytes_per_row = r->w / 2;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 2) + r->x / 2),
               b + row * bytes_per_row, bytes_per_row);
    }
}

static eui_display_drv_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = 4,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int count_nonzero_pixels(void)
{
    int count = 0;
    for (int y = 0; y < MOCK_H; y++) {
        for (int x = 0; x < MOCK_W; x++) {
            int byte_idx = y * (MOCK_W / 2) + (x / 2);
            int shift = (x & 1) ? 0 : 4;
            if ((mock_buf[byte_idx] >> shift) & 0x0F) count++;
        }
    }
    return count;
}

static int get_pixel_value(int x, int y)
{
    int byte_idx = y * (MOCK_W / 2) + (x / 2);
    int shift = (x & 1) ? 0 : 4;
    return (mock_buf[byte_idx] >> shift) & 0x0F;
}

static void test_clear(void)
{
    TEST("clear fills with background (4bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0xFF, sizeof(mock_buf));
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != 0) FAIL("clear should produce 0 pixels");
    eui_canvas_destroy(c);
    PASS();
}

static void test_fill_rect(void)
{
    TEST("fill_rect fills correct area (4bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != 200) FAIL("fill_rect 20x10 should produce 200 pixels");
    eui_canvas_destroy(c);
    PASS();
}

static void test_clear_white_bg(void)
{
    TEST("clear with white background (4bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_bg_color(c, 15);
    eui_canvas_clear(c);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != MOCK_W * MOCK_H) FAIL("all pixels should be non-zero");
    if (get_pixel_value(0, 0) != 15) FAIL("pixel should be white (15)");
    eui_canvas_destroy(c);
    PASS();
}

static void test_invert_rect(void)
{
    TEST("invert_rect toggles pixels (4bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));

    /* Fill rect with white (15).  Invert it: 15^15=0, outside stays 0. */
    eui_canvas_set_color(c, 15);
    eui_canvas_fill_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    eui_canvas_invert_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != 0) FAIL("invert of white rect should clear all pixels");
    if (get_pixel_value(10, 10) != 0) FAIL("inverted white pixel should be black");

    /* Invert a black (0) area: 0^15=15 */
    eui_canvas_invert_rect(c, 0, 0, 10, 10);
    eui_canvas_commit(c);
    if (get_pixel_value(0, 0) != 15) FAIL("inverted black pixel should become white");
    if (count_nonzero_pixels() != 100) FAIL("invert 10x10 black -> 100 white");

    eui_canvas_destroy(c);
    PASS();
}

static void test_color_conversion(void)
{
    TEST("color_from_gray 4bpp");
    if (eui_color_from_gray(0) != 0) FAIL("gray 0 -> 0");
    if (eui_color_from_gray(255) != 15) FAIL("gray 255 -> 15");
    if (eui_color_from_gray(17) != 1) FAIL("gray 17 -> 1");
    if (eui_color_from_gray(128) != 7) FAIL("gray 128 -> 7");
    PASS();

    TEST("color_from_rgb 4bpp");
    if (eui_color_from_rgb(0, 0, 0) != 0) FAIL("black -> 0");
    if (eui_color_from_rgb(255, 255, 255) != 15) FAIL("white -> 15");
    PASS();
}

static void test_buffer_size(void)
{
    TEST("canvas buffer size 4bpp");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (c->buf_width != MOCK_W) FAIL("width mismatch");
    if (c->buf_height != MOCK_H) FAIL("height mismatch");
    eui_canvas_destroy(c);
    PASS();
}

static void test_pixel_values(void)
{
    TEST("pixel values 1,5,10,15 preserved (4bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_set_color(c, 1);  eui_canvas_draw_dot(c, 0, 0);
    eui_canvas_set_color(c, 5);  eui_canvas_draw_dot(c, 1, 0);
    eui_canvas_set_color(c, 10); eui_canvas_draw_dot(c, 2, 0);
    eui_canvas_set_color(c, 15); eui_canvas_draw_dot(c, 3, 0);
    eui_canvas_commit(c);
    if (get_pixel_value(0, 0) != 1) FAIL("pixel(0,0) should be 1");
    if (get_pixel_value(1, 0) != 5) FAIL("pixel(1,0) should be 5");
    if (get_pixel_value(2, 0) != 10) FAIL("pixel(2,0) should be 10");
    if (get_pixel_value(3, 0) != 15) FAIL("pixel(3,0) should be 15");
    eui_canvas_destroy(c);
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== 4bpp Canvas Tests ===\n");
    test_clear();
    test_fill_rect();
    test_clear_white_bg();
    test_invert_rect();
    test_color_conversion();
    test_buffer_size();
    test_pixel_values();
    return eui_test_summary();
}
