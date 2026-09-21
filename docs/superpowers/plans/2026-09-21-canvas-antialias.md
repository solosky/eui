# 画布抗锯齿（AA）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让 eui 画布的圆、圆角矩形、圆弧（含圆环/扇形）输出抗锯齿像素，低色深用有序抖动模拟灰度，且全部为整数运算、无 libm。

**Architecture:** 把光栅化从 `src/eui_canvas.c` 拆到新的 `src/eui_canvas_aa.c`：底层是"覆盖度混合内核"（按色深量化/抖动）与"扫描线几何"（每行两个 isqrt 定位全覆盖区，只有边界带逐像素算到圆心的径向距离），上层把所有曲线形状统一归结到一个泛化内核 `eui_canvas_aa_arc(r_out, r_in, a0, a1)`。四个既有原语原地升级为 AA（签名不变），另加 `draw_arc` / `draw_ring` / `fill_pie` 三个新入口。

**Tech Stack:** C99（无编译器扩展）、CMake 3.22、assert 风格 CTest、TLSF 分配器。

**Spec:** `docs/superpowers/specs/2026-09-21-canvas-antialias-design.md`（含契约 C1–C15 与被实测推翻的两个数值方案的证据）

## Global Constraints

- **C99，无编译器扩展。** 不加 `//` 以外的 C11 特性（`//` 注释在 C99 合法）。
- **AA 内核纯整数**：不 include `math.h`/任何 `mc_*.h`，不使用浮点与随机，`MC_USE_FLOAT` 开启与否输出逐位相同（spec 契约 C12）。
- **像素单位**：几何量一律用 `xx256 = 值 << 8`（1/256 px）。内部覆盖度用 `cov256 ∈ [0,256]`，跨函数边界（`eui_canvas_px_blend`）折算成 `0..255`。
- **命名**：公开符号 `eui_` 前缀；内部跨文件函数同样用 `eui_canvas_` 前缀并声明在 `include/eui/eui_canvas_internal.h`（对齐既有 `eui_font_internal.h` 约定，测试需要能直接驱动内核）。
- **内存**：不新增堆分配。测试夹具的 TLSF 池由测试文件自己初始化（见各任务的测试代码）。
- **测试风格**：`common/eui_test.h` 的 `TEST()` / `PASS()` / `FAIL()`；`test/CMakeLists.txt` 里用 `eui_add_test(名字 canvas/文件名.c)` 注册。
- **每个任务必须跑 5 个色深**（1/2/4/8/16）的 canvas 测试，并把命令与结果写进提交信息或任务报告。
- **每个任务结束必须 `git commit`**；分支 `feat/canvas-antialias`。

## 改动前基线（实测，务必先读）

在基线提交（`main` + 本 spec 文档，无任何 AA 代码）上，`ctest` 的既有结果是：

| 色深 | `test_canvas_render` | `test_font_real_u8g2_render` | 256x750 画布需要 / 测试池 |
|---|---|---|---|
| 1 | pass | pass | 24000 B / 65536 B |
| 2 | pass | **SEGFAULT** | 48000 B / 65536 B |
| 4 | **FAIL** | **SEGFAULT** | 96000 B / 65536 B |
| 8 | **FAIL** | pass | 192000 B / 65536 B |
| 16 | **FAIL** | **SEGFAULT** | 384000 B / 65536 B |

两个都是**既有缺陷、与本次 AA 无关**，但会污染每个任务"跑 5 个色深"的验收口径，且让 CI 的 8bpp/16bpp job 在 `main` 上就是红的。它们在 Task 7 一并修掉——这也是"把 CI 矩阵扩到 5 个色深"能成立的前提：

1. `test/canvas/test_canvas_render.c` 用 `common/eui_test.h` 的 65536 B 池去建 256x750 全缓冲画布（16bpp 需 384 KB），`eui_canvas_create` 返回 NULL 后 `return 1`。修法：像 `test_canvas_16bpp.c` 那样自建池。
2. `test/font/test_font_real_u8g2_render.c` 的 `write_bmp()` 在 `#else`（即 2/4/16bpp）分支把 `img_buf` 当 `uint16_t[]` 按 `IMG_W*IMG_H` 个元素读，而 `BUF_SIZE` 只按每像素 1 字节分配（320000 B）→ 越界读约一倍 → 段错误。1bpp/8bpp 各有专门分支，所以只有 2/4/16 崩。

**各任务验收口径**：AA 相关测试必须全绿；上表里的既有红只要不恶化即算通过；Task 7 修完后才要求 5 个色深全绿。

## 文件结构

| 文件 | 职责 |
|---|---|
| `include/eui/eui_canvas_internal.h`（新建） | 内部共享声明：像素访问（`px_set`/`px_get`/`px_blend`）、整数开方（`eui_canvas_isqrt`）、泛化内核（`eui_canvas_aa_arc`） |
| `src/eui_canvas.c`（改） | 像素服务层（`px_set`/`px_get`/`px_blend` 及几何无关的原语：点/线/矩形）；四个曲线原语**移出**到 AA 文件 |
| `src/eui_canvas_aa.c`（新建） | `eui_canvas_isqrt`、Q14 正弦表、扫描线几何、七个公开曲线原语 |
| `test/canvas/test_canvas_aa.c`（新建） | 契约 C1–C15 的像素级测试 + 成本打印 |
| `test/canvas/test_canvas_1bpp.c`（新建） | 1bpp 抖动目检画廊（BMP） |
| `test/canvas/test_canvas_16bpp.c` / `test_canvas_2bpp.c`（改） | 扩充 AA 目检样本 |
| `.github/workflows/build.yml`（改） | 色深矩阵扩到 5 个 |

---

### Task 1: 内部像素访问层 + 整数开方

**Files:**
- Create: `include/eui/eui_canvas_internal.h`
- Create: `src/eui_canvas_aa.c`
- Modify: `src/eui_canvas.c`（`canvas_set_pixel`@33 → `eui_canvas_px_set`；`canvas_get_pixel`@74 → `eui_canvas_px_get` 并扩到全色深）
- Modify: `src/CMakeLists.txt:7` 之后加入新源文件
- Create: `test/canvas/test_canvas_aa.c`
- Modify: `test/CMakeLists.txt:31` 之后注册新测试

**Interfaces:**
- Produces: `void eui_canvas_px_set(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t color);`、`eui_color_t eui_canvas_px_get(eui_canvas_t *c, int16_t x, int16_t y);`、`uint32_t eui_canvas_isqrt(uint64_t x, uint8_t frac_bits);`

- [ ] **Step 1: 建内部头文件**

```c
/* include/eui/eui_canvas_internal.h */
#ifndef EUI_CANVAS_INTERNAL_H
#define EUI_CANVAS_INTERNAL_H

#include "eui/eui_canvas.h"

/* 光栅化器内部共享的像素访问，语义与公开绘图 API 一致：
 * 受 clip 与屏幕边界约束；越界写被丢弃，越界读返回 0。 */
void        eui_canvas_px_set(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t color);
eui_color_t eui_canvas_px_get(eui_canvas_t *c, int16_t x, int16_t y);

/* 逐位法整数开方：返回 floor(sqrt(x) * 2^frac_bits)。
 * 约束：x << (2 * frac_bits) 必须落在 uint64 内（frac_bits = 8 时 x < 2^48）。
 * 光栅器用法是 frac_bits = 0、radicand 以 (1/256 px)^2 为单位，
 * 于是返回值直接是 1/256 px 单位的长度。 */
uint32_t    eui_canvas_isqrt(uint64_t x, uint8_t frac_bits);

#endif /* EUI_CANVAS_INTERNAL_H */
```

- [ ] **Step 2: 把 eui_canvas.c 里的静态像素访问提升为内部 API**

```bash
cd <repo>/src
sed -i '' 's/\bcanvas_set_pixel(/eui_canvas_px_set(/g; s/\bcanvas_get_pixel(/eui_canvas_px_get(/g' eui_canvas.c
```
（Linux 下 `sed -i ''` 去掉两个单引号。）然后在 `eui_canvas.c` 顶部 `#include "eui/eui_canvas.h"` 之后加：

```c
#include "eui/eui_canvas_internal.h"
```

