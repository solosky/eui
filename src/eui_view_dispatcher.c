#include "eui/eui_view_dispatcher.h"
#include "eui/eui_view.h"
#include "eui/eui_canvas.h"
#include <string.h>

void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas) {
    memset(vd, 0, sizeof(*vd));
    vd->canvas = canvas;
    vd->running = false;
}

int eui_view_dispatcher_add(eui_view_dispatcher_t *vd, uint32_t view_id, eui_view_t *view) {
    if (vd->view_count >= EUI_MAX_VIEWS) return -1;
    vd->views[vd->view_count].id = view_id;
    vd->views[vd->view_count].view = view;
    vd->view_count++;
    return 0;
}

eui_view_t* eui_view_dispatcher_get_active(eui_view_dispatcher_t *vd) {
    if (vd->overlay_count > 0) {
        return vd->overlays[vd->overlay_count - 1];
    }
    if (vd->view_count > 0 && vd->current_view_idx < vd->view_count) {
        return vd->views[vd->current_view_idx].view;
    }
    return NULL;
}

void eui_view_dispatcher_switch_to(eui_view_dispatcher_t *vd, uint32_t view_id, eui_anim_type_t anim) {
    (void)anim;

    int found_idx = -1;
    for (int i = 0; i < (int)vd->view_count; i++) {
        if (vd->views[i].id == view_id) {
            found_idx = i;
            break;
        }
    }
    if (found_idx < 0) return;

    if (vd->overlay_count == 0) {
        if (vd->view_count > 0 && vd->current_view_idx < vd->view_count) {
            eui_view_send_exit(vd->views[vd->current_view_idx].view);
        }
        vd->current_view_idx = (uint8_t)found_idx;
        eui_view_send_enter(vd->views[vd->current_view_idx].view);
        eui_view_send_draw(vd->views[vd->current_view_idx].view, vd->canvas);
    } else {
        vd->current_view_idx = (uint8_t)found_idx;
    }
}

int eui_view_dispatcher_push_overlay(eui_view_dispatcher_t *vd, eui_view_t *overlay, eui_anim_type_t anim) {
    (void)anim;
    if (vd->overlay_count >= EUI_MAX_OVERLAYS) return -1;

    eui_view_t *active = eui_view_dispatcher_get_active(vd);
    if (active) {
        eui_view_send_exit(active);
    }

    vd->overlays[vd->overlay_count] = overlay;
    vd->overlay_count++;

    eui_view_send_enter(overlay);
    return 0;
}

void eui_view_dispatcher_pop_overlay(eui_view_dispatcher_t *vd, eui_anim_type_t anim) {
    (void)anim;
    if (vd->overlay_count == 0) return;

    eui_view_t *top = vd->overlays[vd->overlay_count - 1];
    eui_view_send_exit(top);

    vd->overlay_count--;

    eui_view_t *new_active = eui_view_dispatcher_get_active(vd);
    if (new_active) {
        eui_view_send_enter(new_active);
        if (vd->overlay_count == 0) {
            eui_view_send_draw(new_active, vd->canvas);
        }
    }
}

void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd) {
    eui_view_t *active = eui_view_dispatcher_get_active(vd);
    if (active) {
        eui_view_send_draw(active, vd->canvas);
    }
    eui_canvas_commit(vd->canvas);
}

void eui_view_dispatcher_send_input(eui_view_dispatcher_t *vd, const eui_event_t *evt) {
    eui_view_t *active = eui_view_dispatcher_get_active(vd);
    if (!active) return;

    bool handled = eui_view_send_input(active, evt);

    if (!handled && vd->overlay_count > 0) {
        eui_view_t *base = NULL;
        if (vd->overlay_count > 1) {
            base = vd->overlays[vd->overlay_count - 2];
        } else {
            if (vd->view_count > 0 && vd->current_view_idx < vd->view_count) {
                base = vd->views[vd->current_view_idx].view;
            }
        }
        if (base) {
            eui_view_send_input(base, evt);
        }
    }
}

void eui_view_dispatcher_send_custom(eui_view_dispatcher_t *vd, uint32_t event_id) {
    eui_view_t *active = eui_view_dispatcher_get_active(vd);
    if (!active || !active->handler) return;

    eui_view_event_t event = { .type = EUI_VIEW_EVT_CUSTOM };
    event.event.custom.id = event_id;
    event.event.custom.data = NULL;
    active->handler(&event, active->context);
}
