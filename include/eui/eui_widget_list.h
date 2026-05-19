#ifndef EUI_WIDGET_LIST_H
#define EUI_WIDGET_LIST_H

#include "eui_widget.h"

typedef void (*eui_list_callback_t)(uint8_t index, void *ctx);

#define EUI_LIST_MAX_ITEMS 32

typedef struct {
    const char         *text;
    const eui_bitmap_t *icon;
} eui_list_item_t;

typedef struct {
    eui_widget_t        widget;
    eui_list_item_t     items[EUI_LIST_MAX_ITEMS];
    uint8_t             item_count;
    uint8_t             selected_index;
    uint8_t             scroll_offset;
    uint16_t            item_height;
    eui_list_callback_t callback;
    void               *callback_ctx;
} eui_list_t;

eui_widget_t* eui_list_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
int  eui_list_add_item(eui_widget_t *list, const char *text, const eui_bitmap_t *icon);
void eui_list_set_selected(eui_widget_t *list, uint8_t index);
uint8_t eui_list_get_selected(const eui_widget_t *list);
void eui_list_set_callback(eui_widget_t *list, eui_list_callback_t cb, void *ctx);
void eui_list_clear(eui_widget_t *list);

#endif