把两个定义行去掉 `static`，并把 `eui_canvas_px_get` 的 `#if EUI_COLOR_DEPTH != 16` 早退分支替换为全色深实现：

```c
static void eui_canvas_px_set(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t color)   /* ← 去掉 static */
...
eui_color_t eui_canvas_px_get(eui_canvas_t *c, int16_t x, int16_t y)                      /* ← 去掉 static，全色深 */
{
    if (!c || c->buffer == NULL) return 0;
    if (x < c->clip.x || x >= c->clip.x + (int16_t)c->clip.w ||
        y < c->clip.y || y >= c->clip.y + (int16_t)c->clip.h) {
        return 0;
    }

    uint16_t screen_h = eui_canvas_height(c);
    uint16_t screen_w = eui_canvas_width(c);

    if (x < 0 || x >= (int16_t)screen_w || y < 0 || y >= (int16_t)screen_h) return 0;

#if EUI_COLOR_DEPTH == 1
    uint16_t byte_idx = (uint16_t)(y * (int16_t)(screen_w / 8u)) + (uint16_t)(x / 8);
    return (eui_color_t)((c->buffer[byte_idx] >> (x % 8)) & 1u);
#elif EUI_COLOR_DEPTH == 2
    uint16_t byte_idx = (uint16_t)(y * (int16_t)(screen_w / 4u)) + (uint16_t)(x / 4u);
    uint8_t shift = (uint8_t)(6u - 2u * (uint8_t)(x % 4u));
    return (eui_color_t)((c->buffer[byte_idx] >> shift) & 3u);
#elif EUI_COLOR_DEPTH == 4
    uint16_t byte_idx = (uint16_t)(y * (int16_t)(screen_w / 2u)) + (uint16_t)(x / 2u);
    if (x & 1) return (eui_color_t)(c->buffer[byte_idx] & 0x0Fu);
    return (eui_color_t)(c->buffer[byte_idx] >> 4);
#elif EUI_COLOR_DEPTH == 8
    return (eui_color_t)c->buffer[y * screen_w + x];
#else
    uint16_t *buf16 = (uint16_t *)c->buffer;
    return (eui_color_t)buf16[y * screen_w + x];
#endif
}
```

注意：`px_set`/`px_get` 原实现用 `screen_w`（显示宽度）索引 buffer，`PAGE` 模式下同样成立（band 缓冲宽度 = 显示宽度），保持不动。

- [ ] **Step 3: 写失败的测试（isqrt + 像素读回）**

```c
/* test/canvas/test_canvas_aa.c */
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
    if (!c) FAIL("canvas create failed");
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    /* 用"该色深的最亮级"而不是 EUI_COLOR_WHITE：8bpp 下后者是 1（近黑） */
    eui_canvas_set_color(c, eui_color_from_gray(255));
    return c;
}

/* 取像素：统一走内部访问器，全色深同一写法 */
static uint32_t aa_get(int x, int y) { return (uint32_t)eui_canvas_px_get(aa_cur, x, y); }
static eui_canvas_t *aa_cur = NULL;   /* 供 aa_get 使用，由各测试在末尾复位 */

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
```

`test/CMakeLists.txt` 在 `eui_add_test(test_canvas_render ...)` 之后加：

```cmake
eui_add_test(test_canvas_aa     canvas/test_canvas_aa.c)
```

- [ ] **Step 4: 跑测试确认失败**

```bash
cmake -S . -B /tmp/aa16 -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_CROSS_EXAMPLES=OFF -DEUI_COLOR_DEPTH=16
cmake --build /tmp/aa16 -j8 --target test_canvas_aa
```
Expected: 链接失败 `undefined reference to eui_canvas_isqrt`（`eui_canvas_aa.c` 还不存在）。

- [ ] **Step 5: 实现 isqrt**

```c
/* src/eui_canvas_aa.c */
#include "eui/eui_canvas_internal.h"

/* 逐位法（digit-by-digit）整数开方，无除法、无浮点。
 * 先把 radicand 左移 2*frac_bits，再对 floor(sqrt()) 做整数开方。 */
uint32_t eui_canvas_isqrt(uint64_t x, uint8_t frac_bits)
{
    uint64_t rem  = x << (2u * frac_bits);
    uint64_t root = 0;
    uint64_t bit  = (uint64_t)1 << 62;

    while (bit > rem) bit >>= 2;
    while (bit != 0) {
        if (rem >= root + bit) {
            rem -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)root;
}
```

`src/CMakeLists.txt` 第 7 行 `eui_canvas.c` 之后加 `    eui_canvas_aa.c`。

- [ ] **Step 6: 跑测试确认通过（5 个色深）**

```bash
cmake --build /tmp/aa16 -j8 --target test_canvas_aa && (cd /tmp/aa16 && ctest -R canvas_aa --output-on-failure)
for d in 1 2 4 8; do
  cmake -S . -B /tmp/aa$d -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=OFF -DEUI_BUILD_CROSS_EXAMPLES=OFF -DEUI_COLOR_DEPTH=$d >/dev/null
  cmake --build /tmp/aa$d -j8 --target test_canvas_aa && (cd /tmp/aa$d && ctest -R canvas_aa --output-on-failure) || echo "FAIL depth=$d"
done
```
Expected: 3/3 passed（每个色深）。

- [ ] **Step 7: 提交**

```bash
git add include/eui/eui_canvas_internal.h src/eui_canvas_aa.c src/eui_canvas.c src/CMakeLists.txt test/canvas/test_canvas_aa.c test/CMakeLists.txt
git commit -m "feat(canvas): internal pixel access layer and integer sqrt for AA"
```

---

### Task 2: 覆盖度混合内核 + 4×4 Bayer 抖动

**Files:**
- Modify: `include/eui/eui_canvas_internal.h`（加 `eui_canvas_px_blend`）
- Modify: `src/eui_canvas.c`（`vlw_blend_pixel`@98 提升为共享混合；新增 `px_blend`；`draw_vlw_glyph` 16bpp 分支@657 改调共享内核）
- Modify: `test/canvas/test_canvas_aa.c`

**Interfaces:**
- Consumes: Task 1 的 `eui_canvas_px_set` / `eui_canvas_px_get`
- Produces: `void eui_canvas_px_blend(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t fg, uint8_t cov);`

- [ ] **Step 1: 写失败的测试**

在 `test_canvas_aa.c` 的 `main()` 之前插入：

```c
/* ---- 2bpp/1bpp 抖动相位与密度 ---- */

#if EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2
static const uint8_t aa_bayer_expected[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};

/* 在 (ox,oy) 起 4x4 区域用 cov 混合白色到黑底，数取"高级"的格数 */
static int dither_high_count(int ox, int oy, uint8_t cov, int high_level)
{
    int n = 0;
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++) {
            eui_canvas_px_set(aa_cur, ox + dx, oy + dy, 0);
            eui_canvas_px_blend(aa_cur, ox + dx, oy + dy, EUI_COLOR_WHITE, cov);
            if ((int)aa_get(ox + dx, oy + dy) == high_level) n++;
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
```

在 `main()` 中依次调用这些测试（按色深条件编译，函数体已各自用 `#if` 包住，直接调用即可）。

- [ ] **Step 2: 跑测试确认失败**

```bash
cmake --build /tmp/aa16 -j8 --target test_canvas_aa
```
Expected: 链接失败 `undefined reference to eui_canvas_px_blend`。

- [ ] **Step 3: 实现混合内核**

在 `include/eui/eui_canvas_internal.h` 的 `px_get` 之后加声明：

```c
/* 覆盖度混合：cov 0 → 不写；cov 255 → 直接写 fg；其余读 dst 后按色深混合。
 * 抖动路径的阈值相位锚在 (x, y + c->page_y_offset)，跨 PAGE band 连续。
 * cov 与 VLW 字形的 alpha 同量纲（0..255），16bpp 分支与旧公式逐位一致。 */
void        eui_canvas_px_blend(eui_canvas_t *c, int16_t x, int16_t y,
                               eui_color_t fg, uint8_t cov);
```

