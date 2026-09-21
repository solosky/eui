#include "eui/eui_canvas.h"
#include "eui/eui_canvas_internal.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* C15 参考一致性测试要放 2*120+4 的画布；16bpp 下约 135KB，因此自建池。
 * 池要能同时容下**两个** 16bpp 画布：FAIL() 是 longjmp 出测试函数的，失败的那条
 * 测试走不到末尾的 eui_canvas_destroy，于是它的 135KB 画布会留在池里。 */
#define AA_POOL_SIZE 393216
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

/* 参考光栅器：64 子样本独立判定"像素中心落在圆内"的面积占比（0..256）。
 * 与实现完全独立，是 C15 的判别性基准。 */
static uint32_t aa_ref_disc_cov(int px, int py, int cx, int cy, int r)
{
    int hit = 0;
    for (int sy = 0; sy < 8; sy++)
        for (int sx = 0; sx < 8; sx++) {
            /* 用整数比较避免浮点：以 1/16 px 为单位 */
            int32_t x = ((px * 16) + 2 * sx + 1) - cx * 16;
            int32_t y = ((py * 16) + 2 * sy + 1) - cy * 16;
            if (x * x + y * y <= r * 16 * r * 16) hit++;
        }
    return (uint32_t)((hit * 256 + 32) / 64);
}

/* 参考方向向量：从 1° 旋转矩阵递推，Q30 精度、不用 libm、与实现的 Q14 表无关。
 * （360 步累积的舍入误差约 3e-7，远小于 1/16 px 的容差） */
#define REF_COS1 1073578288LL   /* round(cos 1° * 2^30) —— 实测值，勿手改 */
#define REF_SIN1   18739379LL   /* round(sin 1° * 2^30) —— 实测值，勿手改 */
static void aa_ref_dir(int deg, int64_t *ux, int64_t *uy)
{
    int64_t cx = 1LL << 30, cy = 0;                 /* 0° = (1, 0) */
    int d = deg % 360;
    if (d < 0) d += 360;
    for (int i = 0; i < d; i++) {
        int64_t nx = (cx * REF_COS1 - cy * REF_SIN1) >> 30;
        int64_t ny = (cx * REF_SIN1 + cy * REF_COS1) >> 30;
        cx = nx; cy = ny;
    }
    *ux = cx; *uy = cy;
}

/* 扇区参考：圆盘参考 + 角度区间判定。角度判定用"半平面交（sweep<=180）/
 * 补扇形交的补（sweep>180）"两种数学形式，与实现的两条公式同源但独立实现；
 * 被验证的是覆盖度与光栅化管线，而非三角恒等式。 */
static uint32_t aa_ref_sector_cov(int px, int py, int cx, int cy,
                                  int r_in, int r_out, int a0, int a1)
{
    int sweep = a1 - a0;
    int has_caps = !(sweep >= 360 || sweep <= -360);
    int complement = has_caps && sweep > 180;
    int64_t ux0 = 1LL << 30, uy0 = 0, ux1 = 1LL << 30, uy1 = 0;
    if (has_caps) {
        if (complement) { aa_ref_dir(a1, &ux0, &uy0); aa_ref_dir(a0, &ux1, &uy1); }
        else            { aa_ref_dir(a0, &ux0, &uy0); aa_ref_dir(a1, &ux1, &uy1); }
    }
    int hit = 0;
    for (int sy = 0; sy < 8; sy++)
        for (int sx = 0; sx < 8; sx++) {
            int32_t dx = ((px * 16) + 2 * sx + 1) - cx * 16;
            int32_t dy = ((py * 16) + 2 * sy + 1) - cy * 16;
            int64_t d2 = (int64_t)dx * dx + (int64_t)dy * dy;
            if (d2 < (int64_t)(r_in * 16) * (r_in * 16)) continue;
            if (d2 > (int64_t)(r_out * 16) * (r_out * 16)) continue;
            if (has_caps) {
                if (complement) {
                    /* 在补扇形内则排除 */
                    if ((ux0 * dy - uy0 * dx) >= 0 && (ux1 * dy - uy1 * dx) <= 0) continue;
                } else {
                    if ((ux0 * dy - uy0 * dx) < 0) continue;   /* 起点帽内侧 ⟺ cross >= 0 */
                    if ((ux1 * dy - uy1 * dx) > 0) continue;   /* 终点帽内侧 ⟺ cross <= 0 */
                }
            }
            hit++;
        }
    return (uint32_t)((hit * 256 + 32) / 64);
}

