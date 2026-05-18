#ifndef EUI_CANVAS_H
#define EUI_CANVAS_H

#include "eui/eui_types.h"
#include "eui/eui_display_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct eui_canvas_t {
    eui_display_hal_t *display;
    uint8_t           *buffer;
    uint16_t           buf_width;
    uint16_t           buf_height;
    eui_color_t        fg_color;
    eui_color_t        bg_color;
    const eui_font_t  *font;
    eui_rect_t         clip;
    uint8_t            state_stack_idx;
    eui_rect_t         state_stack_clip[EUI_CANVAS_STATE_STACK];
    eui_color_t        state_stack_fg[EUI_CANVAS_STATE_STACK];
    eui_color_t        state_stack_bg[EUI_CANVAS_STATE_STACK];
    uint8_t            page_current;
    uint8_t            page_total;
    uint16_t           page_y_offset;
} eui_canvas_t;

/* Lifecycle */
eui_canvas_t* eui_canvas_create(eui_display_hal_t *display);
void eui_canvas_destroy(eui_canvas_t *canvas);
void eui_canvas_reset(eui_canvas_t *canvas);
void eui_canvas_commit(eui_canvas_t *canvas);

/* Properties */
uint16_t eui_canvas_width(const eui_canvas_t *canvas);
uint16_t eui_canvas_height(const eui_canvas_t *canvas);
void eui_canvas_set_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_bg_color(eui_canvas_t *canvas, eui_color_t color);
void eui_canvas_set_clip(eui_canvas_t *canvas, const eui_rect_t *rect);
void eui_canvas_clear_clip(eui_canvas_t *canvas);
void eui_canvas_save(eui_canvas_t *canvas);
void eui_canvas_restore(eui_canvas_t *canvas);

/* Drawing primitives */
void eui_canvas_clear(eui_canvas_t *canvas);
void eui_canvas_draw_dot(eui_canvas_t *canvas, int16_t x, int16_t y);
void eui_canvas_draw_line(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void eui_canvas_draw_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_canvas_fill_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);
void eui_canvas_draw_triangle(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);
void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);
void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);

/* Text */
void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font);
uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str);
uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      eui_align_t h_align, eui_align_t v_align, const char *str);
uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str);
uint16_t eui_canvas_font_height(const eui_canvas_t *canvas);

/* Images */
void eui_canvas_draw_xbm(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint8_t *data);
void eui_canvas_draw_bitmap(eui_canvas_t *canvas, int16_t x, int16_t y, const eui_bitmap_t *bmp);

/* Advanced */
void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);

/* PAGE mode control */
bool eui_canvas_begin_page(eui_canvas_t *canvas);
bool eui_canvas_next_page(eui_canvas_t *canvas);

#endif /* EUI_CANVAS_H */
