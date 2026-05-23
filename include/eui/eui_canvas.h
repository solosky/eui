#ifndef EUI_CANVAS_H
#define EUI_CANVAS_H

#include "eui/eui_types.h"
#include "eui/eui_display_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Software framebuffer and 2D drawing API.
 *
 * The canvas provides an immediate-mode drawing surface backed by a
 * memory buffer.  All drawing commands operate on this buffer until
 * eui_canvas_commit() flushes the result to the physical display.
 */
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

/**
 * @brief Create a canvas for the given display HAL.
 *
 * Allocates and initializes an internal framebuffer matching the
 * display dimensions and color depth.
 *
 * @param display  Pointer to an initialized eui_display_hal_t.
 * @return A pointer to the new canvas, or NULL on allocation failure.
 */
eui_canvas_t* eui_canvas_create(eui_display_hal_t *display);

/**
 * @brief Destroy a canvas and free its framebuffer and resources.
 *
 * @param canvas  Pointer to the canvas to destroy.
 */
void eui_canvas_destroy(eui_canvas_t *canvas);

/**
 * @brief Reset the canvas to its default drawing state.
 *
 * Clears the internal state (color, clip, font) but does NOT clear
 * the framebuffer.
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_reset(eui_canvas_t *canvas);

/**
 * @brief Flush the canvas framebuffer to the physical display.
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_commit(eui_canvas_t *canvas);

/* Properties */

/**
 * @brief Get the canvas width in pixels.
 *
 * @param canvas  Pointer to the canvas.
 * @return Width in pixels.
 */
uint16_t eui_canvas_width(const eui_canvas_t *canvas);

/**
 * @brief Get the canvas height in pixels.
 *
 * @param canvas  Pointer to the canvas.
 * @return Height in pixels.
 */
uint16_t eui_canvas_height(const eui_canvas_t *canvas);

/**
 * @brief Set the foreground drawing color.
 *
 * All subsequent draw operations use this color.
 *
 * @param canvas  Pointer to the canvas.
 * @param color   The foreground color.
 */
void eui_canvas_set_color(eui_canvas_t *canvas, eui_color_t color);

/**
 * @brief Set the background drawing color.
 *
 * Used by text drawing and clear operations.
 *
 * @param canvas  Pointer to the canvas.
 * @param color   The background color.
 */
void eui_canvas_set_bg_color(eui_canvas_t *canvas, eui_color_t color);

/**
 * @brief Set the clipping rectangle.
 *
 * All subsequent drawing operations are constrained to this rectangle.
 * Pixels outside the clip region are discarded.
 *
 * @param canvas  Pointer to the canvas.
 * @param rect    Pointer to the clip rectangle (may be NULL to clear).
 */
void eui_canvas_set_clip(eui_canvas_t *canvas, const eui_rect_t *rect);

/**
 * @brief Remove the clipping constraint (draw to full canvas).
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_clear_clip(eui_canvas_t *canvas);

/**
 * @brief Save the current drawing state onto an internal stack.
 *
 * The saved state includes: clip rectangle, foreground color, and
 * background color.
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_save(eui_canvas_t *canvas);

/**
 * @brief Restore the most recently saved drawing state from the stack.
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_restore(eui_canvas_t *canvas);

/* Drawing primitives */

/**
 * @brief Clear the entire canvas (fill with the background color).
 *
 * @param canvas  Pointer to the canvas.
 */
void eui_canvas_clear(eui_canvas_t *canvas);

/**
 * @brief Draw a single pixel at the given coordinates.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       X-coordinate.
 * @param y       Y-coordinate.
 */
void eui_canvas_draw_dot(eui_canvas_t *canvas, int16_t x, int16_t y);

/**
 * @brief Draw a line between two points using Bresenham's algorithm.
 *
 * @param canvas  Pointer to the canvas.
 * @param x1      X-coordinate of the start point.
 * @param y1      Y-coordinate of the start point.
 * @param x2      X-coordinate of the end point.
 * @param y2      Y-coordinate of the end point.
 */
