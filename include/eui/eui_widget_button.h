#ifndef EUI_WIDGET_BUTTON_H
#define EUI_WIDGET_BUTTON_H

#include "eui_widget.h"
#include "eui_types.h"

typedef void (*eui_button_callback_t)(void *ctx);

typedef struct {
    eui_widget_t widget;
    const char  *label;
    const eui_bitmap_t *icon;
    eui_button_callback_t callback;
    void *callback_ctx;
} eui_button_t;

eui_widget_t* eui_button_create(const char *label, int16_t x, int16_t y,
                                 uint16_t w, uint16_t h);
void eui_button_set_callback(eui_widget_t *btn, eui_button_callback_t cb, void *ctx);
void eui_button_set_bitmap(eui_widget_t *btn, const eui_bitmap_t *bmp);

#endif
