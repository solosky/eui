#ifndef EUI_WIDGET_H
#define EUI_WIDGET_H

#include "eui/eui_view.h"
#include "eui/eui_canvas.h"
#include "eui/eui_config.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Focus policy values for widget focus navigation.
 */
#define EUI_FOCUS_NONE    0 /**< Widget cannot receive focus. */
#define EUI_FOCUS_TAB     1 /**< Widget can receive tab-based focus. */
#define EUI_FOCUS_STRONG  2 /**< Widget has strong (preferred) focus. */

/**
 * @brief Widget style flags (bit-field).
 */
#define EUI_STYLE_VISIBLE   (1u << 0) /**< Widget is visible (receives draw). */
#define EUI_STYLE_ENABLED   (1u << 1) /**< Widget is enabled (receives input). */
#define EUI_STYLE_FOCUSED   (1u << 2) /**< Widget currently has input focus. */
#define EUI_STYLE_SELECTED  (1u << 3) /**< Widget is in selected state. */
#define EUI_STYLE_PRESSED   (1u << 4) /**< Widget is currently pressed. */
#define EUI_STYLE_CHECKED   (1u << 5) /**< Widget is in checked/toggled state. */
#define EUI_STYLE_DIRTY     (1u << 6) /**< Widget needs redrawing. */

typedef struct eui_widget_t eui_widget_t;
typedef struct eui_widget_vtable eui_widget_vtable_t;

/**
 * @brief Virtual function table for widgets.
 *
 * Widget subclasses override these methods to customize behavior.
 */
struct eui_widget_vtable {
    /** @brief Draw the widget onto the canvas. */
    void (*draw)(eui_widget_t *self, eui_canvas_t *canvas);
    /**
     * @brief Handle an input event.
     * @return true if the event was consumed, false to pass through.
     */
    bool (*input)(eui_widget_t *self, const eui_event_t *evt);
    /** @brief Called when the widget gains focus or becomes active. */
    void (*enter)(eui_widget_t *self);
    /** @brief Called when the widget loses focus or becomes inactive. */
    void (*exit)(eui_widget_t *self);
    /** @brief Recalculate widget layout (e.g. after resize). */
    void (*layout)(eui_widget_t *self);
    /** @brief Release widget-specific resources. */
    void (*destroy)(eui_widget_t *self);
};

/**
 * @brief Base widget structure.
 *
 * All concrete widgets embed this as their first member so they can
 * be cast to/from eui_widget_t safely.
 */
struct eui_widget_t {
    eui_view_t view;                            /**< Base view (embedded). */
    eui_rect_t area;                            /**< Widget bounding rectangle. */
    uint32_t   style;                           /**< Style flags (EUI_STYLE_*). */
    uint8_t    focus_policy;                    /**< Focus policy (EUI_FOCUS_*). */
    const eui_widget_vtable_t *vt;             /**< Virtual function table. */
    eui_widget_t *parent;                       /**< Parent widget (NULL for root). */
    eui_widget_t *children[EUI_MAX_WIDGET_CHILDREN]; /**< Child widget array. */
    uint8_t    child_count;                     /**< Number of children. */
    uint8_t    focus_index;                     /**< Index of the focused child. */
};

/**
 * @brief Initialize a widget with its vtable and bounding rectangle.
 *
 * @param w   Pointer to the widget (uninitialized).
 * @param vt  Pointer to the widget's virtual function table.
 * @param x   Left edge.
 * @param y   Top edge.
 * @param ww  Width (avoid name clash with parameter w).
 * @param hh  Height.
 */
void eui_widget_init(eui_widget_t *w, const eui_widget_vtable_t *vt,
                     int16_t x, int16_t y, uint16_t ww, uint16_t hh);

/**
 * @brief Deinitialize a widget and release its resources.
 *
 * Calls the vtable's destroy method and removes it from its parent.
 *
 * @param w  Pointer to the widget.
 */
void eui_widget_deinit(eui_widget_t *w);

/**
 * @brief Add a child widget to a parent.
 *
 * @param parent  Parent widget.
 * @param child   Child widget to add.
 */
void eui_widget_add_child(eui_widget_t *parent, eui_widget_t *child);

/**
 * @brief Remove a child widget from a parent.
 *
 * @param parent  Parent widget.
 * @param child   Child widget to remove.
 */
void eui_widget_remove_child(eui_widget_t *parent, eui_widget_t *child);

/**
 * @brief Get the currently focused widget in the widget tree starting at @p root.
 *
 * @param root  Root of the widget tree to search.
 * @return Pointer to the focused widget, or NULL if none.
 */
eui_widget_t* eui_widget_get_focus(const eui_widget_t *root);

/**
 * @brief Move focus to the next focusable widget in the tree.
 *
 * @param root  Root of the widget tree.
 * @return Pointer to the newly focused widget, or NULL if no focusable widget found.
 */
eui_widget_t* eui_widget_focus_next(eui_widget_t *root);

/**
 * @brief Move focus to the previous focusable widget in the tree.
 *
 * @param root  Root of the widget tree.
 * @return Pointer to the newly focused widget, or NULL if no focusable widget found.
 */
eui_widget_t* eui_widget_focus_prev(eui_widget_t *root);

/**
 * @brief Programmatically set focus to a specific widget.
 *
 * @param w  Widget to focus.
 */
void eui_widget_set_focus(eui_widget_t *w);

/** @brief Check if the widget is visible. */
static inline bool eui_widget_is_visible(const eui_widget_t *w) { return w->style & EUI_STYLE_VISIBLE; }
/** @brief Check if the widget is enabled. */
static inline bool eui_widget_is_enabled(const eui_widget_t *w) { return w->style & EUI_STYLE_ENABLED; }
/** @brief Set the widget's visibility. */
static inline void eui_widget_set_visible(eui_widget_t *w, bool v) { if (v) w->style |= EUI_STYLE_VISIBLE; else w->style &= ~EUI_STYLE_VISIBLE; }
/** @brief Set the widget's enabled state. */
static inline void eui_widget_set_enabled(eui_widget_t *w, bool e) { if (e) w->style |= EUI_STYLE_ENABLED; else w->style &= ~EUI_STYLE_ENABLED; }

/**
 * @brief Cast an eui_view_t pointer to its containing eui_widget_t.
 *
 * Assumes the view is embedded as the first member of the widget.
 */
#define eui_widget_from_view(v) ((eui_widget_t*)(v))

#endif
