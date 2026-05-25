#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include "eui/eui_font.h"
#include <string.h>
#include <stdlib.h>

#if EUI_FONT_ENABLE_U8G2
#include "eui/eui_font_u8g2_internal.h"
/* Needed by draw_u8g2_glyph */
int32_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev);
#endif

#define CANVAS_PAGE_BAND_HEIGHT 8

static uint32_t utf8_decode_next(const char **s) {
    const uint8_t *p = (const uint8_t*)(*s);
    uint32_t cp;
    if ((p[0] & 0x80) == 0) {
        cp = p[0]; *s += 1;
    } else if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); *s += 2;
    } else if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); *s += 3;
    } else if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); *s += 4;
    } else {
        cp = p[0]; *s += 1;
    }
    return cp;
}

static void canvas_set_pixel(eui_canvas_t *c, int16_t x, int16_t y, eui_color_t color)
{
    if (x < c->clip.x || x >= c->clip.x + (int16_t)c->clip.w ||
        y < c->clip.y || y >= c->clip.y + (int16_t)c->clip.h) {
        return;
    }

    uint16_t screen_h = eui_canvas_height(c);
    uint16_t screen_w = eui_canvas_width(c);

    if (x < 0 || x >= (int16_t)screen_w || y < 0 || y >= (int16_t)screen_h) return;

#if EUI_COLOR_DEPTH == 1
    uint16_t byte_idx = (y * (screen_w / 8)) + (x / 8);
    uint8_t bit_pos = x % 8;
    if (color) {
        c->buffer[byte_idx] |= (1u << bit_pos);
    } else {
        c->buffer[byte_idx] &= ~(1u << bit_pos);
    }
#elif EUI_COLOR_DEPTH == 2
    uint16_t byte_idx = (uint16_t)(y * (int16_t)(screen_w / 4u)) + (uint16_t)(x / 4u);
    uint8_t shift = 6u - 2u * (uint8_t)(x % 4u);
    c->buffer[byte_idx] = (uint8_t)(c->buffer[byte_idx] & ~(3u << shift)) | ((color & 3u) << shift);
#elif EUI_COLOR_DEPTH == 4
    uint16_t byte_idx = (uint16_t)(y * (int16_t)(screen_w / 2u)) + (uint16_t)(x / 2u);
    if (x & 1) {
        c->buffer[byte_idx] = (uint8_t)(c->buffer[byte_idx] & 0xF0u) | (color & 0x0Fu);
    } else {
        c->buffer[byte_idx] = (uint8_t)(c->buffer[byte_idx] & 0x0Fu) | ((uint8_t)(color & 0x0Fu) << 4);
    }
#elif EUI_COLOR_DEPTH == 8
    c->buffer[y * screen_w + x] = color;
#elif EUI_COLOR_DEPTH == 16
    uint16_t *buf16 = (uint16_t*)c->buffer;
    buf16[y * screen_w + x] = color;
#else
    (void)c; (void)x; (void)y; (void)color;
#endif
}

static size_t canvas_buf_size(eui_canvas_t *c)
{
    size_t pixels = (size_t)c->buf_width * c->buf_height;
#if EUI_COLOR_DEPTH == 1
    return pixels / 8;
#elif EUI_COLOR_DEPTH == 2
    return pixels / 4u;
#elif EUI_COLOR_DEPTH == 4
    return pixels / 2u;
#elif EUI_COLOR_DEPTH == 8
    return pixels;
#elif EUI_COLOR_DEPTH == 16
    return pixels * 2;
#else
    return pixels / 8;
#endif
}

/* ---- Lifecycle ---- */

eui_canvas_t* eui_canvas_create(eui_display_drv_t *display)
{
    eui_canvas_t *c = (eui_canvas_t *)eui_malloc(sizeof(eui_canvas_t));
    if (!c) return NULL;
    memset(c, 0, sizeof(eui_canvas_t));

    c->display = display;
    c->fg_color = EUI_COLOR_WHITE;
    c->bg_color = EUI_COLOR_BLACK;
    c->font = NULL;
    c->state_stack_idx = 0;

    if (display->caps.buffer_mode & EUI_BUFFER_DIRECT) {
        c->buffer = NULL;
        c->buf_width = 0;
        c->buf_height = 0;
    } else if (display->caps.buffer_mode & EUI_BUFFER_PAGE) {
        c->buf_width = display->caps.width;
        c->buf_height = CANVAS_PAGE_BAND_HEIGHT;
        c->buffer = (uint8_t *)eui_malloc(canvas_buf_size(c));
        if (!c->buffer) {
            eui_free(c);
            return NULL;
        }
        memset(c->buffer, 0, canvas_buf_size(c));
    } else {
        c->buf_width = display->caps.width;
        c->buf_height = display->caps.height;
        c->buffer = (uint8_t *)eui_malloc(canvas_buf_size(c));
        if (!c->buffer) {
            eui_free(c);
            return NULL;
        }
        memset(c->buffer, 0, canvas_buf_size(c));
    }

    c->clip.x = 0;
    c->clip.y = 0;
    c->clip.w = display->caps.width;
    c->clip.h = display->caps.height;

    return c;
}