#if EUI_COLOR_DEPTH == 8
static void test_reference_agreement_sector(void)   /* C8/C9 的一般化验证 */
{
    TEST("C15（扇区）：与 64 子样本参考一致，含跨 180° 与整圆（8bpp，容差 24/256）");
    /* 跨 180° 的两种路径与补扇形公式都在这组 sweep 里被逐像素比对；
     * 若实现把大弧拆成两段绘制，接缝处会叠加成 ~0.25·dst+0.75·fg，
     * 偏差约 63/256，必然超出容差。 */
    static const int sweeps[] = { 1, 45, 120, 179, 180, 181, 270, 359, 360 };
    const int r = 40, cx = 130, cy = 130;
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(sweeps) / sizeof(sweeps[0]); i++) {
        int sw = sweeps[i];
        eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
        eui_canvas_clear(c);
        eui_canvas_set_color(c, eui_color_from_gray(255));
        eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 30, (int16_t)(30 + sw));
        for (int y = cy - r - 2; y <= cy + r + 2; y++)
            for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                uint32_t want = aa_ref_sector_cov(x, y, cx, cy, 0, r, 30, 30 + sw);
                uint32_t got  = aa_get(x, y);
                uint32_t diff = (got > want) ? (got - want) : (want - got);
                if (diff > 24) {
                    printf("\n  sweep=%d (%d,%d) got=%u want=%u\n",
                           sw, x - cx, y - cy, got, want);
                    FAIL("扇区与参考偏差超容差（可能是接缝双重混合或覆盖度模型错误）");
                }
            }
    }
    /* 圆环（内孔）也要过参考 */
    {
        eui_canvas_clear(c);
        eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 25, 0, 360);
        for (int y = cy - r - 2; y <= cy + r + 2; y++)
            for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                uint32_t want = aa_ref_sector_cov(x, y, cx, cy, 25, r, 0, 360);
                uint32_t got  = aa_get(x, y);
                uint32_t diff = (got > want) ? (got - want) : (want - got);
                if (diff > 24) {
                    printf("\n  ring (%d,%d) got=%u want=%u\n", x - cx, y - cy, got, want);
                    FAIL("圆环与参考偏差超容差");
                }
            }
    }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

#if EUI_COLOR_DEPTH == 8
static void test_reference_agreement_disc(void)   /* C15 @8bpp */
{
    TEST("C15：与 64 子样本参考一致（8bpp，容差 24/256）");
    static const int radii[] = { 3, 4, 8, 20, 60, 120 };
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(radii) / sizeof(radii[0]); i++) {
        int r = radii[i];
        /* 两种圆心相位：落在像素中心 / 落在像素角 */
        for (int ph = 0; ph < 2; ph++) {
            int cx = 130 + ph, cy = 130;
            eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
            eui_canvas_clear(c);
            eui_canvas_set_color(c, eui_color_from_gray(255));   /* 8bpp 下 EUI_COLOR_WHITE 是 1，不是 255 */
            eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 0, 360);
            for (int y = cy - r - 2; y <= cy + r + 2; y++)
                for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                    uint32_t want = aa_ref_disc_cov(x, y, cx, cy, r);
                    uint32_t got  = aa_get(x, y);            /* 8bpp: 像素值即覆盖度 */
                    uint32_t d    = (got > want) ? (got - want) : (want - got);
                    if (d > 24) {
                        printf("\n  r=%d phase=%d (%d,%d) got=%u want=%u\n",
                               r, ph, x - cx, y - cy, got, want);
                        FAIL("与参考偏差超容差");
                    }
                }
        }
    }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

#if EUI_COLOR_DEPTH == 16
static void test_reference_agreement_disc_16(void)  /* C15 @16bpp：比绿通道 */
{
    TEST("C15：与 64 子样本参考一致（16bpp 绿通道，容差 6 级）");
    /* 容差取 6 级（≈24.3/256，与本任务 8bpp 门的 24/256 同一预算），不是 ±2：
     * - 8x8 参考自身就有 ±2.55 级偏置（r=120 的 (-5,-120)：参考 224/256，精确面积
     *   234.3/256），连精确面积的实现都要偏 3 级，所以 ±2 对任何实现都不可能通过；
     *   把参考加密到 16x16 也不解决（实测仍偏 1/3/3/3/3 级）。
     * - 叠加 6bit 绿通道的量化（1 级 = 4.05/256）后，判别力仍充足：实测本内核与参考
     *   的最大偏差是 4 级（r=20/60/120），留 1.5 倍余量。
     * - 结构性错误仍然必被抓住：被推翻的单轴弦模型偏差 ~115/256 ≈ 28 级。 */
    const int tol = 6;
    static const int radii[] = { 3, 8, 20, 60, 120 };
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(radii) / sizeof(radii[0]); i++) {
        int r = radii[i];
        int cx = 130, cy = 131;
        eui_canvas_clear(c);
        eui_canvas_set_color(c, eui_color_from_gray(255));
        eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 0, 360);
        for (int y = cy - r - 2; y <= cy + r + 2; y++)
            for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                uint32_t ref = aa_ref_disc_cov(x, y, cx, cy, r);
                int want = (int)((ref * 63u + 127u) / 255u);
                int got  = (int)(((uint32_t)aa_get(x, y) >> 5) & 0x3Fu);
                int d    = got - want;
                if (d < -tol || d > tol) {
                    printf("\n  r=%d (%d,%d) got=%d want=%d ref=%u\n", r, x - cx, y - cy, got, want, ref);
                    FAIL("16bpp 与参考偏差超容差");
                }
            }
    }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

