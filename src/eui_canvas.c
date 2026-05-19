#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include <string.h>
#include <stdlib.h>

#define CANVAS_PAGE_BAND_HEIGHT 8

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
#elif EUI_COLOR_DEPTH == 8
    return pixels;
#elif EUI_COLOR_DEPTH == 16
    return pixels * 2;
#else
    return pixels / 8;
#endif
}

/* ---- Lifecycle ---- */

eui_canvas_t* eui_canvas_create(eui_display_hal_t *display)
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
#else
    /* For 8bpp/16bpp, fill with bg_color */
    if (canvas->bg_color == 0) {
        memset(canvas->buffer, 0, size);
    } else {
        /* Fill with bg_color byte by byte for simplicity */
        for (size_t i = 0; i < size; i++) {
            canvas->buffer[i] = (uint8_t)canvas->bg_color;
        }
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

        canvas_set_pixel(canvas, l + cx, t - cy, canvas->fg_color);
        canvas_set_pixel(canvas, ri - cx, t - cy, canvas->fg_color);
        canvas_set_pixel(canvas, l + cx, b + cy, canvas->fg_color);
        canvas_set_pixel(canvas, ri - cx, b + cy, canvas->fg_color);

        canvas_set_pixel(canvas, l + cy, t - cx, canvas->fg_color);
        canvas_set_pixel(canvas, ri - cy, t - cx, canvas->fg_color);
        canvas_set_pixel(canvas, l + cy, b + cx, canvas->fg_color);
        canvas_set_pixel(canvas, ri - cy, b + cx, canvas->fg_color);
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

/* ---- Text (stubs) ---- */

void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font)
{
    if (!canvas) return;
    canvas->font = font;
}

uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str)
{
    (void)canvas; (void)x; (void)y; (void)str;
    return 0;
}

uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      eui_align_t h_align, eui_align_t v_align, const char *str)
{
    (void)canvas; (void)x; (void)y; (void)h_align; (void)v_align; (void)str;
    return 0;
}

uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str)
{
    (void)canvas; (void)str;
    return 0;
}

uint16_t eui_canvas_font_height(const eui_canvas_t *canvas)
{
    (void)canvas;
    return 0;
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
    for (uint16_t row = 0; row < bmp->height; row++) {
        for (uint16_t col = 0; col < bmp->width; col++) {
            canvas_set_pixel(canvas, x + (int16_t)col, y + (int16_t)row, EUI_COLOR_BLACK);
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

    for (int16_t yi = y; yi < ey; yi++) {
        for (int16_t xi = x; xi < ex; xi++) {
            uint16_t byte_idx = (uint16_t)(yi * (int16_t)(screen_w / 8) + (xi / 8));
            uint8_t bit_pos = (uint8_t)(xi % 8);
            canvas->buffer[byte_idx] ^= (1u << bit_pos);
        }
    }
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
        memset(canvas->buffer, canvas->bg_color ? 0xFF : 0x00, canvas_buf_size(canvas));
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
        memset(canvas->buffer, canvas->bg_color ? 0xFF : 0x00, canvas_buf_size(canvas));
    }
    return true;
}