void eui_canvas_destroy(eui_canvas_t *canvas)
{
    if (!canvas) return;
    if (canvas->buffer) {
        eui_free(canvas->buffer);
    }
    eui_free(canvas);
}

void eui_canvas_reset(eui_canvas_t *canvas)
{
    if (!canvas) return;
    canvas->fg_color = EUI_COLOR_WHITE;
    canvas->bg_color = EUI_COLOR_BLACK;
    canvas->font = NULL;
    canvas->clip.x = 0;
    canvas->clip.y = 0;
    canvas->clip.w = canvas->display->caps.width;
    canvas->clip.h = canvas->display->caps.height;
    canvas->state_stack_idx = 0;
}

/* ---- Properties ---- */

uint16_t eui_canvas_width(const eui_canvas_t *canvas)
{
    if (!canvas || !canvas->display) return 0;
    return canvas->display->caps.width;
}

uint16_t eui_canvas_height(const eui_canvas_t *canvas)
{
    if (!canvas || !canvas->display) return 0;
    return canvas->display->caps.height;
}

void eui_canvas_set_color(eui_canvas_t *canvas, eui_color_t color)
{
    if (!canvas) return;
    canvas->fg_color = color;
}

void eui_canvas_set_bg_color(eui_canvas_t *canvas, eui_color_t color)
{
    if (!canvas) return;
    canvas->bg_color = color;
}

void eui_canvas_set_clip(eui_canvas_t *canvas, const eui_rect_t *rect)
{
    if (!canvas || !rect) return;
    canvas->clip = *rect;
}

void eui_canvas_clear_clip(eui_canvas_t *canvas)
{
    if (!canvas) return;
    canvas->clip.x = 0;
    canvas->clip.y = 0;
    canvas->clip.w = canvas->display->caps.width;
    canvas->clip.h = canvas->display->caps.height;
}

void eui_canvas_save(eui_canvas_t *canvas)
{
    if (!canvas) return;
    if (canvas->state_stack_idx >= EUI_CANVAS_STATE_STACK) return;
    canvas->state_stack_clip[canvas->state_stack_idx] = canvas->clip;
    canvas->state_stack_fg[canvas->state_stack_idx] = canvas->fg_color;
    canvas->state_stack_bg[canvas->state_stack_idx] = canvas->bg_color;
    canvas->state_stack_idx++;
}

void eui_canvas_restore(eui_canvas_t *canvas)
{
    if (!canvas) return;
    if (canvas->state_stack_idx == 0) return;
    canvas->state_stack_idx--;
    canvas->clip = canvas->state_stack_clip[canvas->state_stack_idx];
    canvas->fg_color = canvas->state_stack_fg[canvas->state_stack_idx];
    canvas->bg_color = canvas->state_stack_bg[canvas->state_stack_idx];
}

/* ---- Drawing primitives ---- */

void eui_canvas_clear(eui_canvas_t *canvas)
{
    if (!canvas || !canvas->buffer) return;
    size_t size = canvas_buf_size(canvas);
#if EUI_COLOR_DEPTH == 1
    memset(canvas->buffer, canvas->bg_color ? 0xFF : 0x00, size);
#elif EUI_COLOR_DEPTH == 2
    uint8_t fill = (uint8_t)(canvas->bg_color & 3u);
    fill = (uint8_t)(fill | (fill << 2) | (fill << 4) | (fill << 6));
    memset(canvas->buffer, fill, size);
#elif EUI_COLOR_DEPTH == 4
    uint8_t fill = (uint8_t)(canvas->bg_color & 0x0Fu);
    fill = (uint8_t)(fill | (fill << 4));
    memset(canvas->buffer, fill, size);
#else
    /* For 8bpp/16bpp, fill with bg_color */
    if (canvas->bg_color == 0) {
        memset(canvas->buffer, 0, size);
#if EUI_COLOR_DEPTH == 16
    } else {
        uint16_t *buf16 = (uint16_t *)canvas->buffer;
        size_t pixels = (size_t)canvas->buf_width * canvas->buf_height;
        for (size_t i = 0; i < pixels; i++) {
            buf16[i] = canvas->bg_color;
        }
#else
    } else {
        for (size_t i = 0; i < size; i++) {
            canvas->buffer[i] = (uint8_t)canvas->bg_color;
        }
#endif
    }
#endif
}

void eui_canvas_draw_dot(eui_canvas_t *canvas, int16_t x, int16_t y)
{
    if (!canvas) return;
    canvas_set_pixel(canvas, x, y, canvas->fg_color);
}

void eui_canvas_draw_line(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    if (!canvas) return;

    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    int16_t step_x, step_y;
    int16_t err, e2;

    if (dx < 0) { dx = -dx; step_x = -1; } else { step_x = 1; }
    if (dy < 0) { dy = -dy; step_y = -1; } else { step_y = 1; }

    if (dx > dy) {
        err = dx / 2;
        while (x1 != x2) {
            canvas_set_pixel(canvas, x1, y1, canvas->fg_color);
            err -= dy;
            if (err < 0) { y1 += step_y; err += dx; }
            x1 += step_x;
        }
    } else {
        err = dy / 2;
        while (y1 != y2) {
            canvas_set_pixel(canvas, x1, y1, canvas->fg_color);
            err -= dx;
            if (err < 0) { x1 += step_x; err += dy; }
            y1 += step_y;
        }
    }
    canvas_set_pixel(canvas, x2, y2, canvas->fg_color);
}

