#ifndef EUI_WIDGET_LIST_H
#define EUI_WIDGET_LIST_H

#include "eui/eui_str.h"
#include "eui/widget/eui_widget.h"

/**
 * @brief List item selection callback.
 *
 * @param index  Index of the selected item.
 * @param ctx    User context pointer.
 */
typedef void (*eui_list_callback_t)(uint8_t index, void *ctx);

/** @brief Maximum number of items a list can hold. */
#define EUI_LIST_MAX_ITEMS 32

/**
 * @brief A single item in a list widget.
 */
typedef struct {
    eui_str_t           text; /**< Item label text. */
    const eui_bitmap_t *icon; /**< Optional icon (may be NULL). */
} eui_list_item_t;

/**
 * @brief List widget: a vertical, scrollable list of selectable items.
 *
 * Supports optional icons, keyboard/encoder navigation, and a
 * selection callback.  The list highlights the currently selected item
 * and can be scrolled when the content overflows.
 */
typedef struct {
    eui_widget_t        widget;
    eui_list_item_t     items[EUI_LIST_MAX_ITEMS];
    uint8_t             item_count;
    uint8_t             selected_index;
    uint8_t             scroll_offset;
    uint16_t            item_height;
    eui_list_callback_t callback;
    void               *callback_ctx;
    int8_t              anim_from;
    uint8_t             anim_rem;
} eui_list_t;

/**
 * @brief Create a list widget.
 *
 * @param x  Left edge.
 * @param y  Top edge.
 * @param w  Width.
 * @param h  Height.
 * @return Pointer to the underlying eui_widget_t, or NULL on failure.
 */
eui_widget_t* eui_list_create(int16_t x, int16_t y, uint16_t w, uint16_t h);

/**
 * @brief Add an item to the list.
 *
 * @param list  Pointer to the list widget.
 * @param text  Item label text (copied internally).
 * @param icon  Optional icon bitmap (may be NULL).
 * @return The index of the newly added item, or -1 if the list is full.
 */
int  eui_list_add_item(eui_widget_t *list, const char *text, const eui_bitmap_t *icon);

/**
 * @brief Programmatically select an item by index.
 *
 * @param list   Pointer to the list widget.
 * @param index  Zero-based item index.
 */
void eui_list_set_selected(eui_widget_t *list, uint8_t index);

/**
 * @brief Get the index of the currently selected item.
 *
 * @param list  Pointer to the list widget.
 * @return The selected index, or 0 if the list is empty.
 */
uint8_t eui_list_get_selected(const eui_widget_t *list);

/**
 * @brief Register the selection callback.
 *
 * @param list  Pointer to the list widget.
 * @param cb    Callback invoked on selection.
 * @param ctx   User context passed to the callback.
 */
void eui_list_set_callback(eui_widget_t *list, eui_list_callback_t cb, void *ctx);

/**
 * @brief Remove all items from the list.
 *
 * @param list  Pointer to the list widget.
 */
void eui_list_clear(eui_widget_t *list);

#endif
