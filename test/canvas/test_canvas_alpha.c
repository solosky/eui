/* eui_canvas_fill_rect_alpha 语义测试（全色深不变式 + 16bpp 位精确混合值）。
 *
 * 模式沿用 test_canvas_aa.c：自建 tlsf 池、mock 驱动、px_get 直接读画布。
 * FAIL() 是 printf 后 return（非 longjmp），失败测试的画布会漏在池里——
 * 画布取 64x64（16bpp 下 8KB），64KB 池可吸收多次失败。 */
#include "eui/eui_canvas.h"
#include "eui/eui_canvas_internal.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define ALPHA_POOL_SIZE 65536
static uint8_t alpha_pool[ALPHA_POOL_SIZE];

#define AW 64
#define AH 64

static void alpha_mock_write(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)b; (void)r; (void)ud;
}

static eui_display_drv_t alpha_mock_display = {
    .caps = { .width = AW, .height = AH, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = alpha_mock_write,
};

/* 白底画布：该色深的最亮级（16bpp 下是 0xFFFF） */
static eui_canvas_t *alpha_new_canvas(void)
{
    eui_canvas_t *c = eui_canvas_create(&alpha_mock_display);
    if (!c) return NULL;
    eui_canvas_set_bg_color(c, eui_color_from_gray(255));
    eui_canvas_clear(c);
    return c;
}

static uint32_t px(const eui_canvas_t *c, int x, int y)
{
    return (uint32_t)eui_canvas_px_get((eui_canvas_t *)c, (int16_t)x, (int16_t)y);
}

static void test_alpha0_writes_nothing(void)
{
    TEST("fill_rect_alpha alpha=0 是无操作");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    uint32_t before = px(c, 10, 10);
    eui_canvas_fill_rect_alpha(c, 5, 5, 20, 10, EUI_COLOR_BLACK, 0);
    if (px(c, 10, 10) != before) FAIL("alpha=0 改写了像素");
    eui_canvas_destroy(c);
    PASS();
}

static void test_alpha255_equals_fill_rect(void)
{
    TEST("fill_rect_alpha alpha=255 等价不透明填充");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    eui_canvas_fill_rect_alpha(c, 5, 5, 20, 10, EUI_COLOR_BLACK, 255);
    if (px(c, 5, 5) != (uint32_t)EUI_COLOR_BLACK ||
        px(c, 24, 14) != (uint32_t)EUI_COLOR_BLACK)
        FAIL("alpha=255 未产生不透明填充");
    /* 区域外不受影响 */
    if (px(c, 4, 5) != (uint32_t)eui_color_from_gray(255) ||
        px(c, 25, 14) != (uint32_t)eui_color_from_gray(255))
        FAIL("填充越出矩形边界");
    eui_canvas_destroy(c);
    PASS();
}

#if EUI_COLOR_DEPTH == 16
static void test_16bpp_exact_blend(void)
{
    TEST("16bpp 位精确混合（blend_565 公式期望值）");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    /* 白底上黑 alpha=128 → 0x7BEF（半白灰）：r=b=(31*127+127)/255=15, g=31 */
    eui_canvas_fill_rect_alpha(c, 0, 0, 8, 1, EUI_COLOR_BLACK, 128);
    if (px(c, 3, 0) != 0x7BEFu)
        FAIL("黑 50% 混白底期望 0x7BEF");
    /* 白底上红(RGB565 0xF800) alpha=128 → r=31,g=31,b=15 = 0xFBEF */
    eui_canvas_fill_rect_alpha(c, 0, 1, 8, 1, 0xF800u, 128);
    if (px(c, 3, 1) != 0xFBEFu)
        FAIL("红 50% 混白底期望 0xFBEF");
    eui_canvas_destroy(c);
    PASS();
}
#endif

static void test_clip_honored(void)
{
    TEST("fill_rect_alpha 受 clip 约束");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    eui_rect_t clip = { 0, 0, AW / 2, AH };
    eui_canvas_set_clip(c, &clip);
    eui_canvas_fill_rect_alpha(c, 0, 0, AW, AH, EUI_COLOR_BLACK, 255);
    eui_canvas_clear_clip(c);
    if (px(c, AW / 2 - 1, 10) != (uint32_t)EUI_COLOR_BLACK)
        FAIL("clip 内像素未被填充");
    if (px(c, AW / 2, 10) != (uint32_t)eui_color_from_gray(255))
        FAIL("clip 外像素被改写");
    eui_canvas_destroy(c);
    PASS();
}

static void test_partial_offscreen(void)
{
    TEST("矩形越出画布边界：域内混合、越界丢弃");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    /* 右下角越界 8px：不应崩溃，域内像素照常写 */
    eui_canvas_fill_rect_alpha(c, AW - 4, AH - 4, 12, 12, EUI_COLOR_BLACK, 255);
    if (px(c, AW - 1, AH - 1) != (uint32_t)EUI_COLOR_BLACK)
        FAIL("域内边缘像素未填充");
    eui_canvas_destroy(c);
    PASS();
}

static void test_cov128_changes_area(void)
{
    TEST("cov=128 全色深不变式：域内像素部分/全部改变且非全改写");
    eui_canvas_t *c = alpha_new_canvas();
    if (!c) FAIL("canvas create failed");
    uint32_t bg = (uint32_t)eui_color_from_gray(255);
    eui_canvas_fill_rect_alpha(c, 10, 10, 20, 10, EUI_COLOR_BLACK, 128);
    int changed = 0;
    for (int y = 10; y < 20; y++)
        for (int x = 10; x < 30; x++)
            if (px(c, x, y) != bg) changed++;
    /* 1/2bpp 抖动约半数；4/8/16bpp 全部改变 */
    if (changed < 20 * 10 / 4 || changed > 20 * 10)
        FAIL("混合后变化像素数不在合理区间");
    eui_canvas_destroy(c);
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(alpha_pool, ALPHA_POOL_SIZE);
    printf("=== Canvas fill_rect_alpha Tests (depth=%d) ===\n", EUI_COLOR_DEPTH);

    test_alpha0_writes_nothing();
    test_alpha255_equals_fill_rect();
#if EUI_COLOR_DEPTH == 16
    test_16bpp_exact_blend();
#endif
    test_clip_honored();
    test_partial_offscreen();
    test_cov128_changes_area();

    return eui_test_summary();
}