void eui_canvas_fill_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!canvas || w == 0 || h == 0) return;
    int16_t ex = x + (int16_t)w;
    int16_t ey = y + (int16_t)h;
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            canvas_set_pixel(canvas, xi, yi, canvas->fg_color);
        }
    }
}

void eui_canvas_draw_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!canvas || w == 0 || h == 0) return;
    int16_t x2 = x + (int16_t)w - 1;
    int16_t y2 = y + (int16_t)h - 1;
    if (w == 1 && h == 1) {
        canvas_set_pixel(canvas, x, y, canvas->fg_color);
        return;
    }
    eui_canvas_draw_line(canvas, x, y, x2, y);
    eui_canvas_draw_line(canvas, x2, y, x2, y2);
    eui_canvas_draw_line(canvas, x2, y2, x, y2);
    eui_canvas_draw_line(canvas, x, y2, x, y);
}

void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x0, int16_t y0, uint16_t r)
{
    if (!canvas || r == 0) {
        if (r == 0) canvas_set_pixel(canvas, x0, y0, canvas->fg_color);
        return;
    }

    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 0;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t x = 0;
    int16_t y = (int16_t)r;

    canvas_set_pixel(canvas, x0, y0 + (int16_t)r, canvas->fg_color);
    canvas_set_pixel(canvas, x0, y0 - (int16_t)r, canvas->fg_color);
    canvas_set_pixel(canvas, x0 + (int16_t)r, y0, canvas->fg_color);
    canvas_set_pixel(canvas, x0 - (int16_t)r, y0, canvas->fg_color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;

        canvas_set_pixel(canvas, x0 + x, y0 + y, canvas->fg_color);
        canvas_set_pixel(canvas, x0 - x, y0 + y, canvas->fg_color);
        canvas_set_pixel(canvas, x0 + x, y0 - y, canvas->fg_color);
        canvas_set_pixel(canvas, x0 - x, y0 - y, canvas->fg_color);
        canvas_set_pixel(canvas, x0 + y, y0 + x, canvas->fg_color);
        canvas_set_pixel(canvas, x0 - y, y0 + x, canvas->fg_color);
        canvas_set_pixel(canvas, x0 + y, y0 - x, canvas->fg_color);
        canvas_set_pixel(canvas, x0 - y, y0 - x, canvas->fg_color);
    }
}

void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x0, int16_t y0, uint16_t r)
{
    if (!canvas) return;
    if (r == 0) {
        canvas_set_pixel(canvas, x0, y0, canvas->fg_color);
        return;
    }

    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 0;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t x = 0;
    int16_t y = (int16_t)r;

    eui_canvas_draw_line(canvas, x0 - (int16_t)r, y0, x0 + (int16_t)r, y0);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;

        eui_canvas_draw_line(canvas, x0 - x, y0 + y, x0 + x, y0 + y);
        eui_canvas_draw_line(canvas, x0 - x, y0 - y, x0 + x, y0 - y);
        eui_canvas_draw_line(canvas, x0 - y, y0 + x, x0 + y, y0 + x);
        eui_canvas_draw_line(canvas, x0 - y, y0 - x, x0 + y, y0 - x);
    }
}

void eui_canvas_draw_triangle(eui_canvas_t *canvas, int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2, int16_t x3, int16_t y3)
{
    eui_canvas_draw_line(canvas, x1, y1, x2, y2);
    eui_canvas_draw_line(canvas, x2, y2, x3, y3);
    eui_canvas_draw_line(canvas, x3, y3, x1, y1);
}

void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h, uint16_t r)
{
    if (!canvas || w == 0 || h == 0) return;
    if (r == 0) {
        eui_canvas_draw_rect(canvas, x, y, w, h);
        return;
    }

    uint16_t r2 = r * 2;
    if (r2 > w) r = w / 2;
    if (r2 > h) r = h / 2;

    int16_t l = x + (int16_t)r;
    int16_t t = y + (int16_t)r;
    int16_t ri = x + (int16_t)w - (int16_t)r - 1;
    int16_t b = y + (int16_t)h - (int16_t)r - 1;

    eui_canvas_draw_line(canvas, l, y, ri, y);
    eui_canvas_draw_line(canvas, l, y + (int16_t)h - 1, ri, y + (int16_t)h - 1);
    eui_canvas_draw_line(canvas, x, t, x, b);
    eui_canvas_draw_line(canvas, x + (int16_t)w - 1, t, x + (int16_t)w - 1, b);

    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 0;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t cx = 0;
    int16_t cy = (int16_t)r;

    while (cx < cy) {
        if (f >= 0) {
            cy--;
            ddF_y += 2;
            f += ddF_y;
        }
        cx++;
        ddF_x += 2;
        f += ddF_x + 1;

        canvas_set_pixel(canvas, l - cx, t - cy, canvas->fg_color);
        canvas_set_pixel(canvas, ri + cx, t - cy, canvas->fg_color);
        canvas_set_pixel(canvas, l - cx, b + cy, canvas->fg_color);
        canvas_set_pixel(canvas, ri + cx, b + cy, canvas->fg_color);

        canvas_set_pixel(canvas, l - cy, t - cx, canvas->fg_color);
        canvas_set_pixel(canvas, ri + cy, t - cx, canvas->fg_color);
        canvas_set_pixel(canvas, l - cy, b + cx, canvas->fg_color);
        canvas_set_pixel(canvas, ri + cy, b + cx, canvas->fg_color);
    }
}