在 `src/eui_canvas.c` 中，把原 `vlw_blend_pixel`（16bpp 段）改名为 `blend_565` 并去掉 `static`（同文件内使用），在其后追加：

```c
/* 4x4 Bayer 有序抖动阈值表（与 VAMeter startup_view 的既有先例同表） */
static const uint8_t canvas_bayer4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};

#if EUI_COLOR_DEPTH != 16
/* 色深级别 ↔ 0..255 灰度：eui_color_from_gray 的量化就是这套网格 */
static uint8_t canvas_gray_of(eui_color_t v)
{
#if EUI_COLOR_DEPTH == 1
    return (uint8_t)(v ? 255u : 0u);
#elif EUI_COLOR_DEPTH == 2
    return (uint8_t)(v * 85u);
#elif EUI_COLOR_DEPTH == 4
    return (uint8_t)(v * 17u);
#else
    return (uint8_t)v;                     /* 8bpp */
#endif
}

/* 把 0..255 的精确灰度量化到本色深。L ∈ {2,4} 用 4x4 Bayer 抖动合成中间灰，
 * L ∈ {16,256} 直接四舍五入：rem == 0（颜色已在级网格上）时恒等，实心内部零噪点。 */
static eui_color_t canvas_quantize(eui_canvas_t *c, uint8_t g, int16_t x, int16_t y)
{
#if EUI_COLOR_DEPTH == 8
    (void)c; (void)x; (void)y;
    return (eui_color_t)g;
#elif EUI_COLOR_DEPTH == 4
    (void)c; (void)x; (void)y;
    return (eui_color_t)(((uint16_t)g * 15u + 127u) / 255u);
#else
    uint16_t y_abs = (uint16_t)((uint16_t)y + c->page_y_offset);
    uint8_t  t     = canvas_bayer4[y_abs & 3u][(uint16_t)x & 3u];
    uint16_t step  = (EUI_COLOR_DEPTH == 1) ? 255u : 85u;
    uint8_t  q     = (uint8_t)(g / step);
    uint8_t  rem   = (uint8_t)(g - q * step);
    return (eui_color_t)((rem * 16u >= (uint16_t)(t + 1u) * step) ? (uint8_t)(q + 1u) : q);
#endif
}
#endif /* != 16bpp */

void eui_canvas_px_blend(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t fg, uint8_t cov)
{
    if (!c || cov == 0) return;
    if (cov == 255) {
        eui_canvas_px_set(c, x, y, fg);
        return;
    }
#if EUI_COLOR_DEPTH == 16
    eui_canvas_px_set(c, x, y, blend_565(eui_canvas_px_get(c, x, y), fg, cov));
#else
    uint16_t g = ((uint16_t)canvas_gray_of(fg) * cov
                + (uint16_t)canvas_gray_of(eui_canvas_px_get(c, x, y)) * (255u - cov)
                + 127u) / 255u;
    eui_canvas_px_set(c, x, y, canvas_quantize(c, (uint8_t)g, x, y));
#endif
}
```

并把 `draw_vlw_glyph` 的 16bpp 分支（`src/eui_canvas.c:657` 附近）改为：

```c
#if EUI_COLOR_DEPTH == 16
            if (a >= 250) {
                eui_canvas_px_set(canvas, px, py, canvas->fg_color);
            } else {
                eui_canvas_px_blend(canvas, px, py, canvas->fg_color, a);
            }
#else
            if (a >= 128)
                eui_canvas_px_set(canvas, px, py, canvas->fg_color);
#endif
```

（低色深字形路径**故意不动**：改它会破坏 `test/font/test_font_vlw_render.c:133` 的"恰好 28 个墨点"断言，属 spec §7 的非目标。）

- [ ] **Step 4: 跑测试确认通过（5 个色深）**

同 Task 1 Step 6 的命令，另跑一遍全部 canvas 与 font 测试确认 VLW 重构无回归：

```bash
(cd /tmp/aa16 && ctest -R "canvas|font" --output-on-failure)
```
Expected: `test_canvas_aa` 全绿；`test_16bpp_blend_matches_legacy` 通过（证明重构逐位等价）；`test_canvas_render` 与 `test_font_real_u8g2_render` 仍是基线红（见计划开头基线表，Task 7 修）。

- [ ] **Step 5: 提交**

```bash
git add include/eui/eui_canvas_internal.h src/eui_canvas.c test/canvas/test_canvas_aa.c
git commit -m "feat(canvas): coverage blend kernel with ordered dithering for 1/2bpp"
```

---

### Task 3: 泛化圆弧内核 `eui_canvas_aa_arc`

**Files:**
- Modify: `include/eui/eui_canvas_internal.h`
- Modify: `src/eui_canvas_aa.c`
- Modify: `test/canvas/test_canvas_aa.c`

**Interfaces:**
- Consumes: `eui_canvas_px_set` / `eui_canvas_px_blend`（Task 2）、`eui_canvas_isqrt`（Task 1）
- Produces: `void eui_canvas_aa_arc(eui_canvas_t *c, int16_t cx, int16_t cy, uint16_t r_out, uint16_t r_in, int16_t start_deg, int16_t end_deg);`
  语义：填充半径区间 `[r_in, r_out]` 与角度区间 `[start_deg, end_deg]`（0° = 3 点钟、顺时针、整数度）的交集；`r_out == 0` 或 `r_in >= r_out` 不画；`sweep == 0` 不画；`sweep == 360` 为整圆盘/整圆环。

- [ ] **Step 1: 声明内核**

在 `eui_canvas_internal.h` 末尾（`#endif` 之前）加：

```c
/* 泛化圆弧光栅化内核，供圆/圆角矩形/圆弧/圆环/扇形共用。
 * 坐标系与公开 API 一致；角度为整数度，0° = 3 点钟方向，顺时针（屏幕 y 向下）。 */
void eui_canvas_aa_arc(eui_canvas_t *c, int16_t cx, int16_t cy,
                       uint16_t r_out, uint16_t r_in,
                       int16_t start_deg, int16_t end_deg);
```

- [ ] **Step 2: 写失败的测试（C15 参考一致性 + C2/C3/C4/C5/C6/C7）**

在 `test_canvas_aa.c` 中加入（放在 depth 条件块之外，全部无条件编译）：

```c
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
    TEST("C15：与 64 子样本参考一致（16bpp 绿通道，容差 2 级）");
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
                if (d < -2 || d > 2) {
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
            /* 用整数距离平方判断"明确在内部"：d <= r - 1 */
            if (dx * dx + dy * dy <= (r - 1) * (r - 1)) {
                if (aa_get(x, y) != 255) FAIL("C2：内部像素不等于 fg");
            }
            if (dx * dx + dy * dy >= (r + 1) * (r + 1)) {
                if (aa_get(x, y) != 128) FAIL("C3：外部像素被写");
            }
        }
    /* C4：过心水平扫描线上存在严格介于背景与前景之间的像素
     * （本测试底色是 128，所以"部分覆盖"必须是 128 < v < 255；写 v>0 会把背景当部分覆盖 → 假通过） */
    {
        int found = 0;
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            uint32_t v = aa_get(x, cy);
            if (v > 128 && v < 255) found = 1;
        }
        if (!found) FAIL("C4：水平扫描线上没有部分覆盖像素（假 AA）");
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
    /* C7：沿半径向右，覆盖度单调不减 */
    {
        uint32_t prev = 0;
        for (int x = cx; x >= cx - r - 1; x--) {
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
    const int r = 30;
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
    eui_canvas_fill_circle(c, 120, 120, (uint16_t)r);
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
    for (int y = cy - r - 1; y <= cy + r + 1; y++)
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            uint32_t v = aa_get(x, y);
            if (v != aa_get(2 * cx - x - 1, y) ||      /* 镜像索引：2*cx - x - 1 */
                v != aa_get(x, 2 * cy - y - 1)) {
                printf("\n  (%d,%d) v=%u\n", x - cx, y - cy, v);
                FAIL("镜像不对称");
            }
        }
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
```

在 `main()` 中按顺序调用：`test_reference_agreement_disc()`（8bpp）、`test_reference_agreement_sector()`（8bpp）、`test_reference_agreement_disc_16()`（16bpp）、`test_contracts_c2_c3_c4_c6_c7()`（8bpp）、`test_clip_boundary()`、`test_mirror_symmetry()`。

