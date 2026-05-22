#ifndef EUI_WIDGET_LABEL_H
#define EUI_WIDGET_LABEL_H

#include "eui_widget.h"

typedef struct {
    eui_widget_t       widget;
    const char        *text;
    const eui_font_t  *font;
    eui_align_t        h_align;
    eui_align_t        v_align;
} eui_label_t;

eui_widget_t* eui_label_create(const char *text, int16_t x, int16_t y);
void eui_label_set_text(eui_widget_t *label, const char *text);
void eui_label_set_font(eui_widget_t *label, const eui_font_t *font);
void eui_label_set_align(eui_widget_t *label, eui_align_t h, eui_align_t v);

#endif
