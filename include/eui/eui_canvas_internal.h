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
