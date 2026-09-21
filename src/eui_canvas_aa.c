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
