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
 * 扫描线 + 解析覆盖度：每行先用两个整数开方阈值定位"全覆盖区间"，只有边界带
 * 逐像素算径向距离；同一份距离也喂给端帽（角向）判定。角度为整数度，
 * 0° = 3 点钟、顺时针（屏幕 y 向下），与公开 API 一致。全整数、无浮点。 */

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
    int32_t clip_y0 = c->clip.y, clip_y1 = (int32_t)c->clip.y + c->clip.h;

    for (int32_t y = cy_lo; y <= cy_hi; y++) {
        if (y < clip_y0 || y >= clip_y1) continue;
        int32_t dy256 = (y * 256) + 128 - cy256;
        int64_t dy2   = (int64_t)dy256 * dy256;
        int64_t b     = ex_out - dy2;                  /* 有覆盖半宽的平方 */
        if (b <= 0) continue;
        /* xout 以 1/256 px 为单位，折成像素半宽；+1 覆盖像素中心半格偏移 */
        int32_t xout = (int32_t)(eui_canvas_isqrt((uint64_t)b, 0) >> 8) + 1;

        int32_t xs = (int32_t)cx - xout, xe = (int32_t)cx + xout;
        if (xs < clip_x0) xs = clip_x0;
        if (xe >= clip_x1) xe = clip_x1 - 1;

        for (int32_t x = xs; x <= xe; x++) {
            int32_t vx = ((x * 256) + 128) - cx256;      /* p - c 的 x 分量（有符号） */
            int64_t d2 = (int64_t)vx * vx + dy2;

            /* --- 径向覆盖度 --- */
            int32_t rad;
            if (d2 >= in_in && d2 <= in_out) {
                rad = 256;                              /* 内外都全覆盖，不做距离测试 */
            } else if (d2 >= ex_out || d2 <= ex_in) {
                continue;                               /* 完全在外或完全在孔内 */
            } else {
                int32_t d256 = (int32_t)eui_canvas_isqrt((uint64_t)d2, 0);
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
    /* 四个角用四分之一圆盘补齐。角盘与中间矩形的像素集合会相接（例如中间矩形的 ri 列
     * 被右上角盘部分覆盖，实测 w=70/h=24/r=6 下 4 个像素），但这无妨：矩形先以全覆盖
     * 写下 fg，角盘随后的部分混合算出的 g 恒等于 gray(fg) 且落在量化网格上（rem == 0
     * → quantize 恒等），值不变——没有任何像素被部分覆盖混合两次。
     * 已知限制：2r == w（或 2r == h）时相邻两角盘共享一列（一行），该列被两次部分混合，
     * 向 fg 偏移实测 +1..+18/256（r = 2..24；r=1 的退化情形 53），每处 1-2 px 接缝。 */
    eui_canvas_aa_arc(canvas, l,  t,  r, 0, 180, 270);
    eui_canvas_aa_arc(canvas, ri, t,  r, 0, 270, 360);
    eui_canvas_aa_arc(canvas, ri, b,  r, 0,   0,  90);
    eui_canvas_aa_arc(canvas, l,  b,  r, 0,  90, 180);
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
