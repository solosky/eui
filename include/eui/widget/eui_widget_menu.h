#ifndef EUI_WIDGET_MENU_H
#define EUI_WIDGET_MENU_H

#include "eui/widget/eui_widget.h"
#include "eui/eui_str.h"

/**
 * @brief Menu item activation callback.
 *
 * @param ctx  User context pointer from eui_menu_item_t.
 */
typedef void (*eui_menu_callback_t)(void *ctx);

typedef struct eui_menu_t eui_menu_t;

/**
 * @brief A single entry in a menu.
 *
 * May contain a submenu pointer for hierarchical navigation.
 */
typedef struct eui_menu_item {
    eui_str_t label;                /**< Item display label. */
    eui_menu_callback_t callback;   /**< Activation callback (may be NULL for submenus). */
    void *callback_ctx;             /**< Context for the callback. */
    eui_menu_t *submenu;            /**< Pointer to submenu (NULL if leaf item). */
} eui_menu_item_t;

/** @brief Maximum items per menu level. */
#define EUI_MENU_MAX_ITEMS 32
/** @brief Maximum nesting depth for submenus. */
#define EUI_MENU_MAX_DEPTH 4

/**
 * @brief Menu widget: a hierarchical, scrollable menu.
 *
 * Supports nested submenus, keyboard/encoder navigation, and
 * activation callbacks.  The menu tracks an active submenu for
 * multi-level navigation.
 */
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

/**
 * @brief Create a menu widget.
 *
 * @param x  Left edge.
 * @param y  Top edge.
 * @param w  Width.
 * @param h  Height.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_menu_create(int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Add a menu item to the current menu level.
 *
 * @param menu  Pointer to the menu widget.
 * @param label Item label text (copied internally).
 * @param cb    Activation callback (may be NULL).
 * @return Pointer to the added eui_menu_item_t, or NULL on failure.
 */
eui_menu_item_t* eui_menu_add_item(eui_widget_t *menu, const char *label, eui_menu_callback_t cb);

/**
 * @brief Add a submenu entry to the current menu level.
 *
 * Selecting this item navigates the menu one level deeper.
 *
 * @param menu  Pointer to the menu widget.
 * @param label Submenu label text (copied internally).
 * @return Pointer to the added eui_menu_item_t (with submenu created),
 *         or NULL on failure.
 */
eui_menu_item_t* eui_menu_add_submenu(eui_widget_t *menu, const char *label);

/**
 * @brief Navigate up one level in the menu hierarchy.
 *
 * @param menu  Pointer to the menu widget.
 */
void eui_menu_back(eui_widget_t *menu);

#endif