void eui_canvas_draw_line(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

/**
 * @brief Draw a rectangle outline (1-pixel border).
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 */
void eui_canvas_draw_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Draw a filled (solid) rectangle.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 */
void eui_canvas_fill_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Draw a circle outline.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Center X.
 * @param y       Center Y.
 * @param r       Radius in pixels.
 */
void eui_canvas_draw_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);

/**
 * @brief Draw a filled (solid) circle.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Center X.
 * @param y       Center Y.
 * @param r       Radius in pixels.
 */
void eui_canvas_fill_circle(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t r);

/**
 * @brief Draw a triangle outline connecting the three vertices.
 *
 * @param canvas  Pointer to the canvas.
 * @param x1..y3  Coordinates of the three vertices.
 */
void eui_canvas_draw_triangle(eui_canvas_t *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3);

/**
 * @brief Draw a rounded rectangle outline.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 * @param r       Corner radius in pixels.
 */
void eui_canvas_draw_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);

/**
 * @brief Draw a filled rounded rectangle.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 * @param r       Corner radius in pixels.
 */
void eui_canvas_fill_round_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r);

/* Text */

/**
 * @brief Set the active font for text drawing operations.
 *
 * @param canvas  Pointer to the canvas.
 * @param font    Pointer to the font to use.
 */
void eui_canvas_set_font(eui_canvas_t *canvas, const eui_font_t *font);

/**
 * @brief Draw a null-terminated string at the given position.
 *
 * The text is drawn using the current foreground color and active font.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       X-coordinate of the text baseline start.
 * @param y       Y-coordinate of the text baseline.
 * @param str     Null-terminated string to draw.
 * @return The pixel width consumed by the string (for cursor advancement).
 */
uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str);

/**
 * @brief Draw a string with horizontal and vertical alignment.
 *
 * The alignment is applied relative to the bounding box that starts
 * at (x, y) and extends to the right/down.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Anchor X.
 * @param y       Anchor Y.
 * @param h_align Horizontal alignment (LEFT, CENTER, RIGHT).
 * @param v_align Vertical alignment (TOP, MIDDLE, BOTTOM).
 * @param str     Null-terminated string to draw.
 * @return The pixel width consumed by the string.
 */
uint16_t eui_canvas_draw_str_aligned(eui_canvas_t *canvas, int16_t x, int16_t y,
                                      eui_align_t h_align, eui_align_t v_align, const char *str);

/**
 * @brief Get the pixel width of a string as it would be rendered.
 *
 * @param canvas  Pointer to the canvas.
 * @param str     Null-terminated string to measure.
 * @return String width in pixels.
 */
uint16_t eui_canvas_str_width(const eui_canvas_t *canvas, const char *str);

/**
 * @brief Get the current font height.
 *
 * @param canvas  Pointer to the canvas.
 * @return Font height in pixels (0 if no font set).
 */
uint16_t eui_canvas_font_height(const eui_canvas_t *canvas);

/* Advanced text */
#if EUI_FONT_ENABLE_MULTILINE

/**
 * @brief Draw a string constrained to a rectangle, with alignment.
 *
 * Text that exceeds the rectangle width is wrapped.  The total height
 * of the drawn text may exceed the rectangle (it is not clipped here).
 *
 * @param canvas  Pointer to the canvas.
 * @param rect    Bounding rectangle for text layout.
 * @param str     Null-terminated string to draw.
 * @param h_align Horizontal alignment.
 * @param v_align Vertical alignment.
 * @return The actual pixel height consumed by the text.
 */
uint16_t eui_canvas_draw_str_in_rect(eui_canvas_t *canvas,
                                      const eui_rect_t *rect, const char *str,
                                      eui_align_t h_align, eui_align_t v_align);

/**
 * @brief Draw a multiline string inside a rectangle.
 *
 * Each line has the specified line height.  Wrapping occurs at word
 * boundaries when possible.
 *
 * @param canvas      Pointer to the canvas.
 * @param rect        Bounding rectangle.
 * @param str         Null-terminated string to draw.
 * @param line_height Height per line in pixels.
 * @param h_align     Horizontal alignment per line.
 * @return The actual pixel height consumed.
 */
