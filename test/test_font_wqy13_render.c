#include "eui/eui.h"
#include "eui/eui_font_wqy13.h"
#include <string.h>
#include <stdio.h>

#define POOL_SIZE (65536)
static uint8_t mem_pool[POOL_SIZE];
static int tests_run = 0, tests_passed = 0;
#define TEST(n)   do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS()    do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m)   do { printf("FAIL: %s\n", m); return; } while(0)

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
    int bytes_per_row = (int)r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 8) + (int)r->x / 8),
               b + row * bytes_per_row, (size_t)bytes_per_row);
#else
    int bytes_per_row = (int)r->w * (EUI_COLOR_DEPTH / 8);
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * MOCK_W + (int)r->x) * (EUI_COLOR_DEPTH / 8),
               b + row * bytes_per_row, (size_t)bytes_per_row);
#endif
}

static eui_display_hal_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H,
              .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

#if EUI_COLOR_DEPTH == 1
static int count_pixels(void) {
    int c = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        uint8_t b = mock_buf[i];
        while (b) { c += (b & 1); b >>= 1; }
    }
    return c;
}
#else
static int count_pixels(void) {
    int c = 0;
    uint16_t *p = (uint16_t*)mock_buf;
    for (int i = 0; i < (int)(sizeof(mock_buf) / 2); i++)
        if (p[i] != 0) c++;
    return c;
}
#endif

static void test_wqy13_renders_chinese(void)
{
    TEST("wqy13 renders '应用列表' with non-zero pixels");
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) FAIL("canvas create failed");
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_draw_str(c, 0, 0, "应用列表");
    eui_canvas_commit(c);
    int px = count_pixels();
    eui_canvas_destroy(c);
    if (px == 0) FAIL("zero pixels rendered");
    printf("(%d pixels) ", px);
    PASS();
}

static void test_wqy13_str_width(void)
{
    TEST("wqy13 str_width returns non-zero for '应用列表'");
    uint16_t w = eui_font_get_str_width(&eui_font_wqy13, "应用列表");
    if (w == 0) FAIL("str_width is 0");
    printf("(width=%d) ", w);
    PASS();
}

static void test_wqy13_font_metadata(void)
{
    TEST("wqy13 font metadata is valid");
    if (eui_font_get_height(&eui_font_wqy13) == 0) FAIL("height is 0");
    if (eui_font_get_baseline(&eui_font_wqy13) == 0) FAIL("baseline is 0");
    if (eui_font_wqy13.format != EUI_FONT_FORMAT_U8G2) FAIL("wrong format");
    if (eui_font_wqy13.data == NULL) FAIL("data is NULL");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== wqy13 Font Render Test ===\n");
    test_wqy13_font_metadata();
    test_wqy13_str_width();
    test_wqy13_renders_chinese();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
