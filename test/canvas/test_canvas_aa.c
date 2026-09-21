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

/* ---- 2bpp/1bpp 抖动相位与密度 ---- */

#if EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2
static const uint8_t aa_bayer_expected[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};

/* 在 (ox,oy) 起 4x4 区域用 cov 混合白色到黑底，数取到 high_level 及以上的格数 */
static int dither_high_count(int ox, int oy, uint8_t cov, int high_level)
{
    int n = 0;
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++) {
            eui_canvas_px_set(aa_cur, ox + dx, oy + dy, 0);
            eui_canvas_px_blend(aa_cur, ox + dx, oy + dy, EUI_COLOR_WHITE, cov);
            if ((int)aa_get(ox + dx, oy + dy) >= high_level) n++;
        }
    return n;
}

static void test_dither_phase_matches_bayer(void)
{
    TEST("1/2bpp 抖动阈值锚在屏幕坐标的 Bayer 相位");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    /* cov=128 时收敛到 rem 使阈值落在 Bayer 中点：t <= 7 取高级，其余取低级 */
    int high = (EUI_COLOR_DEPTH == 1) ? 1 : 2;
    for (int oy = 0; oy < 4; oy++)
        for (int ox = 0; ox < 4; ox++) {
            for (int dy = 0; dy < 4; dy++)
                for (int dx = 0; dx < 4; dx++) {
                    eui_canvas_px_set(c, ox + dx, oy + dy, 0);
                    eui_canvas_px_blend(c, ox + dx, oy + dy, EUI_COLOR_WHITE, 128);
                    int want = (aa_bayer_expected[(oy + dy) & 3][(ox + dx) & 3] <= 7) ? high : high - 1;
                    if ((int)aa_get(ox + dx, oy + dy) != want) {
                        printf("\n  (%d,%d) got=%d want=%d\n", ox + dx, oy + dy,
                               (int)aa_get(ox + dx, oy + dy), want);
                        FAIL("抖动图案与 Bayer 相位不符");
                    }
                }
        }
    /* PAGE band 偏移参与相位：设 page_y_offset=1 后图案应整体上移一格 */
    c->page_y_offset = 1;
    for (int dx = 0; dx < 4; dx++) {
        eui_canvas_px_set(c, 8 + dx, 0, 0);
        eui_canvas_px_blend(c, 8 + dx, 0, EUI_COLOR_WHITE, 128);
        int want = (aa_bayer_expected[1][(8 + dx) & 3] <= 7) ? high : high - 1;
        if ((int)aa_get(8 + dx, 0) != want) FAIL("page_y_offset 未参与抖动相位");
    }
    c->page_y_offset = 0;
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_dither_density_monotone(void)
{
    TEST("覆盖度递增 → 高级格数单调不减");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    int high = (EUI_COLOR_DEPTH == 1) ? 1 : 2;
    int prev = -1;
    for (int a = 0; a <= 255; a++) {
        int n = dither_high_count(8, 8, (uint8_t)a, high);
        if (n < prev) {
            printf("\n  cov=%d 高级格数 %d < 前一个 %d\n", a, n, prev);
            FAIL("抖动密度非单调");
        }
        prev = n;
    }
    if (prev != 16) FAIL("cov=255 时 4x4 应全为高级");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_dither_phase_is_position_locked(void)
{
    TEST("C10：抖动相位锁在屏幕坐标（平移 4px 后输出逐像素相同）");
    /* 若相位跟着图形走，同一形状换位置会重新"洗牌"，动画里就会闪；
     * 4px 是 Bayer 周期，因此平移 4px 必须逐像素一致。 */
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    eui_canvas_clear(c);
    eui_canvas_set_color(c, eui_color_from_gray(255));
    eui_canvas_fill_circle(c, 60, 60, 20);
    uint32_t base[64 * 64];
    int n = 0;
    for (int y = 40; y < 76; y++)
        for (int x = 40; x < 76; x++) base[n++] = aa_get(x, y);
    eui_canvas_clear(c);
    eui_canvas_fill_circle(c, 64, 60, 20);          /* 右移 4px */
    n = 0;
    for (int y = 40; y < 76; y++)
        for (int x = 40; x < 76; x++)
            if (aa_get(x + 4, y) != base[n++])
                FAIL("4px 平移后图案不一致（抖动相位未锚定屏幕坐标）");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_no_dither_in_solid_area(void)
{
    TEST("全覆盖不抖动（实心内部零噪点）");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++) {
            eui_canvas_px_set(c, 20 + dx, 20 + dy, 0);
            eui_canvas_px_blend(c, 20 + dx, 20 + dy, EUI_COLOR_WHITE, 255);
            if ((int)aa_get(20 + dx, 20 + dy) != (int)EUI_COLOR_WHITE)
                FAIL("全覆盖像素被抖动或未写满");
        }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif /* depth 1 / 2 */

#if EUI_COLOR_DEPTH == 8
static void test_8bpp_coverage_is_pixel_value(void)
{
    TEST("8bpp：cov 即像素值（C15 依赖这条恒等式）");
    /* 注意：8bpp 下 EUI_COLOR_WHITE 是 1（eui_types.h），不是 255。
     * "最亮"必须用 eui_color_from_gray(255)，它在各色深给出该深度的最大级。 */
    const eui_color_t white = eui_color_from_gray(255);
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    if (white != 255) FAIL("8bpp 的 eui_color_from_gray(255) 应为 255");
    for (int a = 1; a <= 254; a++) {
        eui_canvas_px_set(c, 30, 30, 0);
        eui_canvas_px_blend(c, 30, 30, white, (uint8_t)a);
        if ((int)aa_get(30, 30) != a) {
            printf("\n  cov=%d → px=%d\n", a, (int)aa_get(30, 30));
            FAIL("8bpp 混合结果不等于覆盖度");
        }
    }
    /* cov=255 直接写 fg，不读 dst（dst=128 也必须给出 255） */
    eui_canvas_px_set(c, 31, 31, 128);
    eui_canvas_px_blend(c, 31, 31, white, 255);
    if ((int)aa_get(31, 31) != 255) FAIL("全覆盖时读了 dst");
    /* cov=0 完全不写 */
    eui_canvas_px_set(c, 32, 32, 77);
    eui_canvas_px_blend(c, 32, 32, white, 0);
    if ((int)aa_get(32, 32) != 77) FAIL("cov=0 仍然写了像素");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

#if EUI_COLOR_DEPTH == 16
/* Task 2 之前的 VLW 混合公式，原样复刻作为逐位一致的基准 */
static uint16_t legacy_vlw_blend(uint16_t dst, uint16_t fg, uint8_t a)
{
    uint16_t r = (uint16_t)((((fg >> 11) & 0x1Fu) * a + ((dst >> 11) & 0x1Fu) * (255u - a) + 127u) / 255u);
    uint16_t g = (uint16_t)((((fg >> 5) & 0x3Fu) * a + ((dst >> 5) & 0x3Fu) * (255u - a) + 127u) / 255u);
    uint16_t b = (uint16_t)((((fg) & 0x1Fu) * a + ((dst) & 0x1Fu) * (255u - a) + 127u) / 255u);
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void test_16bpp_blend_matches_legacy(void)
{
    TEST("16bpp px_blend 与旧 vlw 公式逐位一致");
    static const uint16_t fgs[] = { 0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xCBDE, 0x5364 };
    static const uint16_t dsts[] = { 0x0000, 0xFFFF, 0x8410, 0x1234 };
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(fgs) / sizeof(fgs[0]); i++)
        for (unsigned j = 0; j < sizeof(dsts) / sizeof(dsts[0]); j++)
            for (int a = 1; a <= 254; a++) {
                eui_canvas_px_set(c, 40, 40, dsts[j]);
                eui_canvas_px_blend(c, 40, 40, fgs[i], (uint8_t)a);
                uint16_t want = legacy_vlw_blend(dsts[j], fgs[i], (uint8_t)a);
                if ((uint16_t)aa_get(40, 40) != want) {
                    printf("\n  fg=%04x dst=%04x a=%d got=%04x want=%04x\n",
                           fgs[i], dsts[j], a, (unsigned)aa_get(40, 40), (unsigned)want);
                    FAIL("16bpp 混合与旧公式不一致");
                }
            }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

int main(void)
{
    eui_allocator_init_tlsf(aa_pool, AA_POOL_SIZE);
    printf("=== Canvas AA Tests (depth=%d) ===\n", EUI_COLOR_DEPTH);

    test_isqrt_exact_interval();
    test_isqrt_known_values();
    test_px_access_roundtrip();

#if EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2
    test_dither_phase_matches_bayer();
    test_dither_density_monotone();
    test_dither_phase_is_position_locked();
    test_no_dither_in_solid_area();
#endif

#if EUI_COLOR_DEPTH == 8
    test_8bpp_coverage_is_pixel_value();
#endif

#if EUI_COLOR_DEPTH == 16
    test_16bpp_blend_matches_legacy();
#endif

    return eui_test_summary();
}
