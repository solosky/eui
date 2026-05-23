#ifndef EUI_WIDGET_SCROLL_H
#define EUI_WIDGET_SCROLL_H

#include "eui/widget/eui_widget.h"

typedef struct {
    eui_widget_t widget;
    uint16_t content_width, content_height;
    int16_t scroll_x, scroll_y;
} eui_scroll_t;

eui_widget_t* eui_scroll_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_scroll_set_content_size(eui_widget_t *scroll, uint16_t cw, uint16_t ch);
void eui_scroll_add_child(eui_widget_t *scroll, eui_widget_t *child);

#endif /* EUI_WIDGET_SCROLL_H */