void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h, uint16_t r)
{
    if (!canvas || w == 0 || h == 0) return;
    if (r == 0) {
        eui_canvas_fill_rect(canvas, x, y, w, h);
        return;
    }

    uint16_t r2 = r * 2;
    if (r2 > w) r = w / 2;
    if (r2 > h) r = h / 2;

    int16_t l = x + (int16_t)r;
    int16_t t = y + (int16_t)r;
    int16_t ri = x + (int16_t)w - (int16_t)r - 1;
    int16_t b = y + (int16_t)h - (int16_t)r - 1;

    eui_canvas_fill_rect(canvas, l, y, (uint16_t)(ri - l + 1), h);

    if (b > t) {
        eui_canvas_fill_rect(canvas, x, t, r, (uint16_t)(b - t + 1));
        eui_canvas_fill_rect(canvas, ri + 1, t, r, (uint16_t)(b - t + 1));
    }

    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 0;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t cx = 0;
    int16_t cy = (int16_t)r;

    while (cx < cy) {
        if (f >= 0) {
            cy--;
            ddF_y += 2;
            f += ddF_y;
        }
        cx++;
        ddF_x += 2;
        f += ddF_x + 1;

        eui_canvas_draw_line(canvas, l - cx, t - cy, ri + cx, t - cy);
        eui_canvas_draw_line(canvas, l - cx, b + cy, ri + cx, b + cy);

        eui_canvas_draw_line(canvas, l - cy, t - cx, ri + cy, t - cx);
        eui_canvas_draw_line(canvas, l - cy, b + cx, ri + cy, b + cx);
    }
}

/* ---- Text ---- */

void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font)
{
    if (!canvas) return;
    canvas->font = font;
}

#if EUI_FONT_ENABLE_U8G2
static void draw_u8g2_glyph(eui_canvas_t *canvas, const eui_font_t *font,
                            uint16_t encoding, uint16_t prev,
                            int16_t x, int16_t y, uint8_t *adv_out)
{
    u8g2_glyph_t g;
    int32_t idx = eui_font_u8g2_lookup_glyph(font, encoding, prev);
    if (idx < 0 || !decode_glyph_at(font, (uint32_t)idx, &g)) {
        if (adv_out) *adv_out = 0;
        return;
    }
    for (uint8_t row = 0; row < g.height; row++) {
        for (uint8_t col = 0; col < g.width; col++) {
            uint16_t px = (uint16_t)row * g.width + col;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, px,
                                   g.width, g.height)) {
                canvas_set_pixel(canvas,
                                 x + col + g.x_offset,
                                 (int16_t)(y - g.height - g.y_offset + row),
                                 canvas->fg_color);
            }
        }
    }
    if (adv_out) *adv_out = g.x_advance;
}

/* Draw a u8g2 glyph by data offset (for kerning) */
static void draw_u8g2_glyph_by_index(eui_canvas_t *canvas,
                                      const eui_font_t *font,
                                      uint32_t idx, int16_t x, int16_t y,
                                      uint8_t *adv_out)
{
    u8g2_glyph_t g;
    if (!decode_glyph_at(font, idx, &g)) {
        if (adv_out) *adv_out = 0;
        return;
    }
    for (uint8_t row = 0; row < g.height; row++) {
        for (uint8_t col = 0; col < g.width; col++) {
            uint16_t px = (uint16_t)row * g.width + col;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, px,
                                   g.width, g.height)) {
                canvas_set_pixel(canvas,
                                 x + col + g.x_offset,
                                 (int16_t)(y - g.height - g.y_offset + row),
                                 canvas->fg_color);
            }
        }
    }
    if (adv_out) *adv_out = g.x_advance;
}
#endif

static void draw_bdf_glyph(eui_canvas_t *canvas, const eui_font_t *font,
                           char c, int16_t x, int16_t y, uint8_t *adv_out)
{
    const uint8_t *p = font->data;
    uint8_t first = p[0];
    uint8_t last  = p[1];
    if ((uint8_t)c < first || (uint8_t)c > last) {
        if (adv_out) *adv_out = 0;
        return;
    }

    uint8_t idx = (uint8_t)c - first;
    uint8_t glyph_count = (uint8_t)(last - first + 1);

    const uint8_t *data_start = p + 3 + glyph_count * 2;
    uint16_t offset = p[3 + idx * 2] | ((uint16_t)p[3 + idx * 2 + 1] << 8);
    const uint8_t *g = data_start + offset;

    uint8_t gw = g[0];
    uint8_t gh = g[1];
    int8_t  x_off = (int8_t)g[2];
    int8_t  y_off = (int8_t)g[3];
    uint8_t x_adv = g[4];
    uint8_t bytes_per_row = (gw + 7) / 8;
    const uint8_t *bitmap = g + 5;

    for (uint8_t row = 0; row < gh; row++) {
        for (uint8_t col = 0; col < gw; col++) {
            uint8_t byte = bitmap[row * bytes_per_row + col / 8];
            uint8_t bit = (byte >> (7 - (col % 8))) & 1;
            if (bit) {
                canvas_set_pixel(canvas, x + col + x_off, y + row + y_off,
                                 canvas->fg_color);
            }
        }
    }

    if (adv_out) *adv_out = x_adv;
}

