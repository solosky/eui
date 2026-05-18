#include "eui/eui_canvas.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include <stdio.h>
#include <string.h>

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

#define MOCK_W 128
#define MOCK_H 64
static uint8_t mock_buf[MOCK_W * MOCK_H / 8];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int bytes_per_row = r->w / 8;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (MOCK_W / 8) + r->x / 8),
               b + row * bytes_per_row, bytes_per_row);
    }
}

static eui_display_hal_t mock_display = {
    .caps = { .width = MOCK_W, .height = MOCK_H, .color_depth = 1, .buffer_mode = 1, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int count_pixels(void)
{
    int count = 0;
    for (int i = 0; i < (int)sizeof(mock_buf); i++) {
        uint8_t b = mock_buf[i];
        while (b) { count += (b & 1); b >>= 1; }
    }
    return count;
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
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Canvas Tests ===\n");
    test_clear();
    test_fill_rect();
    test_clip();
    test_save_restore();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
