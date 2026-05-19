#ifndef EUI_WIDGET_SLIDER_H
#define EUI_WIDGET_SLIDER_H

#include "eui/eui_widget.h"

typedef struct {
    eui_widget_t widget;
    int16_t value, min, max;
} eui_slider_t;

eui_widget_t* eui_slider_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_slider_set_range(eui_widget_t *slider, int16_t min, int16_t max);
void eui_slider_set_value(eui_widget_t *slider, int16_t value);
int16_t eui_slider_get_value(const eui_widget_t *slider);

#endif /* EUI_WIDGET_SLIDER_H */
