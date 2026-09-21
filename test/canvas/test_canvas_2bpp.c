#include "eui/eui_canvas.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define MOCK_W 128
#define MOCK_H 64
#define MOCK_BUF_SIZE (MOCK_W * MOCK_H / 4)  /* 4 pixels per byte */

static uint8_t mock_buf[MOCK_BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int bytes_per_row = r->w / 4;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 4) + r->x / 4),
               b + row * bytes_per_row, bytes_per_row);
    }
}

static eui_display_drv_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = 2,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int count_nonzero_pixels(void)
{
    int count = 0;
    for (int y = 0; y < MOCK_H; y++) {
        for (int x = 0; x < MOCK_W; x++) {
            int byte_idx = y * (MOCK_W / 4) + (x / 4);
            int shift = 6 - 2 * (x % 4);
            if ((mock_buf[byte_idx] >> shift) & 3) count++;
        }
    }
    return count;
}

static int get_pixel_value(int x, int y)
{
    int byte_idx = y * (MOCK_W / 4) + (x / 4);
    int shift = 6 - 2 * (x % 4);
    return (mock_buf[byte_idx] >> shift) & 3;
}

static void test_clear(void)
{
    TEST("clear fills with background (2bpp)");
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
    TEST("fill_rect fills correct area (2bpp)");
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
    TEST("clear with white background (2bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    eui_canvas_set_bg_color(c, 3);
    eui_canvas_clear(c);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != MOCK_W * MOCK_H) FAIL("all pixels should be non-zero");
    if (get_pixel_value(0, 0) != 3) FAIL("pixel should be white (3)");
    eui_canvas_destroy(c);
    PASS();
}

static void test_invert_rect(void)
{
    TEST("invert_rect toggles pixels (2bpp)");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));

    /* Fill a rect with white (3).  Invert it: 3^3=0, outside stays 0. */
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    eui_canvas_invert_rect(c, 10, 10, 20, 10);
    eui_canvas_commit(c);
    if (count_nonzero_pixels() != 0) FAIL("invert of white rect should clear all pixels");
    if (get_pixel_value(10, 10) != 0) FAIL("inverted white pixel should be black");

    /* Invert a black (0) area: 0^3=3 */
    eui_canvas_invert_rect(c, 0, 0, 10, 10);
    eui_canvas_commit(c);
    if (get_pixel_value(0, 0) != 3) FAIL("inverted black pixel should become white");
    if (count_nonzero_pixels() != 100) FAIL("invert 10x10 black -> 100 white");

    eui_canvas_destroy(c);
    PASS();
}

static void test_color_conversion(void)
{
    TEST("color_from_gray 2bpp");
    if (eui_color_from_gray(0) != 0) FAIL("gray 0 -> 0");
    if (eui_color_from_gray(255) != 3) FAIL("gray 255 -> 3");
    if (eui_color_from_gray(42) != 0) FAIL("gray 42 -> 0");
    if (eui_color_from_gray(85) != 1) FAIL("gray 85 -> 1");
    if (eui_color_from_gray(170) != 2) FAIL("gray 170 -> 2");
    PASS();

    TEST("color_from_rgb 2bpp");
    if (eui_color_from_rgb(0, 0, 0) != 0) FAIL("black -> 0");
    if (eui_color_from_rgb(255, 255, 255) != 3) FAIL("white -> 3");
    PASS();
}

static void test_buffer_size(void)
{
    TEST("canvas buffer size 2bpp");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (c->buf_width != MOCK_W) FAIL("width mismatch");
    if (c->buf_height != MOCK_H) FAIL("height mismatch");
    eui_canvas_destroy(c);
    PASS();
}

static void test_pixel_values(void)
{
    TEST("pixel values 0-3 preserved");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    memset(mock_buf, 0, sizeof(mock_buf));

    eui_canvas_set_color(c, 1);
    eui_canvas_draw_dot(c, 0, 0);
    eui_canvas_commit(c);
    if (get_pixel_value(0, 0) != 1) FAIL("pixel(0,0) should be 1");

    eui_canvas_set_color(c, 2);
    eui_canvas_draw_dot(c, 1, 0);
    eui_canvas_commit(c);
    if (get_pixel_value(1, 0) != 2) FAIL("pixel(1,0) should be 2");

    eui_canvas_set_color(c, 3);
    eui_canvas_draw_dot(c, 2, 0);
    eui_canvas_commit(c);
    if (get_pixel_value(2, 0) != 3) FAIL("pixel(2,0) should be 3");

    eui_canvas_destroy(c);
    PASS();
}

/* ---- AA 目检画廊：只产出人工目检产物，不做新的像素断言 ---- */