- [ ] **Step 3: 跑测试确认失败**

```bash
cmake --build /tmp/aa8 -j8 --target test_canvas_aa || true
(cd /tmp/aa8 && ctest -R canvas_aa --output-on-failure) 2>&1 | tail -20
```
Expected: 链接失败（`eui_canvas_aa_arc` 未定义）。

- [ ] **Step 4: 实现内核**

在 `src/eui_canvas_aa.c` 追加（`eui_canvas_isqrt` 之后）：

```c
/* sin(0..90°) * 16384（Q14）。cos(θ) = sin(θ + 90°)，因此只需这一张表。 */
static const int16_t s_sin_q14[91] = {
        0,   286,   572,   857,  1143,  1428,  1713,  1997,  2280,  2563,
     2845,  3126,  3406,  3686,  3964,  4240,  4516,  4790,  5063,  5334,
     5604,  5872,  6138,  6402,  6664,  6924,  7182,  7438,  7692,  7943,
     8192,  8438,  8682,  8923,  9162,  9397,  9630,  9860, 10087, 10311,
    10531, 10749, 10963, 11174, 11381, 11585, 11786, 11982, 12176, 12365,
    12551, 12733, 12911, 13085, 13255, 13421, 13583, 13741, 13894, 14044,
    14189, 14330, 14466, 14598, 14726, 14849, 14968, 15082, 15191, 15296,
    15396, 15491, 15582, 15668, 15749, 15826, 15897, 15964, 16026, 16083,
    16135, 16182, 16225, 16262, 16294, 16322, 16344, 16362, 16374, 16382,
    16384,
};

static int32_t s_sin_q14(int16_t deg)
{
    int32_t d = deg % 360;
    if (d < 0) d += 360;
    if (d <= 90)  return  s_sin_q14[d];
    if (d <= 180) return  s_sin_q14[180 - d];
    if (d <= 270) return -s_sin_q14[d - 180];
    return -s_sin_q14[360 - d];
}

static int32_t s_cos_q14(int16_t deg) { return s_sin_q14((int16_t)(deg + 90)); }

/* 端帽半平面：判定像素相对某条端帽线的位置。
 * 返回 1 = 深在扇形内、-1 = 深在扇形外、0 = 边界带（需精确算覆盖度）。
 * 快路径依据 |cross| >= 8192·d256 ⟺ cross² >= 8192²·d2，用平方形式免除开方与除法。
 * inside_is_positive：起点帽内侧 ⟺ cross >= 0，终点帽内侧 ⟺ cross <= 0。 */
static int s_cap_side(int64_t cross, uint64_t d2, int inside_is_positive)
{
    if ((uint64_t)(cross * cross) < 67108864ULL * d2) return 0;   /* 8192^2 */
    if (inside_is_positive) return (cross >= 0) ? 1 : -1;
    return (cross <= 0) ? 1 : -1;
}

void eui_canvas_aa_arc(eui_canvas_t *c, int16_t cx, int16_t cy,
                       uint16_t r_out, uint16_t r_in,
                       int16_t start_deg, int16_t end_deg)
{
    if (!c || r_out == 0 || r_in >= r_out) return;

    /* sweep 归一化：delta 为端点差（可负，int16 极差 ±65535）。
     * delta == 0 → 不画；|delta| >= 360 → 整圆；否则折到 1..359。 */
    int32_t delta = (int32_t)end_deg - (int32_t)start_deg;
    if (delta == 0) return;
    int32_t sweep;
    if (delta >= 360 || delta <= -360) {
        sweep = 360;
    } else {
        sweep = delta % 360;
        if (sweep < 0) sweep += 360;          /* 例如 90 → 89 即 359° 的补向 */
    }

    /* 端帽：sweep <= 180 直接用扇形自身的两条边界；
     * sweep > 180 改用补扇形（≤180）并取覆盖率补，保证单次混合、无接缝。 */
    int complement = (sweep > 180 && sweep < 360);
    int16_t cap_a = 0, cap_b = 0;
    if (sweep < 360) {
        if (complement) { cap_a = end_deg;   cap_b = start_deg; }
        else            { cap_a = start_deg; cap_b = end_deg;   }
    }

    int32_t cx256 = (int32_t)cx << 8;
    int32_t cy256 = (int32_t)cy << 8;
    int32_t ro256 = (int32_t)r_out << 8;
    int32_t ri256 = (int32_t)r_in << 8;

    /* 径向阈值（平方域，避免逐像素开方） */
    int64_t in_out  = (int64_t)(ro256 - 128) * (ro256 - 128);   /* d <= r_out - 0.5 → 全覆盖 */
    int64_t ex_out  = (int64_t)(ro256 + 128) * (ro256 + 128);   /* d >  r_out + 0.5 → 不写 */
    int64_t in_in   = (ri256 > 0) ? (int64_t)(ri256 + 128) * (ri256 + 128) : 0;
    int64_t ex_in   = (ri256 > 0) ? (int64_t)(ri256 - 128) * (ri256 - 128) : -1;

    int32_t ux_a = 0, uy_a = 0, ux_b = 0, uy_b = 0;
    if (sweep < 360) {
        ux_a = s_cos_q14(cap_a); uy_a = s_sin_q14(cap_a);
        ux_b = s_cos_q14(cap_b); uy_b = s_sin_q14(cap_b);
    }

    int32_t cy_lo = (int32_t)cy - (int32_t)r_out - 1;
    int32_t cy_hi = (int32_t)cy + (int32_t)r_out + 1;
    int32_t clip_x0 = c->clip.x, clip_x1 = (int32_t)c->clip.x + c->clip.w;
    int32_t clip_y0 = c->clip.y, clip_y1 = (int32_t)c->clip.y + c->clip.h;

    for (int32_t y = cy_lo; y <= cy_hi; y++) {
        if (y < clip_y0 || y >= clip_y1) continue;
        int32_t dy256 = (y << 8) + 128 - cy256;
        int64_t dy2   = (int64_t)dy256 * dy256;
        int64_t b     = ex_out - dy2;                  /* 有覆盖半宽的平方 */
        if (b <= 0) continue;
        int32_t xout = (int32_t)eui_canvas_isqrt((uint64_t)b, 0);

        int32_t xs = (int32_t)cx - xout, xe = (int32_t)cx + xout;
        if (xs < clip_x0) xs = clip_x0;
        if (xe >= clip_x1) xe = clip_x1 - 1;

        for (int32_t x = xs; x <= xe; x++) {
            int32_t vx = ((x << 8) + 128) - cx256;      /* p - c 的 x 分量（有符号） */
            int64_t d2 = (int64_t)vx * vx + dy2;

            /* --- 径向覆盖度 --- */
            int32_t d256 = -1;
            int32_t rad;
            if (d2 >= in_in && d2 <= in_out) {
                rad = 256;                              /* 内外都全覆盖，不做距离测试 */
            } else if (d2 >= ex_out || d2 <= ex_in) {
                continue;                               /* 完全在外或完全在孔内 */
            } else {
                d256 = (int32_t)eui_canvas_isqrt((uint64_t)d2, 0);
                rad = 128 + ro256 - d256;
                if (rad <= 0) continue;
                if (rad > 256) rad = 256;
                if (ri256 > 0) {
                    int32_t inner = 128 + d256 - ri256;
                    if (inner <= 0) continue;
                    if (inner > 256) inner = 256;
                    rad = (rad * inner) >> 8;
                }
            }

            /* --- 角向覆盖度（整圆无端帽，跳过） --- */
            if (sweep < 360) {
                int64_t cr_a = (int64_t)ux_a * dy256 - (int64_t)uy_a * vx;
                int64_t cr_b = (int64_t)ux_b * dy256 - (int64_t)uy_b * vx;
                int fa = s_cap_side(cr_a, (uint64_t)d2, 1);
                int fb = s_cap_side(cr_b, (uint64_t)d2, 0);
                if (!complement && (fa < 0 || fb < 0)) continue;   /* 深在扇形外 */
                if (complement && (fa > 0 && fb > 0)) continue;    /* 深在补扇形内 → ang=0 */
                int32_t ang;
                if (fa > 0 && fb > 0) {
                    ang = 256;
                } else {
                    if (d256 <= 0) d256 = (int32_t)eui_canvas_isqrt((uint64_t)d2, 0);
                    if (d256 <= 0) continue;                       /* 理论不可达：中心点距圆心 >= 半像素 */
                    int32_t ca = 256, cb = 256;
                    if (fa == 0) {
                        int32_t perp = (int32_t)(-cr_a / (64 * (int64_t)d256));   /* 外侧为正 */
                        ca = 128 - perp;
                        if (ca <= 0) continue;
                        if (ca > 256) ca = 256;
                    }
                    if (fb == 0) {
                        int32_t perp = (int32_t)(cr_b / (64 * (int64_t)d256));
                        cb = 128 - perp;
                        if (cb <= 0) continue;
                        if (cb > 256) cb = 256;
                    }
                    ang = (ca * cb) >> 8;
                }
                if (complement) ang = 256 - ang;
                if (ang <= 0) continue;
                rad = (rad * ang) >> 8;
                if (rad <= 0) continue;
            }

            if (rad >= 256) {
                eui_canvas_px_set(c, (int16_t)x, (int16_t)y, c->fg_color);
            } else {
                eui_canvas_px_blend(c, (int16_t)x, (int16_t)y, c->fg_color,
                                    (uint8_t)((rad * 255 + 128) >> 8));
            }
        }
    }
}
```

