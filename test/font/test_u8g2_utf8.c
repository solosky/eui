/* u8g2 字体的 UTF-8 语义回归：测宽路径必须与绘制路径同解（UTF-8 码点），
 * 纵向锚定必须与其余字体格式（VLW/BDF=行盒顶部）一致。
 *
 * 两个历史缺陷（本文件即其失败测试）：
 *  1. eui_font_u8g2_get_str_width 按字节累加——"中"(3 字节 UTF-8) 返回 0，
 *     导致 CN 菜单 keyframe 宽度全 0（app 层选中框塌缩）；
 *  2. draw_u8g2_glyph 以 y=基线锚定，而 VLW/BDF 与全部调用方为行盒顶部
 *     语义——CJK 文本整体上浮约一个字高（标题截顶、行错位）。
 * 用 wqy12_ch1（含 中 U+4E2D）与 profont10（ASCII）做正反两向夹逼。 */
#include "eui/eui_canvas.h"
#include "eui/eui_canvas_internal.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include "eui/eui_font_internal.h"
#include "data/test_u8g2_profont10_data.h"
#include "data/test_u8g2_wqy12_ch1_data.h"
#include <stdio.h>
#include <string.h>

#if EUI_FONT_ENABLE_U8G2

#define POOL_SIZE 262144
static uint8_t pool[POOL_SIZE];

#define W 128
#define H 128

static void mock_write(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)b; (void)r; (void)ud;
}

static eui_display_drv_t mock_display = {
    .caps = { .width = W, .height = H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write,
};

static const eui_font_t profont10 = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 10,
    .baseline = 8,
    .flags = 0,
    .data = u8g2_font_profont10_tf_data,
    .lookup_glyph = NULL,
};

static const eui_font_t wqy12 = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 12,
    .baseline = 10,
    .flags = 0,
    .data = u8g2_font_wqy12_ch1_data,
    .lookup_glyph = eui_font_u8g2_lookup_glyph,
};

static int lit_rows(eui_canvas_t *c, int *min_y, int *max_y)
{
    int n = 0;
    *min_y = -1;
    *max_y = -1;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (eui_canvas_px_get(c, (int16_t)x, (int16_t)y) != 0) {
                n++;
                if (*min_y < 0)
                    *min_y = y;
                *max_y = y;
            }
    return n;
}

static void test_str_width_utf8(void)
{
    TEST("u8g2 str_width 解码 UTF-8 码点（非字节）");
    /* 纯 CJK：按字节查表必为 0；按码点应为 full-width 步进 > 0 */
    uint16_t w_cjk = eui_font_get_str_width(&wqy12, "\xe4\xb8\xad");   /* 中 */
    if (w_cjk == 0)
        FAIL("str_width(中) 不应为 0（字节遍历未解码 UTF-8）");
    if (w_cjk > wqy12.line_height * 2)
        FAIL("str_width(中) 超出两倍行高，疑似按字节累加了垃圾步进");
    /* 混排 = ASCII + CJK 线性可加 */
    uint16_t w_mix = eui_font_get_str_width(&wqy12, "A\xe4\xb8\xad");
    uint16_t w_a = eui_font_get_str_width(&wqy12, "A");
    if (w_mix != (uint16_t)(w_a + w_cjk))
        FAIL("混排宽度应等于 ASCII 与 CJK 段之和");
    PASS();
}

static void test_width_paths_agree(void)
{
    TEST("font str_width 与 canvas_str_width 两路径一致");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) FAIL("canvas create failed");
    eui_canvas_set_font(c, &wqy12);
    uint16_t via_font = eui_font_get_str_width(&wqy12, "A\xe4\xb8\xad");
    uint16_t via_canvas = eui_canvas_str_width(c, "A\xe4\xb8\xad");
    eui_canvas_destroy(c);
    if (via_font != via_canvas)
        FAIL("font 与 canvas 两条测宽路径结果不一致");
    PASS();
}

static void test_draw_str_top_anchor(void)
{
    TEST("u8g2 draw_str 行盒顶部锚定（与 VLW/BDF 同语义）");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) FAIL("canvas create failed");
    eui_canvas_set_font(c, &profont10);
    eui_canvas_set_color(c, (eui_color_t)1);
    eui_canvas_clear(c);

    (void)eui_canvas_draw_str(c, 10, 60, "A");
    int min_y, max_y, n;
    n = lit_rows(c, &min_y, &max_y);
    eui_canvas_destroy(c);
    if (n == 0)
        FAIL("未画出任何像素");
    /* 顶部锚定：'A'（h≈7, baseline=8）应落在 [60+8-7, ...] ≈ 68 起；
     * 基线锚定的旧实现悬在 [53, 60)。阈值 58 精确区分两者。 */
    if (min_y < 58)
        FAIL("字形出现在锚点上方约一个字高——仍是基线锚定");
    if (max_y > 60 + profont10.baseline + 2)
        FAIL("字形超出锚点行 + baseline + 容差");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(pool, POOL_SIZE);
    printf("=== u8g2 UTF-8 width/anchor Tests (depth=%d) ===\n", EUI_COLOR_DEPTH);

    test_str_width_utf8();
    test_width_paths_agree();
    test_draw_str_top_anchor();

    return eui_test_summary();
}
#else
int main(void)
{
    printf("u8g2 support disabled; nothing to test\n");
    return 0;
}
#endif