static void put32le(uint8_t *p, uint32_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

/* 2bpp 灰度级 → 24 位 BMP（自下而上、BGR、4 字节行对齐） */
static int write_level_bmp(const char *fn)
{
    int row_bytes = (MOCK_W * 3 + 3) & ~3;
    uint32_t data_size = (uint32_t)row_bytes * (uint32_t)MOCK_H;
    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    put32le(hdr + 2, 54u + data_size);
    put32le(hdr + 10, 54u);
    put32le(hdr + 14, 40u);
    put32le(hdr + 18, (uint32_t)MOCK_W);
    put32le(hdr + 22, (uint32_t)MOCK_H);
    hdr[26] = 1; hdr[28] = 24;
    put32le(hdr + 34, data_size);

    FILE *f = fopen(fn, "wb");
    if (!f) return -1;
    fwrite(hdr, 1, 54, f);
    uint8_t row[MOCK_W * 3 + 4];
    for (int y = MOCK_H - 1; y >= 0; y--) {
        memset(row, 0, (size_t)row_bytes);
        for (int x = 0; x < MOCK_W; x++) {
            uint8_t g = (uint8_t)(get_pixel_value(x, y) * 85);   /* 0,85,170,255 */
            row[x * 3] = g; row[x * 3 + 1] = g; row[x * 3 + 2] = g;   /* BGR */
        }
        fwrite(row, 1, (size_t)row_bytes, f);
    }
    fclose(f);
    return 0;
}

static void print_level_histogram(void)
{
    int hist[4] = { 0, 0, 0, 0 };
    for (int y = 0; y < MOCK_H; y++)
        for (int x = 0; x < MOCK_W; x++) hist[get_pixel_value(x, y)]++;
    printf("  level histogram: 0=%d 1=%d 2=%d 3=%d\n",
           hist[0], hist[1], hist[2], hist[3]);
}

/* 逐像素打印灰度级（' '=0 '.'=1 '+'=2 '#'=3），供人目检抖动网点 */
static void print_level_map(void)
{
    static const char ch[4] = { ' ', '.', '+', '#' };
    for (int y = 0; y < MOCK_H; y++) {
        for (int x = 0; x < MOCK_W; x++) putchar(ch[get_pixel_value(x, y)]);
        putchar('\n');
    }
}

static void test_aa_gallery(void)
{
    printf("=== 2bpp AA gallery（目检抖动，无断言）===\n");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) { printf("FAIL: create\n"); return; }
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    /* 用"该色深的最亮级"而不是 EUI_COLOR_WHITE：8bpp 下后者是 1（近黑） */
    eui_canvas_set_color(c, eui_color_from_gray(255));

    /* 形状家族（128x64 里排成三带）：弧/环/圆/扇形/圆角矩形 */
    eui_canvas_draw_arc(c,  14, 14, 11, 1, -90,  90);      /* 发丝弧 */
    eui_canvas_draw_arc(c,  40, 14, 11, 4,   0, 300);      /* 粗弧 */
    eui_canvas_draw_ring(c, 66, 14, 11, 6,   0, 270);      /* 圆环 */
    eui_canvas_fill_circle(c, 88, 14, 2);                  /* 极小半径 */
    eui_canvas_draw_circle(c, 88, 14, 6);
    eui_canvas_fill_round_rect(c, 102,  4, 24, 9, 3);      /* 圆角矩形角部 */
    eui_canvas_draw_round_rect(c, 102, 16, 24, 9, 3);

    /* 覆盖度阶梯：半径递增的实心圆 + 同半径 1px / 3px 弧（每像素覆盖度真的不同） */
    eui_canvas_fill_pie(c,   12, 38, 10, 30, 210);         /* 扇形 */
    eui_canvas_fill_circle(c, 40, 38, 1);
    eui_canvas_fill_circle(c, 48, 38, 2);
    eui_canvas_fill_circle(c, 56, 38, 3);
    eui_canvas_fill_circle(c, 65, 38, 4);
    eui_canvas_draw_arc(c,   86, 40, 11, 1, -90, 90);      /* 1px 带 */
    eui_canvas_draw_arc(c,  112, 40, 11, 3, -90, 90);      /* 3px 带 */

    /* 越界与 clip：跨出画布边缘 / 被 clip 矩形切断 */
    eui_canvas_fill_circle(c, 130, 64, 16);                /* 圆心在画布外：右/下边缘切 */
    eui_rect_t cut = { 20, 49, 40, 14 };
    eui_canvas_set_clip(c, &cut);
    eui_canvas_fill_circle(c, 30, 57, 14);                 /* 被 clip 切掉左/上/下 */
    eui_canvas_fill_round_rect(c, 45, 51, 30, 10, 3);      /* 被 clip 切掉右 */
    eui_canvas_clear_clip(c);                              /* 复位（set_clip(NULL) 是空操作） */
    eui_canvas_commit(c);

    if (write_level_bmp("test_canvas_2bpp.bmp") == 0)
        printf("  -> test_canvas_2bpp.bmp (%dx%d 24-bit BMP)\n", MOCK_W, MOCK_H);
    else
        printf("  FAIL: BMP write\n");
    print_level_histogram();
    print_level_map();

    eui_canvas_destroy(c);
}

int main(void)
{
    eui_test_init();
    printf("=== 2bpp Canvas Tests ===\n");
    test_clear();
    test_fill_rect();
    test_clear_white_bg();
    test_invert_rect();
    test_color_conversion();
    test_buffer_size();
    test_pixel_values();
    test_aa_gallery();
    return eui_test_summary();
}