实现要点（评审时逐条核对）：

- `s_cap_fast` 的阈值 `67108864 == 8192²`，配合 `d256² = d2` 得到 `|cross| >= 8192·d256 ⟺ 距离 >= 0.5 px`；用 `d2` 的平方形式避免开方。
- `perp = cross / (64·d256)`：`cross` 的单位是 Q14 × (1/256 px)，除以 `16384·d256` 得 px，再乘 256 得 1/256 px，合并即 `/(64·d256)`。
- 符号约定：起点帽内侧 ⟺ `cross(u, v) >= 0`；终点帽内侧 ⟺ `cross(u, v) <= 0`（推导见 spec §4.3）。
- `sweep > 180` 时用补扇形并把角向覆盖度取 `256 - ang`，**不拆两段绘制**（拆开会在接缝处叠加成 `0.25·dst + 0.75·fg`）。

- [ ] **Step 5: 跑测试确认通过（5 个色深）**

同 Task 1 Step 6 命令。Expected: 全绿，其中 8bpp 的 C15 逐像素偏差 ≤ 24/256。

- [ ] **Step 6: 提交**

```bash
git add include/eui/eui_canvas_internal.h src/eui_canvas_aa.c test/canvas/test_canvas_aa.c
git commit -m "feat(canvas): generalized analytic AA arc kernel (radial coverage + caps)"
```

---

### Task 4: 四个既有原语接入内核

**Files:**
- Modify: `src/eui_canvas.c`（删除 `draw_circle`@356、`fill_circle`@395、`draw_round_rect`@436、`fill_round_rect`@487 的实现）
- Modify: `src/eui_canvas_aa.c`（追加四个公开实现）
- Modify: `test/canvas/test_canvas_aa.c`

**Interfaces:**
- Consumes: `eui_canvas_aa_arc`（Task 3）、公开的 `eui_canvas_draw_line` / `eui_canvas_draw_rect` / `eui_canvas_fill_rect`（既有）
- Produces: 签名不变的四个公开原语（AA 语义），供 Task 5/6 与所有消费方使用

- [ ] **Step 1: 写失败的测试（C1 几何不变 + 圆角与四分之一圆一致）**

```c
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
    /* 直边之外不应有像素（角落圆心在 (16,16)/(73,16)/(16,27)/(73,27)） */
    if (aa_get(15, 10) != 0) FAIL("上直边越过左角起点");
    if (aa_get(74, 10) != 0) FAIL("上直边越过右角起点");
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

    eui_canvas_draw_round_rect(c, x, y, (uint16_t)(2 * r), (uint16_t)(2 * r), (uint16_t)r);
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
```

- [ ] **Step 2: 跑测试确认失败**

Expected: `C1` 通过旧实现也成立（直边本来就对），但 `corner_matches_quarter_arc` 失败——旧的中点圆角与 AA 四分之一圆不一致。（若 C1 在旧实现下也通过，说明它作为"几何不变"的回归网是对的，保留它。）

- [ ] **Step 3: 把四个实现搬到 AA 文件并接到内核**

从 `src/eui_canvas.c` 删除这四个函数体（保留 `eui_canvas_draw_line` / `draw_rect` / `fill_rect` / `draw_triangle`），在 `src/eui_canvas_aa.c` 追加：

```c
#include "eui/eui_canvas.h"        /* 公开原语：draw_line / fill_rect / draw_rect */
```
（`eui_canvas_aa.c` 顶部已有 `eui_canvas_internal.h`，其中已 include 公开头，无需重复。）

```c
void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r)
{
    if (!canvas) return;
    if (r == 0) { eui_canvas_px_set(canvas, x, y, canvas->fg_color); return; }
    eui_canvas_aa_arc(canvas, x, y, r, 0, 0, 360);
}

void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r)
{
    if (!canvas) return;
    if (r == 0) { eui_canvas_px_set(canvas, x, y, canvas->fg_color); return; }
    eui_canvas_aa_arc(canvas, x, y, r, (uint16_t)(r - 1), 0, 360);
}

/* 圆角半径夹取与旧实现一致：2r > w/h 时 r 收敛为 w/2、h/2 */
static uint16_t clamp_corner_radius(uint16_t w, uint16_t h, uint16_t r)
{
    if ((uint32_t)r * 2u > w) r = (uint16_t)(w / 2u);
    if ((uint32_t)r * 2u > h) r = (uint16_t)(h / 2u);
    return r;
}

void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                                uint16_t w, uint16_t h, uint16_t r)
{
    if (!canvas || w == 0 || h == 0) return;
    if (r == 0) { eui_canvas_draw_rect(canvas, x, y, w, h); return; }
    r = clamp_corner_radius(w, h, r);

    int16_t l  = (int16_t)(x + (int16_t)r);
    int16_t t  = (int16_t)(y + (int16_t)r);
    int16_t ri = (int16_t)(x + (int16_t)w - (int16_t)r - 1);
    int16_t b  = (int16_t)(y + (int16_t)h - (int16_t)r - 1);

    eui_canvas_draw_line(canvas, l, y, ri, y);
    eui_canvas_draw_line(canvas, l, (int16_t)(y + (int16_t)h - 1), ri, (int16_t)(y + (int16_t)h - 1));
    eui_canvas_draw_line(canvas, x, t, x, b);
    eui_canvas_draw_line(canvas, (int16_t)(x + (int16_t)w - 1), t, (int16_t)(x + (int16_t)w - 1), b);

    uint16_t rin = (uint16_t)(r - 1);
    eui_canvas_aa_arc(canvas, l,  t,  r, rin, 180, 270);   /* 左上 */
    eui_canvas_aa_arc(canvas, ri, t,  r, rin, 270, 360);   /* 右上 */
    eui_canvas_aa_arc(canvas, ri, b,  r, rin,   0,  90);   /* 右下 */
    eui_canvas_aa_arc(canvas, l,  b,  r, rin,  90, 180);   /* 左下 */
}

void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                               uint16_t w, uint16_t h, uint16_t r)
{
    if (!canvas || w == 0 || h == 0) return;
    if (r == 0) { eui_canvas_fill_rect(canvas, x, y, w, h); return; }
    r = clamp_corner_radius(w, h, r);

    int16_t l  = (int16_t)(x + (int16_t)r);
    int16_t t  = (int16_t)(y + (int16_t)r);
    int16_t ri = (int16_t)(x + (int16_t)w - (int16_t)r - 1);
    int16_t b  = (int16_t)(y + (int16_t)h - (int16_t)r - 1);

    eui_canvas_fill_rect(canvas, l, y, (uint16_t)(ri - l + 1), h);
    if (b > t) {
        eui_canvas_fill_rect(canvas, x, t, r, (uint16_t)(b - t + 1));
        eui_canvas_fill_rect(canvas, (int16_t)(ri + 1), t, r, (uint16_t)(b - t + 1));
    }
    /* 四个角用四分之一圆盘补齐（与中间矩形重叠处为全覆盖，px_set 幂等） */
    eui_canvas_aa_arc(canvas, l,  t,  r, 0, 180, 270);
    eui_canvas_aa_arc(canvas, ri, t,  r, 0, 270, 360);
    eui_canvas_aa_arc(canvas, ri, b,  r, 0,   0,  90);
    eui_canvas_aa_arc(canvas, l,  b,  r, 0,  90, 180);
}
```

