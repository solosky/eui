#include "eui/eui_font.h"
#include "eui_font_internal.h"

uint8_t eui_font_vlw_get_char_width(const eui_font_t *font, char c)
{
    (void)font; (void)c;
    return 0;
}

uint16_t eui_font_vlw_get_str_width(const eui_font_t *font, const char *str)
{
    (void)font; (void)str;
    return 0;
}

uint8_t eui_font_vlw_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth)
{
    (void)font; (void)c; (void)buf; (void)buf_stride; (void)color_depth;
    return 0;
}
