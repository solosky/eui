#ifndef EUI_CANVAS_INTERNAL_H
#define EUI_CANVAS_INTERNAL_H

#include "eui/eui_canvas.h"

/* 光栅化器内部共享的像素访问，语义与公开绘图 API 一致：
 * 受 clip 与屏幕边界约束；越界写被丢弃，越界读返回 0。 */
void        eui_canvas_px_set(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t color);
eui_color_t eui_canvas_px_get(eui_canvas_t *c, int16_t x, int16_t y);

/* 覆盖度混合：cov 0 → 不写；cov 255 → 直接写 fg；其余读 dst 后按色深混合。
 * 抖动路径的阈值相位锚在 (x, y + c->page_y_offset)，跨 PAGE band 连续。
 * cov 与 VLW 字形的 alpha 同量纲（0..255），16bpp 分支与旧公式逐位一致。 */
void        eui_canvas_px_blend(eui_canvas_t *c, int16_t x, int16_t y,
                               eui_color_t fg, uint8_t cov);

/* 逐位法整数开方：返回 floor(sqrt(x) * 2^frac_bits)。
 * 约束：x << (2 * frac_bits) 必须落在 uint64 内（frac_bits = 8 时 x < 2^48）。
 * 光栅器用法是 frac_bits = 0、radicand 以 (1/256 px)^2 为单位，
 * 于是返回值直接是 1/256 px 单位的长度。 */
uint32_t    eui_canvas_isqrt(uint64_t x, uint8_t frac_bits);

/* 泛化圆弧光栅化内核，供圆/圆角矩形/圆弧/圆环/扇形共用。
 * 坐标系与公开 API 一致；角度为整数度，0° = 3 点钟方向，顺时针（屏幕 y 向下）。 */
void eui_canvas_aa_arc(eui_canvas_t *c, int16_t cx, int16_t cy,
                       uint16_t r_out, uint16_t r_in,
                       int16_t start_deg, int16_t end_deg);

#endif /* EUI_CANVAS_INTERNAL_H */