#if EUI_COLOR_DEPTH == 8
static void test_contracts_c2_c3_c4_c6_c7(void)
{
    TEST("C2/C3/C4/C6/C7（8bpp 覆盖度可观测）");
    const int r = 60, cx = 130, cy = 130;
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    /* C2：非黑底上全覆盖像素必须精确等于 fg（证明未二次混合、未读 dst） */
    eui_canvas_set_bg_color(c, (eui_color_t)128);
    eui_canvas_clear(c);
    eui_canvas_set_color(c, (eui_color_t)255);
    eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 0, 360);
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++) {
            int dx = x - cx, dy = y - cy;
            /* 像素 (x,y) 覆盖相对圆心的 [dx, dx+1] × [dy, dy+1]，不是一个点：
             * C2 的"明确在内部"要看像素的**最远点**、C3 的"明确在外"要看**最近点**
             * （负偏移侧在 +1 方向）。把 (dx,dy) 当点用会让 C2 要求本就有部分覆盖度
             * 的像素等于 fg（r=60 的 (57,15)：中心 59.55 px > r-0.5，真值也是部分覆盖），
             * 让 C3 把本就有部分覆盖度的像素当成"确定没覆盖"（r=60 的 (-11,-60)：
             * 最近角 59.84 px < r，圆确实切进这个像素）。 */
            int fx = (dx >= 0) ? dx + 1 : dx;      /* 最远点 */
            int fy = (dy >= 0) ? dy + 1 : dy;
            int nx = (dx >= 0) ? dx : dx + 1;      /* 最近点 */
            int ny = (dy >= 0) ? dy : dy + 1;
            if (fx * fx + fy * fy <= (r - 1) * (r - 1)) {
                if (aa_get(x, y) != 255) FAIL("C2：内部像素不等于 fg");
            }
            if (nx * nx + ny * ny >= (r + 1) * (r + 1)) {
                if (aa_get(x, y) != 128) FAIL("C3：外部像素被写");
            }
        }
    /* C4：过心水平扫描线上存在严格介于背景与前景之间的像素
     * （本测试底色是 128，所以"部分覆盖"必须是 128 < v < 255；写 v>0 会把背景当部分覆盖 → 假通过）。
     * 半径不能太大：圆心与半径都是整数时，圆在水平方向恰好与像素栅格相切，最外圈像素
     * 切掉的小角面积只有 ≈ 1/(6r) px²——r=60 时 0.3%，128 底 + 8bpp 舍入后饱和成 255，
     * 与硬边不可分（r=60 的圆在过心行上根本没有中间值像素）。r=8 时切掉 2%，
     * 最外圈落到 253，判别性成立，因此这里用小半径单独探一次。 */
    {
        const int r4 = 8;
        eui_canvas_set_bg_color(c, (eui_color_t)128);
        eui_canvas_clear(c);
        eui_canvas_set_color(c, (eui_color_t)255);
        eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r4, 0, 0, 360);
        int found = 0;
        for (int x = cx - r4 - 1; x <= cx + r4 + 1; x++) {
            uint32_t v = aa_get(x, cy);
            if (v > 128 && v < 255) found = 1;
        }
        if (!found) FAIL("C4：水平扫描线上没有部分覆盖像素（假 AA）");
        /* 复原后面 C6/C7 要用的 r=60 底图 */
        eui_canvas_clear(c);
        eui_canvas_set_color(c, (eui_color_t)255);
        eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 0, 360);
    }
    /* C6：圆顶行的"肩部"有部分覆盖（单轴弦模型在此会给满格或 0） */
    {
        int partial = 0;
        for (int x = cx - r; x <= cx + r; x++) {
            uint32_t v = aa_get(x, cy - r);
            if (v > 128 && v < 255) partial = 1;
        }
        if (!partial) FAIL("C6：圆顶行没有部分覆盖像素（圆顶被削平）");
    }
    /* C7：沿半径向右（自左边界向内），覆盖度单调不减。
     * 原稿的循环从圆心往外走却仍要求"不减"，与实际值的方向相反，必然假红。 */
    {
        uint32_t prev = 0;
        for (int x = cx - r - 1; x <= cx; x++) {
            uint32_t v = aa_get(x, cy);
            if (v < prev) FAIL("C7：覆盖度沿半径非单调");
            prev = v;
        }
    }
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

