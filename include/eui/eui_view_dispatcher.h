#ifndef EUI_VIEW_DISPATCHER_H
#define EUI_VIEW_DISPATCHER_H

#include "eui_view.h"
#include "eui_types.h"
#include "eui/eui_config.h"
#include <stdint.h>

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

void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas, uint32_t (*get_tick_ms)(void));
int  eui_view_dispatcher_add(eui_view_dispatcher_t *vd, uint32_t view_id, eui_view_t *view);
void eui_view_dispatcher_switch_to(eui_view_dispatcher_t *vd, uint32_t view_id, eui_anim_type_t anim);
int  eui_view_dispatcher_push_overlay(eui_view_dispatcher_t *vd, eui_view_t *overlay, eui_anim_type_t anim);
void eui_view_dispatcher_pop_overlay(eui_view_dispatcher_t *vd, eui_anim_type_t anim);
eui_view_t* eui_view_dispatcher_get_active(eui_view_dispatcher_t *vd);
void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd);
void eui_view_dispatcher_send_input(eui_view_dispatcher_t *vd, const eui_event_t *evt);
void eui_view_dispatcher_send_custom(eui_view_dispatcher_t *vd, uint32_t event_id);

#endif /* EUI_VIEW_DISPATCHER_H */