- [ ] **Step 4: 跑测试确认通过（5 个色深 + 全量回归）**

```bash
for d in 1 2 4 8 16; do
  cmake --build /tmp/aa$d -j8 && (cd /tmp/aa$d && ctest --output-on-failure) || echo "FAIL depth=$d"
done
```
Expected: 除基线表里那两个既有红外全绿（`test_canvas_aa` 与全部既有 canvas/font/widget 测试不得新增失败）。VAMeter 侧留待最终验收（本仓不引用 VAMeter）。

- [ ] **Step 5: 提交**

```bash
git add src/eui_canvas.c src/eui_canvas_aa.c test/canvas/test_canvas_aa.c
git commit -m "feat(canvas)!: anti-aliased circle and rounded-rect primitives (in place)"
```

---

### Task 5: 三个新入口 draw_arc / draw_ring / fill_pie

**Files:**
- Modify: `include/eui/eui_canvas.h`（在 `eui_canvas_fill_round_rect` 声明之后加三个声明 + 文档注释）
- Modify: `src/eui_canvas_aa.c`
- Modify: `test/canvas/test_canvas_aa.c`

**Interfaces:**
- Consumes: `eui_canvas_aa_arc`（Task 3）
- Produces:
  - `void eui_canvas_draw_arc(eui_canvas_t *, int16_t cx, int16_t cy, uint16_t r, uint16_t thickness, int16_t start_deg, int16_t end_deg);`
  - `void eui_canvas_draw_ring(eui_canvas_t *, int16_t cx, int16_t cy, uint16_t r_outer, uint16_t r_inner, int16_t start_deg, int16_t end_deg);`
  - `void eui_canvas_fill_pie(eui_canvas_t *, int16_t cx, int16_t cy, uint16_t r, int16_t start_deg, int16_t end_deg);`

- [ ] **Step 1: 写失败的测试（C8 接缝 / C9 等价 / C14 极端输入）**

```c
#if EUI_COLOR_DEPTH == 8
static void test_seam_single_blend(void)          /* C8 */
{
    TEST("C8：跨 180° 无接缝（紧容差 16/256 比对参考）");
    /* 为什么用参考比对而不是手挑坐标：端帽线是"过圆心、沿该端半径方向"的直线，
     * 手算它穿过哪些像素很容易算反；而把大弧拆成两段绘制会让接缝像素叠加成
     * ~0.25·dst + 0.75·fg（8bpp 下 ~191 而非 ~128），偏差 ~63/256，
     * 相对 16/256 的紧容差必然暴露。sweep=180 与 181 分别走两条不同公式。 */
    static const int sweeps[] = { 180, 181 };
    const int r = 50, cx = 130, cy = 130;
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    for (unsigned i = 0; i < sizeof(sweeps) / sizeof(sweeps[0]); i++) {
        int sw = sweeps[i];
        eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
        eui_canvas_clear(c);
        eui_canvas_set_color(c, eui_color_from_gray(255));
        eui_canvas_fill_pie(c, (int16_t)cx, (int16_t)cy, (uint16_t)r, 0, (int16_t)sw);
        for (int y = cy - r - 2; y <= cy + r + 2; y++)
            for (int x = cx - r - 2; x <= cx + r + 2; x++) {
                uint32_t want = aa_ref_sector_cov(x, y, cx, cy, 0, r, 0, sw);
                uint32_t got  = aa_get(x, y);
                uint32_t diff = (got > want) ? (got - want) : (want - got);
                if (diff > 16) {
                    printf("\n  sweep=%d (%d,%d) got=%u want=%u\n", sw, x - cx, y - cy, got, want);
                    FAIL("端帽附近偏差超紧容差（疑似接缝双重混合）");
                }
            }
    }
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
#endif

static void test_equivalence_and_extremes(void)   /* C9 + C14 */
{
    TEST("C9/C14：等价关系与极端输入");
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
    /* 极端输入：不得崩溃，且明确空的条件必须什么都不画 */
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 0, 0, 90);            /* thickness=0 → 空 */
    eui_canvas_draw_ring(c, 60, 60, 20, 20, 0, 90);          /* r_inner>=r_outer → 空 */
    eui_canvas_draw_ring(c, 60, 60, 20, 30, 0, 90);          /* 同上 */
    eui_canvas_fill_pie(c, 60, 60, 0, 0, 360);               /* r=0 → 空 */
    eui_canvas_draw_arc(c, 60, 60, 20, 999, 0, 90);          /* thickness>=r → 扇形，非空 */
    for (int y = 36; y < 84; y++) for (int x = 36; x < 84; x++)
        if (aa_get(x, y) != 0) FAIL("极端输入在应为空时画了像素");
    eui_canvas_clear(c);
    eui_canvas_draw_arc(c, 60, 60, 20, 999, 0, 90);
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
```

- [ ] **Step 2: 跑测试确认失败**

Expected: 链接失败（三个新入口未定义）。

- [ ] **Step 3: 实现三个入口（并补公开头声明）**

`include/eui/eui_canvas.h` 在 `eui_canvas_fill_round_rect` 之后加（含文档注释：角度语义、thickness 语义、端帽平头、整数度粒度）：

```c
/**
 * @brief Draw an anti-aliased circular arc (annulus segment).
 *
 * The stroke spans radii [r - thickness, r]. Angles are integer degrees,
 * 0 deg = 3 o'clock, increasing clockwise (screen y grows downward).
 * thickness == 0 draws nothing; thickness >= r degenerates to a filled
 * sector (equivalent to eui_canvas_fill_pie). Caps are butt (flat).
 */
void eui_canvas_draw_arc(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                         uint16_t r, uint16_t thickness,
                         int16_t start_deg, int16_t end_deg);

/**
 * @brief Draw an anti-aliased circular ring segment between two radii.
 *
 * r_inner >= r_outer draws nothing. Angle semantics as in eui_canvas_draw_arc.
 */
void eui_canvas_draw_ring(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                          uint16_t r_outer, uint16_t r_inner,
                          int16_t start_deg, int16_t end_deg);

/**
 * @brief Draw a filled anti-aliased pie sector (r_inner == 0).
 *
 * Angle semantics as in eui_canvas_draw_arc.
 */
void eui_canvas_fill_pie(eui_canvas_t *canvas, int16_t cx, int16_t cy, uint16_t r,
                         int16_t start_deg, int16_t end_deg);
```

`src/eui_canvas_aa.c` 追加：

```c
void eui_canvas_draw_arc(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                         uint16_t r, uint16_t thickness,
                         int16_t start_deg, int16_t end_deg)
{
    if (!canvas || thickness == 0 || r == 0) return;
    uint16_t r_in = (thickness >= r) ? 0 : (uint16_t)(r - thickness);
    eui_canvas_aa_arc(canvas, cx, cy, r, r_in, start_deg, end_deg);
}

void eui_canvas_draw_ring(eui_canvas_t *canvas, int16_t cx, int16_t cy,
                          uint16_t r_outer, uint16_t r_inner,
                          int16_t start_deg, int16_t end_deg)
{
    if (!canvas || r_inner >= r_outer) return;
    eui_canvas_aa_arc(canvas, cx, cy, r_outer, r_inner, start_deg, end_deg);
}

void eui_canvas_fill_pie(eui_canvas_t *canvas, int16_t cx, int16_t cy, uint16_t r,
                         int16_t start_deg, int16_t end_deg)
{
    if (!canvas || r == 0) return;
    eui_canvas_aa_arc(canvas, cx, cy, r, 0, start_deg, end_deg);
}
```