static void test_clip_boundary(void)   /* C11：全色深可跑 */
{
    TEST("C11：跨裁剪边界时不越界、边界像素与真实 dst 混合（无黑边）");
    const int r = 24;   /* 20 < r < 20√2 ≈ 28.3：圆要伸出裁剪块的四条边（r > 20），
                         * 又不能把 40x40 块整个包住（半径大于块角距离 28.28 时，
                         * 块内全是内部像素，下面的"有混合像素"必然假红——r=30 正是如此）。 */
    const eui_color_t bright = eui_color_from_gray(255);
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    /* 先铺一层亮底，再裁剪出一块区域，让圆心落在裁剪块内、圆伸出块外。
     * 注意不能拿 EUI_COLOR_WHITE 当"亮"：8bpp 下它是 1（近黑）。 */
    eui_canvas_set_bg_color(c, bright);
    eui_canvas_clear(c);
    eui_rect_t clip = { 100, 100, 40, 40 };
    eui_canvas_set_clip(c, &clip);
    eui_canvas_set_color(c, EUI_COLOR_BLACK);
    /* Task 3 的 AA 路径就是内核本身；fill_circle 要到 Task 4 才改走内核，
     * 用 fill_circle 测的是旧硬阈值路径（无混合像素，必然假红）。 */
    eui_canvas_aa_arc(c, 120, 120, (uint16_t)r, 0, 0, 360);
    eui_canvas_clear_clip(c);
    /* 裁剪块外必须原封不动 */
    for (int y = 0; y < AA_H; y++)
        for (int x = 0; x < AA_W; x++) {
            int inside = (x >= 100 && x < 140 && y >= 100 && y < 140);
            if (!inside && aa_get(x, y) != (uint32_t)bright) {
                printf("\n  (%d,%d) v=%u\n", x, y, (unsigned)aa_get(x, y));
                FAIL("裁剪块外被写入");
            }
        }
#if EUI_COLOR_DEPTH == 8 || EUI_COLOR_DEPTH == 16
    /* 裁剪块内的边界像素应与亮 dst 混合，而不是与黑混合（否则会出现黑边）。
     * 只在能表示中间值的色深上检查：1/2bpp 的中间值靠抖动，不存在"中间像素值"。 */
    {
        int found_blend = 0;
        for (int y = 100; y < 140; y++)
            for (int x = 100; x < 140; x++) {
                uint32_t v = aa_get(x, y);
                if (v != 0 && v != (uint32_t)bright) found_blend = 1;
            }
        if (!found_blend) FAIL("裁剪块内没有混合像素（疑似按 clip 外颜色混合）");
    }
#endif
    eui_canvas_set_clip(c, &(eui_rect_t){ 0, 0, AA_W, AA_H });
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_mirror_symmetry(void)   /* C5：全色深可跑 */
{
    TEST("C5：四向镜像对称 + sweep=0/360 与环的边界语义");
    const int r = 21, cx = 130, cy = 130;
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 0, 360);
#if EUI_COLOR_DEPTH >= 4
    /* 1/2bpp 的混合带 4x4 Bayer 抖动，而 C10 要求抖动相位锚在屏幕坐标
     * (x, y + page_y_offset)；镜像像素 (2*cx - x - 1) 的相位与 x 不同，
     * 于是"逐像素一致"在抖动色深上不可能成立（4bpp 只有 round-to-nearest，无抖动，
     * 因此 >= 4bpp 成立）。 */
    for (int y = cy - r - 1; y <= cy + r + 1; y++)
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            uint32_t v = aa_get(x, y);
            if (v != aa_get(2 * cx - x - 1, y) ||      /* 镜像索引：2*cx - x - 1 */
                v != aa_get(x, 2 * cy - y - 1)) {
                printf("\n  (%d,%d) v=%u\n", x - cx, y - cy, v);
                FAIL("镜像不对称");
            }
        }
