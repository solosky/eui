#ifndef EUI_VIEW_DISPATCHER_H
#define EUI_VIEW_DISPATCHER_H

#include "eui_view.h"
#include "eui_types.h"
#include "eui/eui_config.h"
#include <stdint.h>

/**
 * @brief Central view routing and overlay manager.
 *
 * The view dispatcher owns a set of registered views (identified by
 * numeric IDs) and a stack of overlay views on top of the active view.
 * It manages view transitions, input routing, and redraw scheduling.
 */
typedef struct eui_view_dispatcher_t {
    struct {
        uint32_t    id;
        eui_view_t *view;
    } views[EUI_MAX_VIEWS];
    uint8_t view_count;
    uint8_t current_view_idx;
    eui_view_t *overlays[EUI_MAX_OVERLAYS];
    uint8_t overlay_count;
    bool running;
    eui_canvas_t *canvas;

    /* Transition animation state */
    bool transitioning;
    eui_anim_type_t transition_type;
    eui_view_t *transition_prev_view;
    uint32_t transition_start_ms;
    uint32_t (*get_tick_ms)(void);
} eui_view_dispatcher_t;

/**
 * @brief Initialize a view dispatcher.
 *
 * @param vd           Pointer to the dispatcher to initialize.
 * @param canvas       The canvas used for rendering.
 * @param get_tick_ms  Callback returning a monotonic millisecond timestamp.
 */
void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas, uint32_t (*get_tick_ms)(void));

/**
 * @brief Register a view with the dispatcher.
 *
 * @param vd      Pointer to the dispatcher.
 * @param view_id Numeric identifier for the view (must be unique).
 * @param view    Pointer to the eui_view_t to register.
 * @return 0 on success, or a negative error code if the view table is full.
 */
int  eui_view_dispatcher_add(eui_view_dispatcher_t *vd, uint32_t view_id, eui_view_t *view);

/**
 * @brief Switch to a registered view by ID.
 *
 * If the view is already active, this is a no-op.
 *
 * @param vd      Pointer to the dispatcher.
 * @param view_id Numeric ID of the target view.
 * @param anim    Transition animation to use (use EUI_ANIM_NONE for instant).
 */
void eui_view_dispatcher_switch_to(eui_view_dispatcher_t *vd, uint32_t view_id, eui_anim_type_t anim);

/**
 * @brief Push an overlay view on top of the current view.
 *
 * The overlay receives input and draw events before the underlying view.
 *
 * @param vd      Pointer to the dispatcher.
 * @param overlay Pointer to the overlay view.
 * @param anim    Transition animation for the overlay entrance.
 * @return 0 on success, or a negative error code if the overlay stack is full.
 */
int  eui_view_dispatcher_push_overlay(eui_view_dispatcher_t *vd, eui_view_t *overlay, eui_anim_type_t anim);

/**
 * @brief Pop (remove) the topmost overlay view.
 *
 * @param vd   Pointer to the dispatcher.
 * @param anim Transition animation for the overlay exit.
 */
void eui_view_dispatcher_pop_overlay(eui_view_dispatcher_t *vd, eui_anim_type_t anim);

/**
 * @brief Get the currently active view.
 *
 * If overlays are present, returns the topmost overlay;
 * otherwise returns the current main view.
 *
 * @param vd Pointer to the dispatcher.
 * @return Pointer to the active eui_view_t, or NULL if none.
 */
eui_view_t* eui_view_dispatcher_get_active(eui_view_dispatcher_t *vd);

/**
 * @brief Perform per-frame update (redraw + transition logic).
 *
 * Should be called once per frame from the main loop.
 *
 * @param vd Pointer to the dispatcher.
 */
void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd);

/**
 * @brief Send an input event to the active view (or its overlays).
 *
 * The event is first offered to the topmost overlay; if unhandled,
 * it falls through to the main view.
 *
 * @param vd  Pointer to the dispatcher.
 * @param evt Pointer to the input event.
 */
void eui_view_dispatcher_send_input(eui_view_dispatcher_t *vd, const eui_event_t *evt);

/**
 * @brief Send a custom user-defined event to the active view.
 *
 * @param vd       Pointer to the dispatcher.
 * @param event_id Application-defined event identifier.
 */
void eui_view_dispatcher_send_custom(eui_view_dispatcher_t *vd, uint32_t event_id);

#endif /* EUI_VIEW_DISPATCHER_H */
