#ifndef EUI_FONT_H
#define EUI_FONT_H

#include "eui/eui_types.h"
#include <stdint.h>

#define EUI_FONT_FORMAT_BDF  0
#define EUI_FONT_FORMAT_VLW  1

uint8_t  eui_font_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_get_str_width(const eui_font_t *font, const char *str);
uint8_t  eui_font_get_height(const eui_font_t *font);
uint8_t  eui_font_get_baseline(const eui_font_t *font);

uint8_t eui_font_draw_char(const eui_font_t *font, char c,
                            uint8_t *buf, uint16_t buf_stride,
                            uint8_t color_depth);

#endif /* EUI_FONT_H */