#endif
    /* sweep == 0 不画 */
    eui_canvas_clear(c);
    eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, 45, 45);
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++)
            if (aa_get(x, y) != 0) FAIL("sweep=0 仍然画了像素");
    /* r_in >= r_out 不画 */
    eui_canvas_clear(c);
    eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (uint16_t)r, 0, 360);
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++)
            if (aa_get(x, y) != 0) FAIL("r_in >= r_out 仍然画了像素");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_round_rect_edges_geometry(void)   /* C1：全色深 */
{
    TEST("C1：圆角矩形直边几何与夹取行为");
    const int16_t x = 10, y = 10; const uint16_t w = 70, h = 24, r = 6;
    /* l = x+r = 16, t = y+r = 16, ri = x+w-r-1 = 73, b = y+h-r-1 = 27 */
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    eui_canvas_draw_round_rect(c, x, y, w, h, r);
    for (int i = 16; i <= 73; i++) {
        if (aa_get(i, 10) == 0) FAIL("上直边缺失");
        if (aa_get(i, 33) == 0) FAIL("下直边缺失");
    }
    for (int i = 16; i <= 27; i++) {
        if (aa_get(10, i) == 0) FAIL("左直边缺失");
        if (aa_get(79, i) == 0) FAIL("右直边缺失");
    }
    /* 直边之外不应有像素（角落圆心在 (16,16)/(73,16)/(16,27)/(73,27)）。
     * 注意紧邻角起点的 (15,10)/(74,10) 是**角弧自己的**像素：(15,10) 的像素中心距
     * 角心 √(0.5²+5.5²) ≈ 5.5 < r+0.5 = 6.5，角弧必然画它；旧中点圆光栅器同样把
     * (15,10) 画成实心，所以原稿在此断言 ==0 在改动**前**的树上就已经是红的
     * （把"角弧最外圈像素"误当成"直边越界"）。角弧只触及中心距角心 ≤ 6.5 px 的像素，
     * 故退到 (11,10)/(78,10)（中心距 ≈ 7.1 / 7.8）再断言：直边若误画成从 x 到 x+w-1，
     * 这两处必然被点亮，判别力不减。 */
    if (aa_get(11, 10) != 0) FAIL("上直边越过左角起点");
    if (aa_get(78, 10) != 0) FAIL("上直边越过右角起点");
    /* 半径夹取：w < 2r 时 r 收敛为 w/2，不越界 */
    eui_canvas_clear(c);
    eui_canvas_draw_round_rect(c, 10, 10, 5, 5, 9);
    if (aa_get(10, 10) == 0 && aa_get(14, 14) == 0) FAIL("夹取后完全没有绘制");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

static void test_corner_matches_quarter_arc(void)   /* C5 后半 */
{
    TEST("C5：圆角与对应四分之一圆逐像素一致");
    const int cx = 100, cy = 100, r = 9;
    /* 关键：让圆角矩形的角心 (x+r, y+r) 正好落在 (cx, cy)，
     * 即 x = cx - r、y = cy - r；若取 cx-r-1 则角心差一像素，比对必然失败。 */
    const int16_t x = (int16_t)(cx - r), y = (int16_t)(cy - r);
    uint32_t corner_px[16 * 16];
    int n = 0, cap = (int)(sizeof(corner_px) / sizeof(corner_px[0]));
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;

    /* 矩形取 2r+4 见方而不是 2r：w == 2r 会让直边退化成 2 px（ri == l-1、b == t-1），
     * 于是 (a) 上/左直边的端点像素落进比较象限（dx=-1/dy=-1），且 (b) 相邻角的圆心
     * 只差 1 px，其角弧也能覆盖到该象限——实测两者共造成 4 处差异
     * （8bpp：(−1,−9)=255/252、(−9,−1)=255/252、(−1,−8)=8/4、(−8,−1)=8/4），
     * 与"C5 后半"要断言的角弧几何无关。w = h = 2r+4 时四角圆心相距 4 px，
     * 直边退到象限之外，"该区域内只有角弧贡献"才真正成立。 */
    eui_canvas_draw_round_rect(c, x, y, (uint16_t)(2 * r + 4), (uint16_t)(2 * r + 4), (uint16_t)r);
    /* 只比"严格外侧象限"(dx<0 且 dy<0)：该区域内只有角弧贡献。
     * dx==0 / dy==0 那两行/列上有直边的端点像素，裸弧不会画成同样的值。 */
    for (int dy = -r - 1; dy <= -1; dy++)
        for (int dx = -r - 1; dx <= -1; dx++) {
            if (n >= cap) FAIL("测试缓冲不足");
            corner_px[n++] = aa_get(cx + dx, cy + dy);
        }
    eui_canvas_clear(c);
    eui_canvas_aa_arc(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (uint16_t)(r - 1), 180, 270);
    n = 0;
    for (int dy = -r - 1; dy <= -1; dy++)
        for (int dx = -r - 1; dx <= -1; dx++)
            if (aa_get(cx + dx, cy + dy) != corner_px[n++])
                FAIL("圆角与四分之一圆不一致");
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

#if EUI_COLOR_DEPTH == 8
static void test_seam_single_blend(void)          /* C8 */
{
    TEST("C8：跨 180° 无接缝（对比参考 32/256 + 差分单次混合）");
    /* 为什么用参考比对而不是手挑坐标：端帽线是"过圆心、沿该端半径方向"的直线，
     * 手算它穿过哪些像素很容易算反。把大弧拆成两段绘制会让接缝像素叠加两次：
     * 接缝像素在径向上位于圆的内部（单次混合 = 255），两次混合得 0.25·dst + 0.75·fg
     * = 191，偏差 255 − 191 = 64/256（实测 64，见下面的差分断言），远超 32/256。
     * sweep=180 与 181 分别走两条不同公式。
     *
     * 容差 32/256 的两个边界（都是实测值）：
     *  - 本底（参考比对的下限）：内核的径向覆盖度是"线性距离"近似，与 8x8 子样本
     *    真实面积的最大偏差为 11..21/256（r = 40..90）；本例 r = 50 → 16/256，
     *    出现在 (-38,33)（got=56 / want=40），是径向边缘、不是端帽；
     *  - 信号（要捕捉的接缝）：57..68/256，实测 64。
     *  32 = 本底上限 21 的 1.5x，同时是信号 64 的 1/2，两侧都留有余量。
     *
     * 为什么起点取 20° 而不是 0°：0°/90°/180°/270° 的端帽线正好压在像素边界上
     * （dy256 = ±128 → s_cap_side 判为整体在内/在外，不产生部分覆盖像素），此时把
     * sweep 沿 90° 对半拆成两段绘制与单次绘制**逐位相同**（实测：a0=0 时拆 90° 与
     * 单次调用 0 个像素不同），测试对它要抓的 bug 完全失明。起点取 20° 后端帽线
     * 切过像素（同一次拆分实测 65 个像素不同、最大 64），参考比对才有判别力。 */
    static const int sweeps[] = { 180, 181 };
    const int r = 50, cx = 130, cy = 130, a0 = 20;
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(sweeps) / sizeof(sweeps[0]); i++) {
        int sw = sweeps[i];
        eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
        eui_canvas_clear(c);
        eui_canvas_set_color(c, eui_color_from_gray(255));
        eui_canvas_fill_pie(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (int16_t)a0,
                            (int16_t)(a0 + sw));
        for (int y = cy - r - 2; y <= cy + r + 2; y++)
            for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                uint32_t want = aa_ref_sector_cov(x, y, cx, cy, 0, r, a0, a0 + sw);
                uint32_t got  = aa_get(x, y);
                uint32_t diff = (got > want) ? (got - want) : (want - got);
                if (diff > 32) {
                    printf("\n  sweep=%d (%d,%d) got=%u want=%u\n", sw, x - cx, y - cy, got, want);
                    FAIL("端帽附近偏差超容差（疑似接缝双重混合）");
                }
            }
    }
    /* 差分断言（"一次调用内至多混合一次"，不依赖参考光栅器）：同一个扇形沿
     * mid-sweep 拆成两个相邻子扇区分两次调用画，与单次调用逐像素比对。拆线取
     * 20 + 180/2 = 110°（切过像素），实测最大偏差 64/256（单次 255 / 拆段 191，
     * 即两次部分混合的接缝信号）。
     * 断言是**下界**而不是"相等/小于容差"：单次调用若**也**拆成两段画（C8 要抓的
     * bug），它在同一条拆线上的取值会与拆段绘制一致，偏差塌到 ~0（实测 a0=0 沿
     * 像素边界 90° 拆时正是 0）。所以"偏差 ≥ 40"才是这条性质的判别式，一旦内核
     * 改回拆段绘制立刻变红；上界 96 只是量级哨兵（理论极值 64 + 径向残差 21 ≈ 85）。 */
    {
        const int sw = 180, split = a0 + sw / 2;
        static uint32_t one[AA_W * AA_H];
        int n = 0;
        eui_canvas_clear(c);
        eui_canvas_fill_pie(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (int16_t)a0,
                            (int16_t)(a0 + sw));
        for (int y = 0; y < AA_H; y++)
            for (int x = 0; x < AA_W; x++) one[n++] = aa_get(x, y);
        eui_canvas_clear(c);
        eui_canvas_fill_pie(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (int16_t)a0,
                            (int16_t)split);
        eui_canvas_fill_pie(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, (int16_t)split,
                            (int16_t)(a0 + sw));
        uint32_t worst = 0;
        int wd = 0, wx = 0, wy = 0;
        n = 0;
        for (int y = 0; y < AA_H; y++)
            for (int x = 0; x < AA_W; x++) {
                uint32_t s = aa_get(x, y), o = one[n++];
                uint32_t diff = (s > o) ? (s - o) : (o - s);
                if (diff) wd++;
                if (diff > worst) { worst = diff; wx = x - cx; wy = y - cy; }
            }
        if (worst < 40) {
            printf("\n  单次 vs 拆段 maxdiff=%u n=%d at (%d,%d)\n", worst, wd, wx, wy);
            FAIL("单次绘制与拆段绘制几乎相同（疑似内核内部拆段双重混合）");
        }
        if (worst > 96) {
            printf("\n  单次 vs 拆段 maxdiff=%u n=%d at (%d,%d)\n", worst, wd, wx, wy);
            FAIL("拆段接缝偏差超出量级上界");
        }
    }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

static void test_equivalence_and_extremes(void)   /* C9 + C14 */
{
    TEST("C9/C14：等价关系（含带宽映射与 sweep 规则）与极端输入");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    /* fill_pie(0,360) == fill_circle */
    eui_canvas_clear(c);
    eui_canvas_fill_pie(c, 60, 60, 25, 0, 360);
    uint32_t a[64 * 64]; int n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++) a[n++] = aa_get(x, y);
    eui_canvas_clear(c);
    eui_canvas_fill_circle(c, 60, 60, 25);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++)
        if (aa_get(x, y) != a[n++]) FAIL("fill_pie(0,360) 与 fill_circle 不一致");
    /* draw_ring(r,0,0,360) == fill_circle */
    eui_canvas_clear(c);
    eui_canvas_draw_ring(c, 60, 60, 25, 0, 0, 360);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++) a[n++] = aa_get(x, y);
    eui_canvas_clear(c);
    eui_canvas_fill_circle(c, 60, 60, 25);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++)
        if (aa_get(x, y) != a[n++]) FAIL("draw_ring(r,0,0,360) 与 fill_circle 不一致");
    /* 带宽映射 [r - thickness, r] 在**一般** thickness 上也必须成立：上面两条只压
     * 住两个退化端（thickness == r → r_in=0、r_inner == 0），r_in 差 1 也能全过。
     * draw_arc(20,5) 与 draw_ring(20,15) 必须逐像素相同。 */
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 5, 0, 360);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++) a[n++] = aa_get(x, y);
    eui_canvas_clear(c);
    eui_canvas_draw_ring(c, 60, 60, 20, 15, 0, 360);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++)
        if (aa_get(x, y) != a[n++]) FAIL("draw_arc(20,5) 与 draw_ring(20,15) 不一致");
    /* 头文件公开的两条 sweep 规则：delta == 0 不画、|delta| >= 360 整圆。
     * 取 start == end（而不是非 360 的 sweep）与 delta = 400（而不是恰好 360）。 */
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 20, 90, 90);          /* delta == 0 → 空 */
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++)
        if (aa_get(x, y) != 0) FAIL("delta == 0 仍然画了像素");
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 20, 0, 400);          /* |delta| > 360 → 整圆 */
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++) a[n++] = aa_get(x, y);
    eui_canvas_clear(c);
    eui_canvas_fill_circle(c, 60, 60, 20);
    n = 0;
    for (int y = 40; y < 80; y++) for (int x = 40; x < 80; x++)
        if (aa_get(x, y) != a[n++]) FAIL("delta > 360 与整圆不一致");
    /* 极端输入：不得崩溃，且明确空的条件必须什么都不画。
     * 原稿把这条"应为空"的断言放在下面那条 thickness>=r（退化为扇形，**非空**）
     * 之后，断言窗口 [36,84)² 与扇形覆盖的 [60,80]² 相交（例如 (70,66) 的像素中心
     * 在圆内、角度 31.7° ∈ [0°,90°]），因此原稿在正确实现下也必红。这里只把该断言
     * 上移到最后一个"非空"绘制之前，断言文本与窗口逐字不变。 */
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 0, 0, 90);            /* thickness=0 → 空 */
    eui_canvas_draw_ring(c, 60, 60, 20, 20, 0, 90);          /* r_inner>=r_outer → 空 */
    eui_canvas_draw_ring(c, 60, 60, 20, 30, 0, 90);          /* 同上 */
    eui_canvas_fill_pie(c, 60, 60, 0, 0, 360);               /* r=0 → 空 */
    for (int y = 36; y < 84; y++) for (int x = 36; x < 84; x++)
        if (aa_get(x, y) != 0) FAIL("极端输入在应为空时画了像素");
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 999, 0, 90);          /* thickness>=r → 扇形，非空 */
    if (aa_get(60, 66) == 0) FAIL("thickness>=r 未退化为扇形");
    /* r=1/2、sweep=1/359 不得崩溃 */
    eui_canvas_draw_arc(c, 100, 100, 1, 1, 0, 1);
    eui_canvas_draw_arc(c, 100, 100, 2, 2, 0, 359);
    eui_canvas_fill_pie(c, 100, 100, 2, -90, 90);
    eui_canvas_draw_ring(c, 100, 100, 2, 1, -170, 170);
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