static void draw_glyph(eui_canvas_t *canvas, const eui_font_t *font,
                       char c, uint16_t prev, int16_t x, int16_t y, uint8_t *adv_out)
{
    if (!font || !font->data) { if (adv_out) *adv_out = 0; return; }
    if (font->format == EUI_FONT_FORMAT_BDF) {
        draw_bdf_glyph(canvas, font, c, x, y, adv_out);
#if EUI_FONT_ENABLE_U8G2
    } else if (font->format == EUI_FONT_FORMAT_U8G2) {
        draw_u8g2_glyph(canvas, font, (uint16_t)(uint8_t)c, (uint16_t)(uint8_t)prev, x, y, adv_out);
#endif
    } else {
        if (adv_out) *adv_out = 0;
    }
}

uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str)
{
    if (!canvas || !canvas->font || !str) return 0;

    int16_t cur_x = x;
    const char *s = str;
    uint16_t prev = 0;
    while (*s) {
        uint32_t cp = utf8_decode_next(&s);
        uint8_t adv = 0;
        uint16_t encoding = (uint16_t)(cp > 0xFF ? cp : (cp & 0xFF));
#if EUI_FONT_ENABLE_U8G2
        if (canvas->font->format == EUI_FONT_FORMAT_U8G2 && canvas->font->lookup_glyph && cp > 0xFF) {
            draw_u8g2_glyph(canvas, canvas->font, (uint16_t)cp, 0, cur_x, y, &adv);
        } else {
            draw_glyph(canvas, canvas->font, (char)(encoding & 0xFF), prev, cur_x, y, &adv);
        }
#else
        draw_glyph(canvas, canvas->font, (char)(encoding & 0xFF), prev, cur_x, y, &adv);
#endif
        cur_x += adv;
        prev = encoding;
    }
    return (uint16_t)(cur_x - x);
}

uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      eui_align_t h_align, eui_align_t v_align, const char *str)
{
    if (!canvas || !canvas->font || !str) return 0;

    const eui_font_t *font = canvas->font;
    uint16_t str_w = eui_font_get_str_width(font, str);
    uint8_t  fh    = eui_font_get_height(font);

    int16_t dx = x, dy = y;

    if (h_align & EUI_ALIGN_CENTER)  dx = x - (int16_t)(str_w / 2);
    if (h_align & EUI_ALIGN_RIGHT)   dx = x - (int16_t)str_w;
    if (v_align & EUI_ALIGN_MIDDLE)  dy = y - (int16_t)(fh / 2);
    if (v_align & EUI_ALIGN_BOTTOM)  dy = y - (int16_t)fh;

    return eui_canvas_draw_str(canvas, dx, dy, str);
}

uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str)
{
    if (!canvas || !canvas->font || !str) return 0;
#if EUI_FONT_ENABLE_U8G2
    if (canvas->font->format == EUI_FONT_FORMAT_U8G2 && canvas->font->lookup_glyph) {
        uint16_t w = 0;
        const char *s = str;
        uint16_t prev = 0;
        while (*s) {
            uint32_t cp = utf8_decode_next(&s);
            uint16_t encoding = (uint16_t)(cp > 0xFF ? cp : (cp & 0xFF));
            int32_t idx = canvas->font->lookup_glyph(canvas->font, encoding, prev);
            if (idx >= 0) {
                u8g2_glyph_t g;
                if (decode_glyph_at(canvas->font, (uint32_t)idx, &g)) {
                    w += g.x_advance;
                }
            }
            prev = encoding;
        }
        return w;
    }
#endif
    return eui_font_get_str_width(canvas->font, str);
}

uint16_t eui_canvas_font_height(const eui_canvas_t *canvas)
{
    if (!canvas) return 0;
    return eui_font_get_height(canvas->font);
}

/* ---- Images ---- */

void eui_canvas_draw_xbm(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint8_t *data)
{
    if (!canvas || !data || w == 0 || h == 0) return;
    uint16_t bytes_per_row = (w + 7) / 8;
    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            uint16_t byte_off = row * bytes_per_row + (col / 8);
            uint8_t bit = 7 - (col % 8);
            if (data[byte_off] & (1u << bit)) {
                canvas_set_pixel(canvas, x + (int16_t)col, y + (int16_t)row, canvas->fg_color);
            }
        }
    }
}

