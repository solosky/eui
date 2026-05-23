#ifndef EUI_VIEW_H
#define EUI_VIEW_H

#include "eui_types.h"
#include "eui_canvas.h"
#include "eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Identifies the type of event being dispatched to a view handler.
 */
typedef enum {
    EUI_VIEW_EVT_DRAW     = 0,      /**< View should redraw itself. */
    EUI_VIEW_EVT_INPUT    = 1,      /**< Input event (key, touch, encoder). */
    EUI_VIEW_EVT_ENTER    = 2,      /**< View is becoming active. */
    EUI_VIEW_EVT_EXIT     = 3,      /**< View is no longer active. */
    EUI_VIEW_EVT_NAVIGATE = 4,      /**< Navigation action (e.g. sub-screen). */
    EUI_VIEW_EVT_CUSTOM   = 0x1000, /**< User-defined custom event base. */
} eui_view_event_type_t;

/**
 * @brief Event structure passed to the view handler callback.
 */
typedef struct {
    eui_view_event_type_t type; /**< Event type identifier. */
    union {
        struct { eui_canvas_t *canvas; void *model; } draw;     /**< DRAW event payload. */
        struct { const eui_event_t *input; } input;             /**< INPUT event payload. */
        struct { uint32_t nav_id; } navigate;                   /**< NAVIGATE event payload. */
        struct { uint32_t id; void *data; } custom;             /**< CUSTOM event payload. */
    } event;
} eui_view_event_t;

/**
 * @brief View event handler function signature.
 *
 * @param event   Pointer to the incoming event.
 * @param context User-supplied context pointer (set at view creation).
 * @return true if the event was handled, false to pass it through.
 */
typedef bool (*eui_view_handler_t)(eui_view_event_t *event, void *context);

/**
 * @brief Generic view structure.
 *
 * A view represents a single "screen" or UI surface.  All user interaction
 * and rendering for a given screen is handled through the view's handler.
 */
typedef struct eui_view {
    eui_rect_t          area;
    eui_view_handler_t  handler;
    void               *context;
    void               *model;
    uint32_t            flags;
} eui_view_t;

/** @brief View is visible and should receive draw events. */
#define EUI_VIEW_FLAG_VISIBLE   (1u << 0)
/** @brief View content has changed and needs redrawing. */
#define EUI_VIEW_FLAG_DIRTY     (1u << 1)
/** @brief View is currently animated. */
#define EUI_VIEW_FLAG_ANIMATING (1u << 2)

/* Lifecycle helpers */

/**
 * @brief Initialize a view structure.
 *
 * @param view    Pointer to the uninitialized eui_view_t.
 * @param handler Event handler callback.
 * @param context User context pointer passed to the handler.
 */
void eui_view_init(eui_view_t *view, eui_view_handler_t handler, void *context);

/**
 * @brief Attach an arbitrary model pointer to the view.
 *
 * The model is passed along with the DRAW event so the handler can
 * access application state.
 *
 * @param view   Pointer to the view.
 * @param model  Pointer to the model data (may be NULL).
 */
void eui_view_set_model(eui_view_t *view, void *model);

/**
 * @brief Mark the view as needing a redraw.
 *
 * The view will receive a DRAW event during the next eui_tick() cycle.
 *
 * @param view  Pointer to the view.
 */
void eui_view_mark_dirty(eui_view_t *view);

/* Dispatch events */

/**
 * @brief Send a DRAW event to the view.
 *
 * @param view   Pointer to the view.
 * @param canvas The canvas the view should draw onto.
 * @return The handler's return value.
 */
bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas);

/**
 * @brief Send an INPUT event to the view.
 *
 * @param view Pointer to the view.
 * @param evt  Pointer to the input event.
 * @return The handler's return value.
 */
bool eui_view_send_input(eui_view_t *view, const eui_event_t *evt);

/**
 * @brief Send an ENTER event (view just became active / appeared).
 *
 * @param view Pointer to the view.
 * @return The handler's return value.
 */
bool eui_view_send_enter(eui_view_t *view);

/**
 * @brief Send an EXIT event (view is no longer active).
 *
 * @param view Pointer to the view.
 * @return The handler's return value.
 */
bool eui_view_send_exit(eui_view_t *view);

/**
 * @brief Send a NAVIGATE event with a navigation identifier.
 *
 * @param view   Pointer to the view.
 * @param nav_id Application-defined navigation action ID.
 * @return The handler's return value.
 */
bool eui_view_send_navigate(eui_view_t *view, uint32_t nav_id);

#endif /* EUI_VIEW_H */