/* 画布上非背景像素数：用**各自真正画到的像素**做分母。
 * 圆只覆盖 πr² ≈ 0.785·(2r)²，用包围盒归一化会把圆的单像素成本低估约 27%。 */
static long aa_count_ink(void)
{
    long n = 0;
    for (int y = 0; y < AA_H; y++)
        for (int x = 0; x < AA_W; x++)
            if (aa_get(x, y) != 0) n++;
    return n;
}

static void test_cost_ratio(void)                 /* C13：只打印，不断言绝对阈值 */
{
    TEST("C13：成本比值（AA fill_circle vs 等包围盒 fill_rect，各按自身覆盖像素归一化）");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    const int r = 120, N = 20;

    /* 先量两个形状各自真正覆盖的像素数（背景为 0；dither 掉到 0 的边界像素不算覆盖） */
    eui_canvas_clear(c);
    eui_canvas_fill_circle(c, 130, 130, (uint16_t)r);
    long aa_px = aa_count_ink();
    eui_canvas_clear(c);
    eui_canvas_fill_rect(c, 10, 10, (uint16_t)(2 * r), (uint16_t)(2 * r));
    long rc_px = aa_count_ink();
    eui_canvas_clear(c);

    clock_t t0 = clock();
    for (int i = 0; i < N; i++) eui_canvas_fill_circle(c, 130, 130, (uint16_t)r);
    clock_t t1 = clock();
    for (int i = 0; i < N; i++) eui_canvas_fill_rect(c, 10, 10, (uint16_t)(2 * r), (uint16_t)(2 * r));
    clock_t t2 = clock();

    double aa_call = (double)(t1 - t0) * 1e9 / (CLOCKS_PER_SEC * (double)N);  /* ns/次 */
    double rc_call = (double)(t2 - t1) * 1e9 / (CLOCKS_PER_SEC * (double)N);
    double aa_ns = (aa_px > 0) ? aa_call / (double)aa_px : 0.0;
    double rc_ns = (rc_px > 0) ? rc_call / (double)rc_px : 0.0;
    printf("AA fill_circle r=%d: %.0f ns/次, 覆盖 %ld px -> %.2f ns/px/自身像素\n",
           r, aa_call, aa_px, aa_ns);
    printf("fill_rect %dx%d（同包围盒）: %.0f ns/次, 覆盖 %ld px -> %.2f ns/px/自身像素\n",
           2 * r, 2 * r, rc_call, rc_px, rc_ns);
    printf("比值（各按自身覆盖像素）%.2fx；按包围盒比 %.2fx\n",
           (rc_ns > 0) ? aa_ns / rc_ns : 0.0,
           (rc_call > 0) ? aa_call / rc_call : 0.0);
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}