void eui_canvas_draw_bitmap(eui_canvas_t *canvas, int16_t x, int16_t y, const eui_bitmap_t *bmp)
{
    if (!bmp || !bmp->data) return;
    uint8_t depth = bmp->color_depth;
    if (depth == 0) depth = 1;
    uint16_t bytes_per_pixel = (depth + 7) / 8;
    uint16_t row_bytes = bmp->width * bytes_per_pixel;
    const uint8_t *src = bmp->data;

    for (uint16_t row = 0; row < bmp->height; row++) {
        for (uint16_t col = 0; col < bmp->width; col++) {
            uint32_t pixel_raw = 0;
            uint16_t src_off = row * row_bytes + col * bytes_per_pixel;
            for (uint8_t b = 0; b < bytes_per_pixel; b++) {
                pixel_raw = (pixel_raw << 8) | src[src_off + b];
            }
            eui_color_t color;
            if (depth == 4) {
                color = (eui_color_t)(pixel_raw & 0x0Fu);
            } else if (depth == 2) {
                color = (eui_color_t)(pixel_raw & 3u);
            } else if (bytes_per_pixel == 1) {
                color = (pixel_raw & 1) ? EUI_COLOR_WHITE : EUI_COLOR_BLACK;
            } else if (bytes_per_pixel == 2) {
                color = (eui_color_t)(pixel_raw & 0xFFFF);
            } else {
                color = (eui_color_t)(pixel_raw & 0xFFFF);
            }
            canvas_set_pixel(canvas, x + (int16_t)col, y + (int16_t)row, color);
        }
    }
}

/* ---- Advanced ---- */

void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    if (!canvas || !canvas->buffer || w == 0 || h == 0) return;
    uint16_t screen_w = eui_canvas_width(canvas);
    uint16_t screen_h = eui_canvas_height(canvas);

    int16_t ex = x + (int16_t)w;
    int16_t ey = y + (int16_t)h;
    if (ex > (int16_t)screen_w) ex = (int16_t)screen_w;
    if (ey > (int16_t)screen_h) ey = (int16_t)screen_h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

#if EUI_COLOR_DEPTH == 1
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            uint16_t byte_idx = (uint16_t)(yi * (int16_t)(screen_w / 8) + (xi / 8));
            uint8_t bit_pos = (uint8_t)(xi % 8);
            canvas->buffer[byte_idx] ^= (1u << bit_pos);
        }
    }
#elif EUI_COLOR_DEPTH == 2
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            uint16_t byte_idx = (uint16_t)(yi * (int16_t)(screen_w / 4u) + (xi / 4u));
            uint8_t shift = 6u - 2u * (uint8_t)(xi % 4u);
            canvas->buffer[byte_idx] ^= (3u << shift);
        }
    }
#elif EUI_COLOR_DEPTH == 4
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            uint16_t byte_idx = (uint16_t)(yi * (int16_t)(screen_w / 2u) + (xi / 2u));
            if (xi & 1) {
                canvas->buffer[byte_idx] ^= 0x0Fu;
            } else {
                canvas->buffer[byte_idx] ^= 0xF0u;
            }
        }
    }
#elif EUI_COLOR_DEPTH == 8
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            canvas->buffer[yi * screen_w + xi] ^= 0xFF;
        }
    }
#elif EUI_COLOR_DEPTH == 16
    uint16_t *buf16 = (uint16_t*)canvas->buffer;
    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            buf16[yi * screen_w + xi] ^= 0xFFFF;
        }
    }
#endif
}

/* ---- Commit ---- */

void eui_canvas_commit(eui_canvas_t *canvas)
{
    if (!canvas || !canvas->display) return;

    if (canvas->display->caps.buffer_mode & EUI_BUFFER_DIRECT) {
        return;
    }

    if (canvas->display->caps.buffer_mode & EUI_BUFFER_PAGE) {
        eui_rect_t band_rect;
        band_rect.x = 0;
        band_rect.y = 0;
        band_rect.w = canvas->display->caps.width;
        band_rect.h = CANVAS_PAGE_BAND_HEIGHT;

        canvas->page_current = 0;
        canvas->page_total = (canvas->display->caps.height + CANVAS_PAGE_BAND_HEIGHT - 1) / CANVAS_PAGE_BAND_HEIGHT;

        for (uint8_t p = 0; p < canvas->page_total; p++) {
            canvas->page_y_offset = (uint16_t)p * CANVAS_PAGE_BAND_HEIGHT;
            band_rect.y = canvas->page_y_offset;
            if (band_rect.y + band_rect.h > canvas->display->caps.height) {
                band_rect.h = canvas->display->caps.height - band_rect.y;
            }
            if (canvas->display->write_buffer) {
                canvas->display->write_buffer(canvas->buffer, &band_rect, canvas->display->user_data);
            }
        }
    } else {
        eui_rect_t full_rect;
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = canvas->display->caps.width;
        full_rect.h = canvas->display->caps.height;
        if (canvas->display->write_buffer) {
            canvas->display->write_buffer(canvas->buffer, &full_rect, canvas->display->user_data);
        }
    }
}

/* ---- PAGE mode ---- */

