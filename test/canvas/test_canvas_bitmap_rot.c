/*
 * test_canvas_bitmap_rot.c — eui_canvas_draw_bitmap_rot / _rot_keyed。
 *
 * 用例移植自 VAMeter app-eui 的 test_ui_draw.c（ battle-tested 的最近邻
 * 旋转 blit 断言），数据改用 eui_bitmap_t（16bpp RGB565）表达。仅在
 * EUI_COLOR_DEPTH == 16 时注册（CMakeLists 控制三档色深矩阵）。
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "eui/eui_canvas.h"
#include "common/eui_test.h"

/* 16bpp 画布测试辅助：手工构造最小画布。px_set 会读 display->caps
 * （PAGE 判定与屏宽高），所以必须挂一个 FULL 模式的 mock display。 */
static eui_display_drv_t rot_mock_display = {
    .caps = { .width = 0, .height = 0, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
};

static eui_canvas_t new_canvas(uint16_t *fb, int w, int h)
{
    eui_canvas_t c;
    memset(&c, 0, sizeof(c));
    c.buffer = (uint8_t *)fb;
    c.buf_width = w;
    c.buf_height = h;
    c.clip.x = 0; c.clip.y = 0; c.clip.w = w; c.clip.h = h;
    rot_mock_display.caps.width = w;
    rot_mock_display.caps.height = h;
    c.display = &rot_mock_display;
    return c;
}

/* eui_bitmap_t 包一个 uint16_t RGB565 数组 */
static eui_bitmap_t bmp565(int w, int h, const uint16_t *px)
{
    eui_bitmap_t b;
    b.width = w; b.height = h; b.color_depth = 16;
    b.data = (const uint8_t *)px;
    return b;
}

static void test_rot_identity_and_quarter_turns(void)
{
    TEST("bitmap rot: 0/90/-90 quarter turns match reference layout");
    uint16_t fb[16] = {0};
    eui_canvas_t c = new_canvas(fb, 4, 4);

    /* 2x3 非对称图案（w!=h，保证 90° 后宽高互换可观测），row-major */
    uint16_t img23[6] = {1, 2, 3, 4, 5, 6};

    /* 90° 顺时针，绕图像中心旋转：{1,2/3,4/5,6} → 3 宽 x 2 高
     *   5 3 1
     *   6 4 2
     * 画在 (1,0)：占据 (1..3, 0..1) */
    memset(fb, 0, sizeof(fb));
    eui_bitmap_t b23 = bmp565(2, 3, img23);
    eui_canvas_draw_bitmap_rot(&c, 1, 0, &b23, 90);
    assert(fb[0 * 4 + 1] == 5 && fb[0 * 4 + 2] == 3 && fb[0 * 4 + 3] == 1);
    assert(fb[1 * 4 + 1] == 6 && fb[1 * 4 + 2] == 4 && fb[1 * 4 + 3] == 2);
    assert(fb[0] == 0 && fb[4] == 0);          /* 旋转 bbox 外不写 */

    /* -90 归一化为 270（逆时针 90°）：{1,2/3,4/5,6} →
     *   2 4 6
     *   1 3 5
     * 画在 (1,0)：偶数宽中心采样半像素偏移 → 落位 (0..2, 1..2) */
    memset(fb, 0, sizeof(fb));
    eui_canvas_draw_bitmap_rot(&c, 1, 0, &b23, -90);
    assert(fb[1 * 4 + 0] == 2 && fb[1 * 4 + 1] == 4 && fb[1 * 4 + 2] == 6);
    assert(fb[2 * 4 + 0] == 1 && fb[2 * 4 + 1] == 3 && fb[2 * 4 + 2] == 5);
    assert(fb[0] == 0 && fb[3] == 0 && fb[7] == 0);

    /* 0° 恒等（含奇数宽中心采样）：3x2 @ (0,0) 原样，且与 draw_bitmap 逐像素一致 */
    memset(fb, 0, sizeof(fb));
    uint16_t img32[6] = {1, 2, 3, 4, 5, 6};
    eui_bitmap_t b32 = bmp565(3, 2, img32);
    eui_canvas_draw_bitmap_rot(&c, 0, 0, &b32, 0);
    uint16_t ref[16] = {0};
    eui_canvas_t cref = new_canvas(ref, 4, 4);
    eui_canvas_draw_bitmap(&cref, 0, 0, &b32);
    assert(memcmp(fb, ref, sizeof(fb)) == 0);
    PASS();
}

static void test_rot_out_of_bounds_clipping(void)
{
    TEST("bitmap rot: out-of-bounds blit clips, no crash");
    uint16_t fb[16] = {0};
    eui_canvas_t c = new_canvas(fb, 4, 4);

    /* 越界裁剪：4x4 图案 90° 画在 (2,2) → 旋转 bbox (2..5)^2，只留
     * (2..3, 2..3)；src(0,3)=13 → dest(2,2)，src(1,2)=10 → dest(3,3) */
    uint16_t img44[16];
    for (int i = 0; i < 16; i++)
        img44[i] = (uint16_t)(i + 1);
    memset(fb, 0, sizeof(fb));
    eui_bitmap_t b44 = bmp565(4, 4, img44);
    eui_canvas_draw_bitmap_rot(&c, 2, 2, &b44, 90);
    assert(fb[2 * 4 + 2] == 13);
    assert(fb[3 * 4 + 3] == 10);
    assert(fb[0] == 0 && fb[1 * 4 + 1] == 0);
    assert(fb[0 * 4 + 3] == 0 && fb[3 * 4 + 0] == 0);

    /* NULL / 零尺寸防御：不崩溃 */
    eui_canvas_draw_bitmap_rot(&c, 0, 0, NULL, 90);
    eui_bitmap_t zero = bmp565(0, 0, NULL);
    eui_canvas_draw_bitmap_rot(&c, 0, 0, &zero, 90);
    eui_canvas_draw_bitmap_rot(NULL, 0, 0, &b44, 90);
    PASS();
}

static void test_rot_keyed_transparency(void)
{
    TEST("bitmap rot keyed: key pixels preserve canvas content");
    uint16_t fb[16];
    eui_canvas_t c = new_canvas(fb, 4, 4);

    /* 背景铺满 0xAAAA；2x2 图案 {K,2 / 3,4}（K=key）0° 画出：
     * key 像素 (0,0) 与 (0,1) 不写、保留背景，其余落位 */
    for (int i = 0; i < 16; i++)
        fb[i] = 0xAAAA;
    uint16_t img[4] = {0x0003, 0x0002, 0x0003, 0x0004};
    eui_bitmap_t b = bmp565(2, 2, img);
    eui_canvas_draw_bitmap_rot_keyed(&c, 0, 0, &b, 0, 0x0003);

    assert(fb[0] == 0xAAAA);            /* src(0,0)=key：保留背景 */
    assert(fb[1] == 0x0002);
    assert(fb[4] == 0xAAAA);            /* src(0,1)=key：保留背景 */
    assert(fb[5] == 0x0004);
    PASS();
}

int main(void)
{
    eui_test_init();
    test_rot_identity_and_quarter_turns();
    test_rot_out_of_bounds_clipping();
    test_rot_keyed_transparency();
    return eui_test_summary();
}