/* ---- PAGE 模式的条带边界 ----------------------------------------------------
 * PAGE 模式画布只拥有一个 width x 8 的条带缓冲（buf_height = 8），但
 * eui_canvas_height() 返回的是**显示**高度。若不把纵向边界收到 buf_height，落在
 * 当前 band 之外的行会按 y * screen_w 寻址，写到条带缓冲之外（AA 路径在 <16bpp
 * 下还要读回 dst，于是越界读一并发生）。 */
#define AA_PG_W 64
#define AA_PG_H 40                                            /* 5 个 band */
#define AA_PG_BAND 8
#define AA_PG_BAND_BYTES ((size_t)AA_PG_W * EUI_COLOR_DEPTH)  /* W * 8 * bpp / 8 */
#define AA_PG_POOL 16384
static uint8_t aa_pg_pool[AA_PG_POOL];

static eui_display_drv_t aa_page_display = {
    .caps = { .width = AA_PG_W, .height = AA_PG_H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_PAGE, .has_gram = false },
    .init = NULL,
    .write_buffer = aa_mock_write,
};

/* 同尺寸的 FULL 模式画布：用来证明 band 内的行画得**对**（不只是"没越界"） */
static eui_display_drv_t aa_page_ref_display = {
    .caps = { .width = AA_PG_W, .height = AA_PG_H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = aa_mock_write,
};

/* 放在 main() 最后：它把全局分配器换成一个独立的 16KB 池，这样"条带缓冲之后的
 * 内存"一定落在池内、可安全当作只读 canary（我们只比对前后，不写它），而且
 * 即便 FAIL() 提前返回也不会污染后面测试用的池。 */
static void test_page_band_overflow(void)
{
    TEST("PAGE：画布只有 width x 8 的条带，band 之外的行被丢弃");
    eui_allocator_init_tlsf(aa_pg_pool, AA_PG_POOL);
    eui_canvas_t *pc = eui_canvas_create(&aa_page_display);
    if (!pc) FAIL("PAGE 画布创建失败");
    if (pc->buf_width != AA_PG_W || pc->buf_height != AA_PG_BAND)
        FAIL("PAGE 画布的缓冲尺寸应为 width x 8");

    /* canary 紧跟在条带缓冲之后：band 之外的行若被写出，这里必然变化 */
    uint8_t *band = pc->buffer;
    uint8_t *canary = band + AA_PG_BAND_BYTES;
    if (canary + 16 > aa_pg_pool + AA_PG_POOL)
        FAIL("池布局不允许 canary 检查（条带缓冲之后不在池内）");

    eui_canvas_set_bg_color(pc, EUI_COLOR_BLACK);
    eui_canvas_clear(pc);
    /* 用"该色深的最亮级"：8bpp 下 EUI_COLOR_WHITE 是 1（近黑） */
    eui_canvas_set_color(pc, eui_color_from_gray(255));

    /* 参照：同一形状画在 FULL 画布上（64x40 与 PAGE 画布的前 8 行应逐像素相同） */
    eui_canvas_t *fc = eui_canvas_create(&aa_page_ref_display);
    if (!fc) FAIL("FULL 参照画布创建失败");
    eui_canvas_set_bg_color(fc, EUI_COLOR_BLACK);
    eui_canvas_clear(fc);
    eui_canvas_set_color(fc, eui_color_from_gray(255));
    eui_canvas_fill_circle(fc, 32, 6, 10);
    static uint32_t ref_band[AA_PG_W * AA_PG_BAND];
    int n = 0;
    for (int y = 0; y < AA_PG_BAND; y++)
        for (int x = 0; x < AA_PG_W; x++)
            ref_band[n++] = (uint32_t)eui_canvas_px_get(fc, (int16_t)x, (int16_t)y);

    uint8_t canary0[16];
    memcpy(canary0, canary, sizeof canary0);

    /* 圆心 y=6、r=10：圆跨 band 0（行 -4..15 中的 0..7）、band 1（8..15）与 band 2 */
    aa_cur = pc;
    eui_canvas_fill_circle(pc, 32, 6, 10);

    /* (a) band 内画到的内容与 FULL 参照的前 8 行逐像素相同 */
    n = 0;
    for (int y = 0; y < AA_PG_BAND; y++)
        for (int x = 0; x < AA_PG_W; x++)
            if (aa_get(x, y) != ref_band[n++]) {
                printf("\n  (%d,%d) got=%u want=%u\n", x, y, aa_get(x, y),
                       (unsigned)ref_band[n - 1]);
                FAIL("band 内的行与 FULL 参照不一致");
            }
    long ink = 0;
    for (int y = 0; y < AA_PG_BAND; y++)
        for (int x = 0; x < AA_PG_W; x++) if (aa_get(x, y)) ink++;
    if (ink == 0) FAIL("band 内没有任何墨点（跨 band 的图形完全没画出来）");
    /* (b) 条带缓冲之后的 canary 一个字节都没变（"只写到当前 band 缓冲之内"的直接证据） */
    if (memcmp(canary0, canary, sizeof canary0) != 0)
        FAIL("条带缓冲之后的内存被写入（band 之外的写没有被丢弃）");
    /* (c) band 之外的行读回 0：修复前这两处读的是自己越界写进去的值 */
    if (aa_get(32, 8) != 0 || aa_get(32, 10) != 0 || aa_get(32, 15) != 0)
        FAIL("band 之外的行仍可读回内容（越界读写未封堵）");

    /* 完全落在 band 之外的圆：band 缓冲与 canary 都必须逐字节不变 */
    static uint8_t band_copy[AA_PG_BAND_BYTES];
    memcpy(band_copy, band, AA_PG_BAND_BYTES);
    memcpy(canary0, canary, sizeof canary0);
    eui_canvas_fill_circle(pc, 32, 24, 8);            /* 行 16..32，全在 band 0 之外 */
    if (memcmp(band_copy, band, AA_PG_BAND_BYTES) != 0)
        FAIL("band 之外的绘制改动了当前 band 的内容");
    if (memcmp(canary0, canary, sizeof canary0) != 0)
        FAIL("band 之外的绘制越过了条带缓冲");

    aa_cur = NULL;
    eui_canvas_destroy(fc);
    eui_canvas_destroy(pc);
    PASS();
}

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

#if EUI_COLOR_DEPTH == 8
    test_reference_agreement_disc();
    test_reference_agreement_sector();
#endif

#if EUI_COLOR_DEPTH == 16
    test_reference_agreement_disc_16();
#endif

#if EUI_COLOR_DEPTH == 8
    test_contracts_c2_c3_c4_c6_c7();
    test_seam_single_blend();
#endif

    test_clip_boundary();
    test_mirror_symmetry();

    test_round_rect_edges_geometry();
    test_corner_matches_quarter_arc();
    test_equivalence_and_extremes();

    test_cost_ratio();

    /* 最后跑：它把全局分配器换成自己的小池 */
    test_page_band_overflow();

    return eui_test_summary();
}