- [ ] **Step 4: 跑测试确认通过（5 个色深）**

Expected: 全绿，特别是 8bpp 的 C8 端帽覆盖 ≤150。

- [ ] **Step 5: 提交**

```bash
git add include/eui/eui_canvas.h src/eui_canvas_aa.c test/canvas/test_canvas_aa.c
git commit -m "feat(canvas): add anti-aliased draw_arc / draw_ring / fill_pie"
```

---

### Task 6: BMP 画廊 + 成本记录

**Files:**
- Create: `test/canvas/test_canvas_1bpp.c`
- Modify: `test/canvas/test_canvas_16bpp.c`（画廊加 AA 样本）
- Modify: `test/canvas/test_canvas_2bpp.c`（画廊加 AA 样本）
- Modify: `test/CMakeLists.txt`（注册 1bpp）
- Modify: `test/canvas/test_canvas_aa.c`（成本打印）

**Interfaces:**
- Consumes: Task 5 的三个新入口
- Produces: 人工目检产物 `test_canvas_1bpp.bmp` / `test_canvas_16bpp.bmp` / `test_canvas_2bpp.bmp`

- [ ] **Step 1: 写 1bpp 灰度画廊（BMP 目检）**

```c
/* test/canvas/test_canvas_1bpp.c — 1bpp/2bpp 抖动目检画廊 */
#include "eui/eui_canvas.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define GAL_W 200
#define GAL_H 200
#define GAL_POOL 65536
static uint8_t gal_pool[GAL_POOL];
static uint8_t gal_buf[GAL_W * GAL_H * (EUI_COLOR_DEPTH == 16 ? 2 : 1)];

static void gal_write(const uint8_t *b, const eui_rect_t *r, void *ud)
{ (void)b; (void)r; (void)ud; }

static eui_display_drv_t gal_display = {
    .caps = { .width = GAL_W, .height = GAL_H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL, .write_buffer = gal_write,
};

static void put32(uint8_t *p, uint32_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

/* 单像素灰度画布 → 24 位 BMP（自下而上、BGR、4 字节行对齐） */
static void write_gray_bmp(const char *path, const uint8_t *gray, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) { printf("  (无法写出 %s)\n", path); return; }
    int row_bytes = (w * 3 + 3) & ~3;
    uint32_t data_size = (uint32_t)row_bytes * (uint32_t)h;
    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    put32(hdr + 2, 54u + data_size);
    put32(hdr + 10, 54u);
    put32(hdr + 14, 40u);
    put32(hdr + 18, (uint32_t)w);
    put32(hdr + 22, (uint32_t)h);
    hdr[26] = 1; hdr[28] = 24;
    put32(hdr + 34, data_size);
    fwrite(hdr, 1, 54, f);
    uint8_t row[GAL_W * 3 + 4];
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, (size_t)row_bytes);
        for (int x = 0; x < w; x++) {
            uint8_t g = gray[y * w + x];
            row[x * 3] = g; row[x * 3 + 1] = g; row[x * 3 + 2] = g;   /* BGR */
        }
        fwrite(row, 1, (size_t)row_bytes, f);
    }
    fclose(f);
}

/* 把 canvas 像素转成 0..255 灰度（低位深按级数放大） */
static uint8_t pixel_gray(eui_canvas_t *c, int x, int y)
{
    uint32_t v = (uint32_t)eui_canvas_px_get(c, x, y);
#if EUI_COLOR_DEPTH == 1
    return (uint8_t)(v ? 255u : 0u);
#elif EUI_COLOR_DEPTH == 2
    return (uint8_t)(v * 85u);
#elif EUI_COLOR_DEPTH == 4
    return (uint8_t)(v * 17u);
#else
    return (uint8_t)v;
#endif
}

int main(void)
{
    eui_allocator_init_tlsf(gal_pool, GAL_POOL);
    eui_canvas_t *c = eui_canvas_create(&gal_display);
    if (!c) { printf("FAIL: create\n"); return 1; }
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);

    /* 细弧（1px） / 粗环 / 扇形 / 极小半径 / 抖动斜坡 */
    eui_canvas_draw_arc(c, 50, 50, 40, 1, -90, 90);
    eui_canvas_draw_ring(c, 150, 50, 40, 30, 0, 270);
    eui_canvas_fill_pie(c, 50, 150, 40, 30, 210);
    eui_canvas_fill_circle(c, 155, 155, 2);
    eui_canvas_draw_circle(c, 170, 170, 5);
    for (int i = 0; i < 32; i++) {                 /* 覆盖度斜坡：目检抖动密度 */
        eui_canvas_draw_ring(c, 100, 100, 90, 80, (int16_t)(i * 11), (int16_t)(i * 11 + 8));
    }
    eui_canvas_commit(c);

    static uint8_t gray[GAL_W * GAL_H];
    for (int y = 0; y < GAL_H; y++)
        for (int x = 0; x < GAL_W; x++) gray[y * GAL_W + x] = pixel_gray(c, x, y);
    write_gray_bmp("test_canvas_1bpp.bmp", gray, GAL_W, GAL_H);
    printf("wrote test_canvas_1bpp.bmp\n");

    eui_canvas_destroy(c);
    return 0;
}
```

`test/CMakeLists.txt` 的 2bpp 块之后加：

```cmake
if(EUI_COLOR_DEPTH EQUAL 1 OR EUI_COLOR_DEPTH EQUAL 2)
    eui_add_test(test_canvas_1bpp   canvas/test_canvas_1bpp.c)
endif()
```

- [ ] **Step 2: 扩充 16bpp / 2bpp 画廊**

在 `test_canvas_16bpp.c` 的 `render_all_methods`（或等价的绘制序列）末尾加一段：

```c
    /* --- AA arc / ring / pie 目检样本 --- */
    eui_canvas_set_color(c, WHITE);
    eui_canvas_draw_arc(c,  40, y + 40, 35,  1, -90,  90);      /* 发丝弧 */
    eui_canvas_draw_arc(c, 120, y + 40, 35,  8,   0, 300);      /* 粗弧 */
    eui_canvas_draw_ring(c, 200, y + 40, 35, 22,   0, 270);     /* 圆环 */
    eui_canvas_fill_pie(c,  40, y + 130, 35, 30, 210);          /* 扇形 */
    eui_canvas_fill_circle(c, 120, y + 130, 2);                 /* 极小半径 */
    eui_canvas_draw_circle(c, 120, y + 130, 12);
    eui_canvas_fill_round_rect(c, 160, y + 100, 70, 24, 6);     /* 圆角矩形角部 */
    eui_canvas_draw_round_rect(c, 160, y + 130, 70, 24, 6);
    y += 175;
```

在 `test_canvas_2bpp.c` 加同样一组（用其已有的 `get_pixel_value` 断言 + 打印灰度图，便于目检抖动）。

- [ ] **Step 3: 加成本记录（C13）**

在 `test_canvas_aa.c` 加：

```c
static void test_cost_ratio(void)                 /* C13：只打印，不断言绝对阈值 */
{
    TEST("C13：成本比值（AA 填充 vs 同面积 fill_rect）");
    eui_canvas_t *c = aa_new_canvas();
    aa_cur = c;
    const int r = 120, N = 20;
    clock_t t0 = clock();
    for (int i = 0; i < N; i++) eui_canvas_fill_circle(c, 130, 130, (uint16_t)r);
    clock_t t1 = clock();
    for (int i = 0; i < N; i++) eui_canvas_fill_rect(c, 10, 10, (uint16_t)(2 * r), (uint16_t)(2 * r));
    clock_t t2 = clock();
    double px = (double)(2 * r) * (2 * r);
    double aa_ns = (double)(t1 - t0) * 1e9 / (CLOCKS_PER_SEC * (double)N) / px;
    double rc_ns = (double)(t2 - t1) * 1e9 / (CLOCKS_PER_SEC * (double)N) / px;
    printf("AA fill_circle r=%d: %.2f ns/px | fill_rect 同面积: %.2f ns/px | 比值 %.2fx\n",
           r, aa_ns, rc_ns, (rc_ns > 0) ? aa_ns / rc_ns : 0.0);
    eui_canvas_destroy(c);
    aa_cur = NULL;
    PASS();
}
```
`clock()` 需要 `#include <time.h>`。把该函数在 `main()` 里最后调用，并把输出抄进提交信息。

