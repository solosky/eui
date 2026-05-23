#ifndef EUI_WIDGET_MENU_H
#define EUI_WIDGET_MENU_H

#include "eui/widget/eui_widget.h"

typedef void (*eui_menu_callback_t)(void *ctx);

typedef struct eui_menu_t eui_menu_t;

typedef struct eui_menu_item {
    const char *label;
    eui_menu_callback_t callback;
    void *callback_ctx;
    eui_menu_t *submenu;
} eui_menu_item_t;

#define EUI_MENU_MAX_ITEMS 32
#define EUI_MENU_MAX_DEPTH 4

struct eui_menu_t {
    eui_widget_t    widget;
    eui_menu_item_t items[EUI_MENU_MAX_ITEMS];
    uint8_t         item_count;
    uint8_t         selected_index;
    uint8_t         scroll_offset;
    eui_menu_t     *parent_menu;
    eui_menu_t     *active_submenu;
    uint8_t         item_height;
};

eui_widget_t* eui_menu_create(int16_t x, int16_t y, uint16_t w, uint16_t h);
eui_menu_item_t* eui_menu_add_item(eui_widget_t *menu, const char *label, eui_menu_callback_t cb);
eui_menu_item_t* eui_menu_add_submenu(eui_widget_t *menu, const char *label);
void eui_menu_back(eui_widget_t *menu);

#endif
