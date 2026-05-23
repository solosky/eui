#ifndef EUI_WIDGET_PROGRESS_H
#define EUI_WIDGET_PROGRESS_H

#include "eui/widget/eui_widget.h"

typedef struct {
    eui_widget_t widget;
    uint8_t value;
    bool indeterminate;
} eui_progress_t;

eui_widget_t* eui_progress_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
void eui_progress_set_value(eui_widget_t *prog, uint8_t percent);
void eui_progress_set_indeterminate(eui_widget_t *prog, bool indet);

#endif /* EUI_WIDGET_PROGRESS_H */
