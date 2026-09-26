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

/* ---- 泛化圆弧内核 ---------------------------------------------------------
 * 扫描线 + 解析覆盖度：每行用**一次**整数开方（外界阈值 b 的 isqrt）定出该行
 * "可能有覆盖"的像素区间，区间内逐像素只做平方域比较（全覆盖 / 完全在外），
 * 只有边界像素再算一次径向距离（开方）。端帽（角向）覆盖度不走径向距离，而是
 * 在端帽带上做"锥 ∩ 像素"的定点面积裁剪（见 s_cone_cov256）。
 * 角度为整数度，0° = 3 点钟、顺时针（屏幕 y 向下），与公开 API 一致。全整数、无浮点。 */

/* sin(0..90°) * 16384（Q14）。cos(θ) = sin(θ + 90°)，因此只需这一张表。
 * 表值即 round(sin(i°) * 16384)；角度参数是整数度，表在每个可接受角度上都精确，
 * 不做插值（方向向量误差只剩 Q14 量化：1/16384 ≈ 6e-5 rad）。 */
static const int16_t s_sin_tab[91] = {
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

/* 角度按 int32 传入：调用方的 end_deg 可达 ±32767，+90° 折象限时若先窄化到
 * int16 会绕回（例：32700° 折出 334° 而非 30°），这里直接取模避免。 */
static int32_t s_sin_q14(int32_t deg)
{
    int32_t d = deg % 360;
    if (d < 0) d += 360;
    if (d <= 90)  return  s_sin_tab[d];
    if (d <= 180) return  s_sin_tab[180 - d];
    if (d <= 270) return -s_sin_tab[d - 180];
    return -s_sin_tab[360 - d];
}

static int32_t s_cos_q14(int32_t deg) { return s_sin_q14(deg + 90); }

/* 端帽半平面：判定像素整体落在哪一侧。
 * 返回 1 = 整体在扇形内、-1 = 整体在扇形外、0 = 直线穿过像素（须精确算面积）。
 * thr = 128·(|ux| + |uy|) 是像素四角相对中心的 cross 偏移上界，取它作阈值才能保证
 * 四角同侧；若按"半像素"定死阈值，45° 朝向的端帽会把"角已越线"的像素误判成深在内侧。
 * inside_is_positive：起点帽内侧 ⟺ cross >= 0，终点帽内侧 ⟺ cross <= 0。 */
static int s_cap_side(int64_t cross, int64_t thr, int inside_is_positive)
{
    if (cross > -thr && cross < thr) return 0;
    if (inside_is_positive) return (cross >= 0) ? 1 : -1;
    return (cross <= 0) ? 1 : -1;
}

/* floor / ceil 整除（b != 0；C 的 / 向零截断，这里要数学上的向下/向上取整） */
static int64_t s_fdiv_floor(int64_t a, int64_t b)
{
    int64_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}
static int64_t s_fdiv_ceil(int64_t a, int64_t b) { return -s_fdiv_floor(-a, b); }

/* 32 位逐位开方：与 eui_canvas_isqrt(x, 0) 同算法同结果，但全程 32 位运算。
 * 64 位版本在 Xtensa 上每次开方要数百拍（64 位移位无桶形移位器），是圆角
 * 矩形角落扫描的主要开销；n < 2^32（r_out <= 255 时所有径向被开方数都在
 * 此范围内）时用它。 */
static uint32_t s_isqrt32(uint32_t n)
{
    uint32_t rem = n, root = 0, bit = 1u << 30;
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
    return root;
}

/* 被开方数落在 32 位域时走 32 位版本（结果逐位一致），否则回 64 位版本 */
static inline uint32_t s_isqrt_fit(int64_t x)
{
    return (x >= 0 && x <= 0xFFFFFFFFll) ? s_isqrt32((uint32_t)x)
                                         : (uint32_t)eui_canvas_isqrt((uint64_t)x, 0);
}

/* 轴对齐端帽（ux == 0 的水平端帽线）的"深内侧" x 区间：
 * cross(x) = ux*dy256 - uy*vx 在 ux==0 时 = uy*(cx256-128) - uy*256*x，与行 y 无关
 * （水平端帽线）、随 x 线性变化。sign：cap_a 内侧为 +1、cap_b 内侧为 -1，即
 * sign*cross >= 0 为内侧；深内侧 ⇔ sign*cross >= thr。
 * 返回深内侧区间 [*xlo, *xhi]（半无界用 INT32_MIN/MAX）；中心恰在端帽线上的
 * 跨缝像素不在区间内，仍由逐像素路径按锥面积精确算覆盖度（语义与慢路径一致）。 */
static void s_cap_deep_span_x(int32_t uy, int64_t thr, int sign,
                              int32_t cx256, int32_t *xlo, int32_t *xhi)
{
    int64_t a = (int64_t)uy * ((int64_t)cx256 - 128);  /* cross(x) = a - uy*256*x */
    int64_t t = (int64_t)sign * uy * 256;              /* sign*cross(x) = sign*a - t*x */
    int64_t rhs = (int64_t)sign * a - thr;             /* t*x <= rhs */
    if (t > 0) {
        *xlo = INT32_MIN;
        *xhi = (int32_t)s_fdiv_floor(rhs, t);
    } else {
        *xhi = INT32_MAX;
        *xlo = (int32_t)s_fdiv_ceil(rhs, t);
    }
}

/* 用半平面 value(P) >= 0（sign = 1）或 <= 0（sign = -1）裁剪凸多边形（原位）。
 * value(P) = base + ux*Ly - uy*Lx，base 已含 4 倍标度，与局部坐标同量纲。
 * 交点按 1/1024 px 取整，面积误差 < 1/1024 px^2（覆盖度 0.25/256，可忽略）。 */
static int s_clip_half(int32_t *vx, int32_t *vy, int n, int64_t base,
                       int32_t ux, int32_t uy, int sign)
{
    int32_t ox[8], oy[8];
    int m = 0;

    for (int i = 0; i < n; i++) {
        int j = (i + 1 == n) ? 0 : i + 1;
        int64_t fi = (base + (int64_t)ux * vy[i] - (int64_t)uy * vx[i]) * sign;
        int64_t fj = (base + (int64_t)ux * vy[j] - (int64_t)uy * vx[j]) * sign;

        if (fi >= 0) { ox[m] = vx[i]; oy[m] = vy[i]; m++; }
        if ((fi > 0 && fj < 0) || (fi < 0 && fj > 0)) {
            int64_t den = fi - fj;      /* 与 fi 同号；归一成正数后按 1/1024 px 四舍五入 */
            int64_t num = fi;
            if (den < 0) { den = -den; num = -num; }
            ox[m] = vx[i] + (int32_t)(((int64_t)(vx[j] - vx[i]) * num + den / 2) / den);
            oy[m] = vy[i] + (int32_t)(((int64_t)(vy[j] - vy[i]) * num + den / 2) / den);
            m++;
        }
    }
    for (int i = 0; i < m; i++) { vx[i] = ox[i]; vy[i] = oy[i]; }
    return m;
}

/* 凸锥 ∩ 像素方形 的精确面积 → 角向覆盖度 0..256。
 * 端帽边界是过圆心的两条直线，"两条端帽半平面覆盖度相乘"只有在两条线在像素
 * 尺度上彼此远离时才近似成立：锥顶落在像素内、或两线在像素内近乎重合
 * （sweep 接近 180°，或窄扇形对径一侧）时，两个"外侧"区域重叠，乘积会误判
 * 出半覆盖——实测偏差达 66/256，远超 C15 的 24/256 容差。这里直接算交集面积
 * （spec §4.3 所说的"面积精确解"），三种情形一次覆盖，无需分支。
 * 顶点定点到 1/1024 px（原点平移到像素左上角）以免大数乘除。 */
static int32_t s_cone_cov256(int32_t vx, int32_t dy,
                             int32_t ux_a, int32_t uy_a,
                             int32_t ux_b, int32_t uy_b)
{
    int32_t px[8], py[8];
    /* 左上角 (vx - 128, dy - 128) 处的 cross 乘 4；局部点 (Lx, Ly) 的 4·cross
     * 就是 base + ux*Ly - uy*Lx（局部 1024 单位 = 1 px = 4 · 256 单位）。 */
    int64_t base_a = 4 * ((int64_t)ux_a * (dy - 128) - (int64_t)uy_a * (vx - 128));
    int64_t base_b = 4 * ((int64_t)ux_b * (dy - 128) - (int64_t)uy_b * (vx - 128));

    px[0] = 0;    py[0] = 0;
    px[1] = 1024; py[1] = 0;
    px[2] = 1024; py[2] = 1024;
    px[3] = 0;    py[3] = 1024;

    int n = s_clip_half(px, py, 4, base_a, ux_a, uy_a, 1);    /* cross_a >= 0 */
    if (n == 0) return 0;
    n = s_clip_half(px, py, n, base_b, ux_b, uy_b, -1);       /* cross_b <= 0 */
    if (n == 0) return 0;

    int64_t s2 = 0;                       /* 鞋带公式 ×2（1/1024 px 单位的面积） */
    for (int i = 0; i < n; i++) {
        int j = (i + 1 == n) ? 0 : i + 1;
        s2 += (int64_t)px[i] * py[j] - (int64_t)px[j] * py[i];
    }
    if (s2 < 0) s2 = -s2;
    /* 像素面积 = 1024^2，覆盖率 256 级 → cov256 = 256 · (s2/2) / 1024^2 = s2 / 8192 */
    return (int32_t)((s2 + 4096) / 8192);
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

    /* 用乘法而不是 << 8：cx / cy / y / x 都是**有符号**量，圆心与 clip 原点都可以
     * 为负（fill_circle(c, -500, 40, 20)、set_clip({-10, 30, 40, 20})），而 C99 里
     * 负值左移是 UB（UBSan: "left shift of negative value"）。乘 256 在 int32 域内
     * 不可能溢出：|坐标| ≤ 32767 → |坐标 * 256| ≤ 8.4e6。 */
    int32_t cx256 = (int32_t)cx * 256;
    int32_t cy256 = (int32_t)cy * 256;
    int32_t ro256 = (int32_t)r_out << 8;
    int32_t ri256 = (int32_t)r_in << 8;

    /* 径向阈值（平方域，避免逐像素开方） */
    int64_t in_out  = (int64_t)(ro256 - 128) * (ro256 - 128);   /* d <= r_out - 0.5 → 全覆盖 */
    int64_t ex_out  = (int64_t)(ro256 + 128) * (ro256 + 128);   /* d >  r_out + 0.5 → 不写 */
    int64_t in_in   = (ri256 > 0) ? (int64_t)(ri256 + 128) * (ri256 + 128) : 0;
    int64_t ex_in   = (ri256 > 0) ? (int64_t)(ri256 - 128) * (ri256 - 128) : -1;

    int32_t ux_a = 0, uy_a = 0, ux_b = 0, uy_b = 0;
    int64_t thr_a = 0, thr_b = 0;
    if (sweep < 360) {
        ux_a = s_cos_q14(cap_a); uy_a = s_sin_q14(cap_a);
        ux_b = s_cos_q14(cap_b); uy_b = s_sin_q14(cap_b);
        thr_a = 128 * ((ux_a < 0 ? -ux_a : ux_a) + (uy_a < 0 ? -uy_a : uy_a));
        thr_b = 128 * ((ux_b < 0 ? -ux_b : ux_b) + (uy_b < 0 ? -uy_b : uy_b));
    }

    int32_t cy_lo = (int32_t)cy - (int32_t)r_out - 1;
    int32_t cy_hi = (int32_t)cy + (int32_t)r_out + 1;
    int32_t clip_x0 = c->clip.x, clip_x1 = (int32_t)c->clip.x + c->clip.w;
    int32_t clip_y0 = c->clip.y, clip_y1 = c->clip.y + c->clip.h;

    /* 快路径（launcher 圆角矩形/图标底板热点）：实心盘 + 端帽轴对齐（90° 整数倍，
     * 圆角矩形四角；sweep==360 即 fill_circle 无端帽）时：
     *   1) 行常量端帽（uy==0）整行深度判定 → 深外侧整行跳过（90° 扇形因此只扫
     *      四分之一 bbox，扫描量 4r² → ~r²）；
     *   2) 水平端帽线（ux==0）解出"深内侧" x 区间钳制直填段；
     *   3) 每行用一次 isqrt 求出径向全满半宽（d <= r-0.5px），与端帽区间求交后
     *      连续写 fg，只有边界环像素走逐像素覆盖度。
     * 直填段像素在慢路径下必满足 rad==256 → px_set(fg)，逐位等价；不满足前提
     * （一般角度端帽、圆环 r_in>0、补扇形）→ 完全走原逐像素路径。 */
    bool fast = (ri256 == 0 && r_out <= 255);   /* r<=255：径向被开方数均在 32 位域 */
    if (fast && sweep < 360) {
        fast = !complement &&
               (ux_a == 0 || uy_a == 0) && (ux_b == 0 || uy_b == 0);
    }

    /* 轴对齐端帽的"深内侧"x 区间与行 y 无关（cross 对 x 线性、系数全是常量），
     * 行循环外一次解出。s_cap_deep_span_x 里的 64 位除法在 Xtensa 上是软除法
     * 库调用（数百拍），绝不能落进行循环。 */
    int32_t xlo_a = INT32_MIN, xhi_a = INT32_MAX;
    int32_t xlo_b = INT32_MIN, xhi_b = INT32_MAX;
    if (fast && sweep < 360) {
        if (uy_a != 0)
            s_cap_deep_span_x(uy_a, thr_a, 1, cx256, &xlo_a, &xhi_a);
        if (uy_b != 0)
            s_cap_deep_span_x(uy_b, thr_b, -1, cx256, &xlo_b, &xhi_b);
    }

    for (int32_t y = cy_lo; y <= cy_hi; y++) {
        if (y < clip_y0 || y >= clip_y1) continue;
        int32_t dy256 = (y * 256) + 128 - cy256;
        int64_t dy2   = (int64_t)dy256 * dy256;
        int64_t b     = ex_out - dy2;                  /* 有覆盖半宽的平方 */
        if (b <= 0) continue;
        /* xout 以 1/256 px 为单位，折成像素半宽；+1 覆盖像素中心半格偏移 */
        int32_t xout = (int32_t)(s_isqrt_fit(b) >> 8) + 1;

        int32_t xs = (int32_t)cx - xout, xe = (int32_t)cx + xout;
        if (xs < clip_x0) xs = clip_x0;
        if (xe >= clip_x1) xe = clip_x1 - 1;

        /* 行级端帽处理（仅快路径）：uy==0 的端帽 cross 是行常量，整行同侧。
         * ux==0 的端帽把"深内侧"区间解析解出后**直接钳制扫描范围**——轴对齐
         * 端帽（Q14 单位向量）没有跨缝像素：|cross| < thr ⇔ |vx| < 128，而
         * vx = 256(x-cx)+128 对整数 x 恒有 |vx| >= 128，故 s_cap_side 只会
         * 返回 ±1、锥面积路径永不触发，区间外像素与慢路径的 continue 等价。 */
        bool skip_row = false, row_all_pixel = false;
        if (fast && sweep < 360) {
            if (uy_a == 0) {
                int sd = s_cap_side((int64_t)ux_a * dy256, thr_a, 1);
                if (sd < 0) skip_row = true;
                else if (sd == 0) row_all_pixel = true;
            } else {
                if (xlo_a > xs) xs = xlo_a;
                if (xhi_a < xe) xe = xhi_a;
            }
            if (uy_b == 0) {
                int sd = s_cap_side((int64_t)ux_b * dy256, thr_b, 0);
                if (sd < 0) skip_row = true;
                else if (sd == 0) row_all_pixel = true;
            } else {
                if (xlo_b > xs) xs = xlo_b;
                if (xhi_b < xe) xe = xhi_b;
            }
        }
        if (skip_row || xs > xe) continue;

        /* 径向全满段：xin = isqrt(in_out - dy2) 向下取整到像素。区间
         * [cx-xin, cx+xin-1] 内 |vx| <= 256*xin - 128 → d2 <= in_out 恒成立
         * （像素中心 +0.5 偏移使左右不对称），慢路径对这些像素给出 rad==256
         * → px_set(fg)，直填逐位等价。 */
        int32_t span_lo = INT32_MAX, span_hi = INT32_MIN;
        if (fast && !row_all_pixel) {
            int64_t b_full = in_out - dy2;
            if (b_full > 0) {
                int32_t xin = (int32_t)(s_isqrt_fit(b_full) >> 8);
                span_lo = (int32_t)cx - xin;
                span_hi = (int32_t)cx + xin - 1;
                if (sweep < 360) {
                    if (span_lo < xlo_a) span_lo = xlo_a;
                    if (span_lo < xlo_b) span_lo = xlo_b;
                    if (span_hi > xhi_a) span_hi = xhi_a;
                    if (span_hi > xhi_b) span_hi = xhi_b;
                }
                if (span_lo < xs) span_lo = xs;
                if (span_hi > xe) span_hi = xe;
                if (span_lo > span_hi) {
                    span_lo = INT32_MAX;
                    span_hi = INT32_MIN;
                }
            }
        }

        for (int32_t x = xs; x <= xe; x++) {
            if (x == span_lo && span_hi >= span_lo) {
                /* 跨度段连续直填（仅 FULL 模式画布；PAGE 条带布局不同，退回
                 * px_set）。直填语义 = 对每个像素 px_set(fg)，与慢路径的
                 * rad==256 分支逐位一致，但免去逐像素函数调用 + clip 判断。 */
                if (!(c->display->caps.buffer_mode & EUI_BUFFER_PAGE)) {
                    uint16_t *row = (uint16_t *)c->buffer + (uint32_t)y * c->buf_width;
                    for (int32_t xi = span_lo; xi <= span_hi; xi++)
                        row[xi] = (uint16_t)c->fg_color;
                    x = span_hi;
                    continue;
                }
                eui_canvas_px_set(c, (int16_t)x, (int16_t)y, c->fg_color);
                continue;
            }
            int32_t vx = ((x * 256) + 128) - cx256;      /* p - c 的 x 分量（有符号） */
            int64_t d2 = (int64_t)vx * vx + dy2;

            /* --- 径向覆盖度 --- */
            int32_t rad;
            if (d2 >= in_in && d2 <= in_out) {
                rad = 256;                              /* 内外都全覆盖，不做距离测试 */
            } else if (d2 >= ex_out || d2 <= ex_in) {
                continue;                               /* 完全在外或完全在孔内 */
            } else {
                int32_t d256 = (int32_t)s_isqrt_fit(d2);
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
                int fa = s_cap_side(cr_a, thr_a, 1);
                int fb = s_cap_side(cr_b, thr_b, 0);
                int32_t ang;
                if (!complement) {
                    if (fa < 0 || fb < 0) continue;     /* 深在扇形外 */
                    ang = (fa > 0 && fb > 0) ? 256
                        : s_cone_cov256(vx, dy256, ux_a, uy_a, ux_b, uy_b);
                } else {
                    if (fa > 0 && fb > 0) continue;     /* 深在补扇形内 → 角向 0 */
                    ang = (fa < 0 || fb < 0) ? 256      /* 深在补扇形外 → 角向满 */
                        : 256 - s_cone_cov256(vx, dy256, ux_a, uy_a, ux_b, uy_b);
                }
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

/* ---- 公开曲线原语：内核之上的薄包装 ---------------------------------------
 * 签名与几何契约（圆心、l/t/ri/b、半径夹取规则）与旧的中点光栅器逐字一致，
 * 只有边缘从硬阈值变为覆盖度混合。r == 0 的退化情形仍是单像素。 */

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
    /* 角弧圆心取**外边界内缩 r**：右上 (x+w-r, y+r) = (ri+1, t)、右下 (x+w-r, y+h-r)
     * = (ri+1, b+1)、左下 (x+r, y+h-r) = (l, b+1)。直边最后一列/行 ri/b 比右、下角心
     * 小 1，直接拿它们当圆心会让右、下两侧的角弧整体内缩 1 px：轮廓在"直边与角弧相切"
     * 的那一行/列出现 1 px 台阶，描边版更是 1 px 断口（弧接不到直边那一列）。旧中点
     * 光栅器把"含边界点的像素"整格涂满，恰好掩盖了这 1 px 差，所以是 AA 化后才显形的。 */
    eui_canvas_aa_arc(canvas, l,      t,     r, rin, 180, 270);   /* 左上 */
    eui_canvas_aa_arc(canvas, ri + 1, t,     r, rin, 270, 360);   /* 右上 */
    eui_canvas_aa_arc(canvas, ri + 1, b + 1, r, rin,   0,  90);   /* 右下 */
    eui_canvas_aa_arc(canvas, l,      b + 1, r, rin,  90, 180);   /* 左下 */
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
    /* 四个角用四分之一圆盘补齐，圆心取外边界内缩 r（见 draw_round_rect 处的说明）：
     * 左/上角心与旧式的 (l,t)/(ri,t) 恰好相同，右/下角心是 (ri+1, b+1)，比 ri/b 大 1。
     * 角盘与中间矩形的像素集合会相接（例如中间矩形的 ri 列被右上角盘部分覆盖，实测
     * w=70/h=24/r=6 下 4 个像素），但这无妨：矩形先以全覆盖写下 fg，角盘随后的部分混合
     * 算出的 g 恒等于 gray(fg) 且落在量化网格上（rem == 0 → quantize 恒等），值不变——
     * 没有任何像素被部分覆盖混合两次。
     * 退化配置（2r == w / 2r == h，即相邻两角盘圆心重合）也因此变得精确：圆心是整数，
     * 两个盘的分界轴正好落在像素**边界**上（没有像素被轴切开），每个像素整格属于某一侧，
     * 不会被两次部分混合。实测三种退化组合与 64 子样本参考的最大偏差 16/256，与非退化
     * 配置同量级（见 test_canvas_aa.c 的 C16 参考门）。
     * **不要**改用 r_in = r-1：角内部没有矩形覆盖、只由该角盘覆盖，那样会在每个角挖出
     * 一个 r x r 的空洞。 */
    eui_canvas_aa_arc(canvas, l,      t,     r, 0, 180, 270);
    eui_canvas_aa_arc(canvas, ri + 1, t,     r, 0, 270, 360);
    eui_canvas_aa_arc(canvas, ri + 1, b + 1, r, 0,   0,  90);
    eui_canvas_aa_arc(canvas, l,      b + 1, r, 0,  90, 180);
}

/* ---- 圆弧/圆环/扇形入口：内核之上的薄包装 --------------------- */
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