- [ ] **Step 4: 生成并目检 BMP**

```bash
for d in 1 2 16; do
  cmake --build /tmp/aa$d -j8 --target test_canvas_1bpp test_canvas_16bpp test_canvas_2bpp 2>/dev/null
  (cd /tmp/aa$d && ctest -R "canvas_1bpp|canvas_16bpp|canvas_2bpp" --output-on-failure)
done
ls -l /tmp/aa1/test_canvas_1bpp.bmp /tmp/aa2/test_canvas_1bpp.bmp
```
逐张打开 1bpp/2bpp 的 BMP，确认：圆顶无平口、边缘有渐变（抖动网点）、扇形端帽是平头且无接缝、极小半径是圆润的团块而非十字。
Expected: 目检通过；成本比值记录在案。

- [ ] **Step 5: 提交**

```bash
git add test/canvas/test_canvas_1bpp.c test/canvas/test_canvas_16bpp.c test/canvas/test_canvas_2bpp.c test/canvas/test_canvas_aa.c test/CMakeLists.txt
git commit -m "test(canvas): AA visual galleries (1/2/16bpp) and cost ratio print"
```

---

### Task 7: CI 色深矩阵 + 文档

**Files:**
- Modify: `.github/workflows/build.yml:11`
- Modify: `test/canvas/test_canvas_render.c`（修既有池缺陷）
- Modify: `test/font/test_font_real_u8g2_render.c`（修既有越界读）
- Modify: `docs/api_reference.md`（绘图原语段）
- Modify: `docs/eui_framework_design.md`（图形引擎段）

- [ ] **Step 1: 修既有基线缺陷（先让 5 个色深真有绿的机会）**

`test/canvas/test_canvas_render.c`：把 `eui_test_init()` 换成自建池（对齐 `test_canvas_16bpp.c` 的既有做法）：

```c
#define POOL_SIZE 524288
static uint8_t mem_pool[POOL_SIZE];
...
int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);   /* 取代 eui_test_init() */
```

`test/font/test_font_real_u8g2_render.c`：`img_buf` 全是 1bpp 位打包写入，`write_bmp()` 的 `#else` 分支把它当 `uint16_t[]` 读是错的。把该分支限到 16bpp 并按其真实字节数索引：

```c
#if EUI_COLOR_DEPTH == 1
            int idx = y * (IMG_W / 8) + x / 8;
            int bit = (img_buf[idx] >> (7 - (x % 8))) & 1;
#elif EUI_COLOR_DEPTH == 8
            int bit = img_buf[y * IMG_W + x] > 128 ? 1 : 0;
#elif EUI_COLOR_DEPTH == 16
            uint16_t *p16 = (uint16_t *)img_buf;
            int bit = p16[y * IMG_W + x] > 0 ? 1 : 0;
#else
            /* 2/4bpp：img_buf 是位打包布局，与 1bpp 同读法 */
            int idx = y * (IMG_W / 8) + x / 8;
            int bit = (img_buf[idx] >> (7 - (x % 8))) & 1;
#endif
```

并把 `BUF_SIZE` 的 16bpp 情形改为 `(IMG_W * IMG_H * 2)`：

```c
#if EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2 || EUI_COLOR_DEPTH == 4
#define BUF_SIZE (IMG_W * IMG_H / 8)
#elif EUI_COLOR_DEPTH == 8
#define BUF_SIZE (IMG_W * IMG_H)
#else
#define BUF_SIZE (IMG_W * IMG_H * 2)
#endif
```

跑一遍确认 5 个色深这两个测试都转绿（诊断依据见计划开头的基线表）。

- [ ] **Step 2: 扩 CI 矩阵**

```yaml
        color_depth: [1, 2, 4, 8, 16]
```

- [ ] **Step 3: 更新 API 参考**

在 `docs/api_reference.md` 的"绘图原语"代码块中 `eui_canvas_fill_round_rect` 之后加入三个新入口的声明，并在块后补一段说明：四个圆/圆角矩形原语自本版本起输出抗锯齿像素；1/2bpp 用 4×4 Bayer 有序抖动模拟灰度，实心内部不抖动；角度为整数度、0° = 3 点钟、顺时针；`draw_arc` 的 `thickness >= r` 退化为扇形。

- [ ] **Step 4: 更新框架设计文档**

在 `docs/eui_framework_design.md` 的图形引擎章节补一小节"抗锯齿"：覆盖度模型（边界带取到圆心的径向距离，内部 span 直写）、整数 isqrt + Q14 正弦表（无 libm、不依赖 motionc）、每色深量化策略表、以及"抖动相位锚在屏幕坐标（含 `page_y_offset`）"这一条。

- [ ] **Step 5: 提交并确认 CI**

```bash
git add .github/workflows/build.yml test/canvas/test_canvas_render.c test/font/test_font_real_u8g2_render.c docs/api_reference.md docs/eui_framework_design.md
git commit -m "ci(test): cover all five color depths, fix two pre-existing test fixture defects, document AA"
git push -u origin feat/canvas-antialias
```
Expected: 5 个色深 job 全绿（Step 1 修完两个既有缺陷后，5 个色深本地全量 ctest 也应为 20/20）。

---

## 收尾（不在任务内，供执行者参考）

- **VAMeter 验收**：eui 合入后升 `dependencies/eui` 子模块指针，跑 `app-eui` 全套 ctest（预期 16 pass + 1 环境失败的 `sim_smoke_exit`），逐个复核像素断言。原地升级的回归面已扫描过（断言探的是形状内部像素与 app 自身抖动格），但必须实测。
- **spec §7 的非目标**保持不变：不动 VLW 在 ≤4bpp 的阈值路径（会破坏 `test_font_vlw_render.c:133` 的 28 墨点断言）。
- **测量过但未采纳的方案**（记录以免重新走一遍）：Q16.16/`mc_fp_sqrt`（值域溢出 + 精度 0.449 px）、单轴弦覆盖度（圆顶 0.449 px 偏差）、把 >180° 弧拆两段绘制（接缝 `0.25·dst + 0.75·fg`）。

## Self-Review 记录

- **spec 覆盖**：C1→Task 4、C2/C3/C4/C6/C7→Task 3、C5→Task 3/4、C8/C9→Task 5、C10→Task 2（相位/单调/实心无噪点/4px 位置锁定）、C11→Task 3（`test_clip_boundary`：跨裁剪不越界 + 与真实 dst 混合）与 Task 1（`px_set`/`px_get` 的 clip 语义）、C12→全局约束（不 include `mc_*`/libm）、C13→Task 6、C14→Task 5、C15→Task 3。spec §6 的 BMP 画廊与 CI 矩阵 → Task 6/7。无遗漏。
- **命名一致性**：`eui_canvas_px_set` / `eui_canvas_px_get` / `eui_canvas_px_blend` / `eui_canvas_isqrt` / `eui_canvas_aa_arc` 在全部任务中同名同签名；`s_cap_side` 的三态返回（1/-1/0）在调用处一致。
- **写计划时自查推翻的四处**（已在正文修正）：
  1. `sweep` 归一化原稿在 `end - start == 360` 时会走成"带端帽的畸形扇区"，改为先判 `delta == 0` / `|delta| >= 360`。
  2. 原 `s_cap_fast` 把"贴近端帽（需精确算）"与"确定在扇形外"混成同一个返回值，改为三态 `s_cap_side`。
  3. 8bpp 下 `EUI_COLOR_WHITE` 是 1 而非 255，覆盖度恒等测试必须用 `eui_color_from_gray(255)`（并在测试里显式断言该值确实是 255）。
  4. 底色非黑时用 `v > 0 && v < 255` 判"部分覆盖"会把背景当部分覆盖 → 假通过；改为相对背景判断。
- **判别性 RED**：Task 3 的 C15 会拒绝单轴弦模型（115/256 > 24/256）；Task 5 的 C8 会拒绝分层绘制（>150 vs ~128）；Task 2 的密度单调性与 4px 位置锁定会拒绝硬阈值与图形锚定相位。