bool eui_canvas_begin_page(eui_canvas_t *canvas)
{
    if (!canvas) return false;
    if (!(canvas->display->caps.buffer_mode & EUI_BUFFER_PAGE)) return false;

    canvas->page_current = 0;
    canvas->page_total = (canvas->display->caps.height + CANVAS_PAGE_BAND_HEIGHT - 1) / CANVAS_PAGE_BAND_HEIGHT;
    canvas->page_y_offset = 0;
    if (canvas->buffer) {
#if EUI_COLOR_DEPTH == 1
        memset(canvas->buffer, canvas->bg_color ? 0xFF : 0x00, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 2
        uint8_t fill = (uint8_t)(canvas->bg_color & 3u);
        fill = (uint8_t)(fill | (fill << 2) | (fill << 4) | (fill << 6));
        memset(canvas->buffer, fill, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 4
        uint8_t fill = (uint8_t)(canvas->bg_color & 0x0Fu);
        fill = (uint8_t)(fill | (fill << 4));
        memset(canvas->buffer, fill, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 16
        uint16_t *buf16 = (uint16_t *)canvas->buffer;
        size_t pixels = (size_t)canvas->buf_width * canvas->buf_height;
        for (size_t i = 0; i < pixels; i++) buf16[i] = canvas->bg_color;
#else
        memset(canvas->buffer, (uint8_t)canvas->bg_color, canvas_buf_size(canvas));
#endif
    }
    return canvas->page_total > 0;
}

bool eui_canvas_next_page(eui_canvas_t *canvas)
{
    if (!canvas) return false;
    if (!(canvas->display->caps.buffer_mode & EUI_BUFFER_PAGE)) return false;

    eui_rect_t band_rect;
    band_rect.x = 0;
    band_rect.y = canvas->page_y_offset;
    band_rect.w = canvas->display->caps.width;
    band_rect.h = CANVAS_PAGE_BAND_HEIGHT;
    if (band_rect.y + band_rect.h > canvas->display->caps.height) {
        band_rect.h = canvas->display->caps.height - band_rect.y;
    }

    if (canvas->display->write_buffer) {
        canvas->display->write_buffer(canvas->buffer, &band_rect, canvas->display->user_data);
    }

    canvas->page_current++;
    if (canvas->page_current >= canvas->page_total) return false;

    canvas->page_y_offset = (uint16_t)canvas->page_current * CANVAS_PAGE_BAND_HEIGHT;
    if (canvas->buffer) {
#if EUI_COLOR_DEPTH == 1
        memset(canvas->buffer, canvas->bg_color ? 0xFF : 0x00, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 2
        uint8_t fill = (uint8_t)(canvas->bg_color & 3u);
        fill = (uint8_t)(fill | (fill << 2) | (fill << 4) | (fill << 6));
        memset(canvas->buffer, fill, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 4
        uint8_t fill = (uint8_t)(canvas->bg_color & 0x0Fu);
        fill = (uint8_t)(fill | (fill << 4));
        memset(canvas->buffer, fill, canvas_buf_size(canvas));
#elif EUI_COLOR_DEPTH == 16
        uint16_t *buf16 = (uint16_t *)canvas->buffer;
        size_t pixels = (size_t)canvas->buf_width * canvas->buf_height;
        for (size_t i = 0; i < pixels; i++) buf16[i] = canvas->bg_color;
#else
        memset(canvas->buffer, (uint8_t)canvas->bg_color, canvas_buf_size(canvas));
#endif
    }
    return true;
}

/* ---- Advanced Text Operations ---- */

uint16_t eui_canvas_draw_str_clipped(eui_canvas_t *canvas,
                                      const eui_rect_t *clip_rect,
                                      int16_t x, int16_t y, const char *str)
{
    if (!canvas || !canvas->font || !str || !clip_rect) return 0;

    eui_rect_t old_clip = canvas->clip;
    canvas->clip = *clip_rect;

    int16_t cur_x = x;
    uint16_t drawn_w = eui_canvas_draw_str(canvas, cur_x, y, str);

    canvas->clip = old_clip;
    return drawn_w;
}

uint16_t eui_canvas_draw_str_ellipsis(eui_canvas_t *canvas,
                                       int16_t x, int16_t y,
                                       const char *str, uint16_t max_width)
{
    if (!canvas || !canvas->font || !str) return 0;

    const eui_font_t *font = canvas->font;
    uint16_t str_w = eui_font_get_str_width(font, str);

    if (str_w <= max_width) {
        return eui_canvas_draw_str(canvas, x, y, str);
    }

    uint16_t ellipsis_w = eui_font_get_str_width(font, "...");
    if (ellipsis_w >= max_width) {
        return eui_canvas_draw_str(canvas, x, y, "...");
    }

    uint16_t avail = max_width - ellipsis_w;
    uint16_t cur_w = 0;
    int16_t cur_x = x;
    char prev = 0;
    const char *s;
    for (s = str; *s; s++) {
        uint8_t cw = eui_font_get_char_width(font, *s);
        if (cur_w + cw > avail) break;
        uint8_t adv = 0;
        draw_glyph(canvas, font, *s, (uint16_t)(uint8_t)prev, cur_x, y, &adv);
        cur_x += adv;
        cur_w += cw;
        prev = *s;
    }
    cur_x += eui_canvas_draw_str(canvas, cur_x, y, "...");
    return (uint16_t)(cur_x - x);
}

#if EUI_FONT_ENABLE_MULTILINE

typedef struct {
    const char *start;
    uint16_t    len;
} text_line_t;

static uint16_t word_wrap(const eui_font_t *font, const char *str,
                           uint16_t max_width,
                           text_line_t *lines, uint16_t max_lines)
{
    uint16_t line_count = 0;
    const char *line_start = str;
    const char *last_space = NULL;
    uint16_t line_w = 0;
    uint16_t line_chars = 0;

    if (!str || !*str) return 0;

    for (const char *s = str; ; s++) {
        if (*s == '\0' || *s == '\n') {
            if (line_count < max_lines) {
                lines[line_count].start = line_start;
                lines[line_count].len = line_chars;
                line_count++;
            }
            if (*s == '\0') break;
            line_start = s + 1;
            line_w = 0;
            line_chars = 0;
            last_space = NULL;
            continue;
        }
        if (*s == '\r') continue;

        uint8_t cw = eui_font_get_char_width(font, *s);
        if (line_w + cw > max_width) {
            if (last_space && last_space > line_start) {
                uint16_t space_offset = (uint16_t)(last_space - line_start);
                if (line_count < max_lines) {
                    lines[line_count].start = line_start;
                    lines[line_count].len = space_offset;
                    line_count++;
                }
                line_start = last_space + 1;
                line_chars = (uint16_t)(s - line_start) + 1;
                line_w = cw;
                for (const char *t = line_start; t < s; t++)
                    line_w += eui_font_get_char_width(font, *t);
                last_space = NULL;
            } else {
                if (line_chars > 0 && line_count < max_lines) {
                    lines[line_count].start = line_start;
                    lines[line_count].len = line_chars;
                    line_count++;
                }
                line_start = s;
                line_w = cw;
                line_chars = 1;
            }
            continue;
        }

        if (*s == ' ') {
            last_space = s;
        }
        line_w += cw;
        line_chars++;
    }
    return line_count;
}

uint16_t eui_canvas_draw_str_multiline(eui_canvas_t *canvas,
                                        const eui_rect_t *rect, const char *str,
                                        uint8_t line_height, eui_align_t h_align)
{
    if (!canvas || !canvas->font || !str || !rect) return 0;

    const eui_font_t *font = canvas->font;
    uint8_t lh = line_height > 0 ? line_height : eui_font_get_height(font);
    text_line_t lines[64];
    uint16_t line_count = word_wrap(font, str, rect->w, lines, 64);

    int16_t y = rect->y;
    uint16_t total_h = 0;
    uint16_t max_w = rect->w;

    for (uint16_t i = 0; i < line_count; i++) {
        uint16_t line_w = 0;
        for (uint16_t j = 0; j < lines[i].len; j++)
            line_w += eui_font_get_char_width(font, lines[i].start[j]);

        int16_t dx = rect->x;
        if (h_align & EUI_ALIGN_CENTER) dx = rect->x + (int16_t)((max_w - line_w) / 2);
        if (h_align & EUI_ALIGN_RIGHT)  dx = rect->x + (int16_t)(max_w - line_w);

        int16_t cx = dx;
        char prevc = 0;
        for (uint16_t j = 0; j < lines[i].len; j++) {
            uint8_t adv = 0;
            draw_glyph(canvas, font, lines[i].start[j], (uint16_t)(uint8_t)prevc, cx, y, &adv);
            cx += adv;
            prevc = lines[i].start[j];
        }

        y += lh;
        total_h += lh;
    }
    return total_h;
}

uint16_t eui_canvas_draw_str_in_rect(eui_canvas_t *canvas,
                                      const eui_rect_t *rect, const char *str,
                                      eui_align_t h_align, eui_align_t v_align)
{
    if (!canvas || !canvas->font || !str || !rect) return 0;

    const eui_font_t *font = canvas->font;
    uint16_t str_w = eui_font_get_str_width(font, str);
    uint8_t  fh = eui_font_get_height(font);

    int16_t dx = rect->x, dy = rect->y;

    if (h_align & EUI_ALIGN_CENTER) dx = rect->x + (int16_t)((rect->w - str_w) / 2);
    if (h_align & EUI_ALIGN_RIGHT)  dx = rect->x + (int16_t)(rect->w - str_w);
    if (v_align & EUI_ALIGN_MIDDLE) dy = rect->y + (int16_t)((rect->h - fh) / 2);
    if (v_align & EUI_ALIGN_BOTTOM) dy = rect->y + (int16_t)(rect->h - fh);

    return eui_canvas_draw_str_clipped(canvas, rect, dx, dy, str);
}

uint16_t eui_canvas_str_multiline_height(const eui_canvas_t *canvas,
                                          const char *str,
                                          uint16_t max_width, uint8_t line_height)
{
    if (!canvas || !canvas->font || !str) return 0;
    const eui_font_t *font = canvas->font;
    uint8_t lh = line_height > 0 ? line_height : eui_font_get_height(font);
    text_line_t lines[64];
    uint16_t line_count = word_wrap(font, str, max_width, lines, 64);
    return line_count * lh;
}

#endif /* EUI_FONT_ENABLE_MULTILINE */
