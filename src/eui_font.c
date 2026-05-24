#include "eui/eui_font.h"
#include "eui/eui_font_internal.h"

uint8_t eui_font_get_char_width(const eui_font_t *font, char c)
{
    if (!font) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_get_char_width(font, c);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_get_char_width(font, c);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_get_char_width(font, c);
#endif
    return 0;
}

uint16_t eui_font_get_str_width(const eui_font_t *font, const char *str)
{
    if (!font) return 0;
    if (!str) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_get_str_width(font, str);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_get_str_width(font, str);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_get_str_width(font, str);
#endif
    return 0;
}

uint8_t eui_font_get_height(const eui_font_t *font)
{
    if (!font) return 0;
    return font->line_height;
}

uint8_t eui_font_get_baseline(const eui_font_t *font)
{
    if (!font) return 0;
    return font->baseline;
}

uint8_t eui_font_draw_char(const eui_font_t *font, char c,
                            uint8_t *buf, uint16_t buf_stride,
                            uint8_t color_depth)
{
    if (!font) return 0;
    if (!buf) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_draw_char(font, c, buf, buf_stride, color_depth);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_draw_char(font, c, buf, buf_stride, color_depth);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_draw_char(font, c, buf, buf_stride, color_depth);
#endif
    return 0;
}