uint16_t eui_canvas_draw_str_multiline(eui_canvas_t *canvas,
                                        const eui_rect_t *rect, const char *str,
                                        uint8_t line_height, eui_align_t h_align);

/**
 * @brief Calculate the height needed to render a multiline string.
 *
 * @param canvas     Pointer to the canvas.
 * @param str        Null-terminated string to measure.
 * @param max_width  Available width in pixels.
 * @param line_height Height per line in pixels.
 * @return The required height in pixels.
 */
uint16_t eui_canvas_str_multiline_height(const eui_canvas_t *canvas,
                                          const char *str,
                                          uint16_t max_width, uint8_t line_height);
#endif

/**
 * @brief Draw a string clipped to an arbitrary rectangle.
 *
 * Unlike the global clip, this clips only this one text operation.
 *
 * @param canvas    Pointer to the canvas.
 * @param clip_rect Clipping rectangle for this string.
 * @param x         X-coordinate of the text baseline.
 * @param y         Y-coordinate of the text baseline.
 * @param str       Null-terminated string to draw.
 * @return The pixel width consumed by the visible portion.
 */
uint16_t eui_canvas_draw_str_clipped(eui_canvas_t *canvas,
                                      const eui_rect_t *clip_rect,
                                      int16_t x, int16_t y, const char *str);

/**
 * @brief Draw a string with ellipsis if it exceeds the maximum width.
 *
 * If the full string does not fit within @p max_width, it is truncated
 * and "..." is appended.
 *
 * @param canvas    Pointer to the canvas.
 * @param x         X-coordinate of the text baseline.
 * @param y         Y-coordinate of the text baseline.
 * @param str       Null-terminated string to draw.
 * @param max_width Maximum allowed pixel width.
 * @return The actual pixel width consumed (may be 0 if nothing fits).
 */
uint16_t eui_canvas_draw_str_ellipsis(eui_canvas_t *canvas,
                                       int16_t x, int16_t y,
                                       const char *str, uint16_t max_width);

/* Images */

/**
 * @brief Draw an XBM-format bitmap (1-bit per pixel).
 *
 * XBM data is stored as a byte array where each bit represents one pixel.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width in pixels.
 * @param h       Height in pixels.
 * @param data    XBM bitmap data array.
 */
void eui_canvas_draw_xbm(eui_canvas_t *canvas, int16_t x, int16_t y,
                          uint16_t w, uint16_t h, const uint8_t *data);

/**
 * @brief Draw an eui_bitmap_t at the given position.
 *
 * The bitmap can be any color depth supported by the canvas.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param bmp     Pointer to the eui_bitmap_t descriptor.
 */
void eui_canvas_draw_bitmap(eui_canvas_t *canvas, int16_t x, int16_t y, const eui_bitmap_t *bmp);

/* Advanced */

/**
 * @brief Invert (XOR) all pixel values within a rectangle.
 *
 * @param canvas  Pointer to the canvas.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 */
void eui_canvas_invert_rect(eui_canvas_t *canvas, int16_t x, int16_t y, uint16_t w, uint16_t h);

/* PAGE mode control */

/**
 * @brief Begin a paginated rendering pass.
 *
 * In PAGE buffer mode, the framebuffer only covers a portion of the
 * display.  Call this before drawing, then iterate with
 * eui_canvas_next_page().
 *
 * @param canvas  Pointer to the canvas.
 * @return true if there are pages to render, false if the display does
 *         not require paging.
 *
 * @see eui_canvas_next_page()
 */
bool eui_canvas_begin_page(eui_canvas_t *canvas);

/**
 * @brief Advance to the next page in a paginated rendering pass.
 *
 * @param canvas  Pointer to the canvas.
 * @return true if more pages remain, false if all pages have been rendered.
 *
 * @see eui_canvas_begin_page()
 */
bool eui_canvas_next_page(eui_canvas_t *canvas);

#endif /* EUI_CANVAS_H */
