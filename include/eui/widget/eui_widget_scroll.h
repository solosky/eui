#ifndef EUI_WIDGET_SCROLL_H
#define EUI_WIDGET_SCROLL_H

#include "eui/widget/eui_widget.h"

/**
 * @brief Scrollable container widget.
 *
 * Provides a viewport onto virtual content that may be larger than
 * the widget's visible area.  Supports horizontal and vertical
 * scrolling via input events (keys, encoder, or touch drag).
 */
typedef struct {
    eui_widget_t widget;
    uint16_t content_width;   /**< Virtual content width in pixels. */
    uint16_t content_height;  /**< Virtual content height in pixels. */
    int16_t scroll_x;         /**< Current horizontal scroll offset. */
    int16_t scroll_y;         /**< Current vertical scroll offset. */
} eui_scroll_t;

/**
 * @brief Create a scrollable container widget.
 *
 * @param x  Left edge.
 * @param y  Top edge.
 * @param w  Width of the visible viewport.
 * @param h  Height of the visible viewport.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_scroll_create(int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Set the virtual content size for scroll extent calculation.
 *
 * The scroll range is computed as (content_size - viewport_size).
 *
 * @param scroll  Pointer to the scroll widget.
 * @param cw      Content width in pixels.
 * @param ch      Content height in pixels.
 */
void eui_scroll_set_content_size(eui_widget_t *scroll, uint16_t cw, uint16_t ch);

/**
 * @brief Add a child widget to the scroll container.
 *
 * The child is clipped to the scroll viewport and offset by the
 * current scroll position during rendering.
 *
 * @param scroll  Pointer to the scroll widget.
 * @param child   Child widget to add.
 */
void eui_scroll_add_child(eui_widget_t *scroll, eui_widget_t *child);

#endif /* EUI_WIDGET_SCROLL_H */
