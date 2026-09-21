#include "eui/eui_canvas.h"
#include "eui/eui_canvas_internal.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

/* C15 参考一致性测试要放 2*120+4 的画布；16bpp 下约 135KB，因此自建池 */
#define AA_POOL_SIZE 262144
static uint8_t aa_pool[AA_POOL_SIZE];

#define AA_W 260
#define AA_H 260

/* 测试直接通过 eui_canvas_px_get 读 canvas 自己的缓冲，刷新回调无需做任何事 */
static void aa_mock_write(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)b; (void)r; (void)ud;
}

static eui_display_drv_t aa_mock_display = {
    .caps = { .width = AA_W, .height = AA_H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = aa_mock_write,
};

static eui_canvas_t *aa_new_canvas(void)
{
    eui_canvas_t *c = eui_canvas_create(&aa_mock_display);
    if (!c) return NULL;
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    /* 用"该色深的最亮级"而不是 EUI_COLOR_WHITE：8bpp 下后者是 1（近黑） */
    eui_canvas_set_color(c, eui_color_from_gray(255));
    return c;
}

static eui_canvas_t *aa_cur = NULL;   /* 供 aa_get 使用，由各测试在末尾复位 */

/* 取像素：统一走内部访问器，全色深同一写法 */
static uint32_t aa_get(int x, int y) { return (uint32_t)eui_canvas_px_get(aa_cur, x, y); }

static void test_isqrt_exact_interval(void)
{
    TEST("isqrt 落在精确区间内（floor(sqrt(x)*2^f) 的定义）");
    static const uint8_t fracs[] = { 0, 8 };
    for (unsigned fi = 0; fi < sizeof(fracs) / sizeof(fracs[0]); fi++) {
        uint8_t f = fracs[fi];
        for (uint64_t x = 0; x <= 115200; x++) {          /* 240x240 画布的距离平方上界 */
            uint64_t r = eui_canvas_isqrt(x, f);
            uint64_t scaled = x << (2u * f);
            if (!(r * r <= scaled && scaled < (r + 1) * (r + 1))) {
                printf("\n  x=%llu f=%u r=%llu\n", (unsigned long long)x, f,
                       (unsigned long long)r);
                FAIL("isqrt 区间性质不成立");
            }
        }
    }
    PASS();
}

static void test_isqrt_known_values(void)
{
    TEST("isqrt 定点值");
    if (eui_canvas_isqrt(0, 0) != 0) FAIL("isqrt(0) != 0");
    if (eui_canvas_isqrt(1, 0) != 1) FAIL("isqrt(1) != 1");
    if (eui_canvas_isqrt(2, 0) != 1) FAIL("isqrt(2) != 1");
    if (eui_canvas_isqrt(3, 0) != 1) FAIL("isqrt(3) != 1");
    if (eui_canvas_isqrt(14400, 0) != 120) FAIL("isqrt(14400) != 120");
    if (eui_canvas_isqrt(1, 8) != 256) FAIL("isqrt(1,8) != 256");
    if (eui_canvas_isqrt(115200, 0) != 339) FAIL("isqrt(115200) != 339");
    PASS();
}

static void test_px_access_roundtrip(void)
{
    TEST("px_set/px_get 在各色深下往返一致");
    eui_canvas_t *c = aa_new_canvas();
    if (!c) FAIL("canvas create failed");
    aa_cur = c;
    eui_color_t fg = eui_canvas_px_get(c, 5, 5);
    eui_canvas_px_set(c, 5, 5, c->fg_color);
    if (aa_get(5, 5) != (uint32_t)c->fg_color) FAIL("px_get 未能读回 px_set 写入的值");
    if (aa_get(6, 5) != (uint32_t)fg) FAIL("px_set 越界写到了相邻像素");
    /* clip 外不写、读回 0 */
    eui_rect_t clip = { 10, 10, 4, 4 };
    eui_canvas_set_clip(c, &clip);
    eui_canvas_px_set(c, 5, 5, c->fg_color);
    if (eui_canvas_px_get(c, 5, 5) != 0) FAIL("clip 外读回应为 0");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(aa_pool, AA_POOL_SIZE);
    printf("=== Canvas AA Tests (depth=%d) ===\n", EUI_COLOR_DEPTH);

    test_isqrt_exact_interval();
    test_isqrt_known_values();
    test_px_access_roundtrip();

    return eui_test_summary();
}
