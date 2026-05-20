#include "eui/eui_view_dispatcher.h"
#include "eui/eui_view.h"
#include "eui/eui_canvas.h"
#include <string.h>

void eui_view_dispatcher_init(eui_view_dispatcher_t *vd, eui_canvas_t *canvas, uint32_t (*get_tick_ms)(void)) {
    memset(vd, 0, sizeof(*vd));
    vd->canvas = canvas;
    vd->get_tick_ms = get_tick_ms;
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
    int found_idx = -1;
    for (int i = 0; i < (int)vd->view_count; i++) {
        if (vd->views[i].id == view_id) {
            found_idx = i;
            break;
        }
    }
    if (found_idx < 0) return;

    /* Finalize any in-progress transition */
    if (vd->transitioning) {
        if (vd->transition_prev_view) {
            eui_view_send_exit(vd->transition_prev_view);
        }
        vd->transitioning = false;
    }

    if (vd->overlay_count > 0) {
        vd->current_view_idx = (uint8_t)found_idx;
        return;
    }

    eui_view_t *old_view = NULL;
    if (vd->view_count > 0 && vd->current_view_idx < vd->view_count) {
        old_view = vd->views[vd->current_view_idx].view;
    }

    if (anim != EUI_ANIM_NONE && old_view && old_view != vd->views[found_idx].view) {
        vd->transitioning = true;
        vd->transition_type = anim;
        vd->transition_prev_view = old_view;
        vd->transition_start_ms = vd->get_tick_ms ? vd->get_tick_ms() : 0;
        vd->current_view_idx = (uint8_t)found_idx;
        eui_view_send_enter(vd->views[vd->current_view_idx].view);
    } else {
        if (old_view) eui_view_send_exit(old_view);
        vd->current_view_idx = (uint8_t)found_idx;
        eui_view_send_enter(vd->views[vd->current_view_idx].view);
        eui_view_send_draw(vd->views[vd->current_view_idx].view, vd->canvas);
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

static void render_transition(eui_view_dispatcher_t *vd) {
    if (!vd->transition_prev_view || !vd->transition_start_ms) return;

    eui_view_t *new_view = eui_view_dispatcher_get_active(vd);
    if (!new_view) return;

    uint32_t now = vd->get_tick_ms ? vd->get_tick_ms() : 0;
    uint32_t elapsed = now - vd->transition_start_ms;
    uint32_t duration = (vd->transition_type == EUI_ANIM_FADE) ? 400 : 300;
    float progress = (float)elapsed / (float)duration;
    if (progress > 1.0f) progress = 1.0f;

    int16_t W = (int16_t)eui_canvas_width(vd->canvas);
    int16_t H = (int16_t)eui_canvas_height(vd->canvas);

    if (vd->transition_type == EUI_ANIM_FADE) {
        /* Wipe: draw old view fully, then new view with growing clip */
        eui_view_send_draw(vd->transition_prev_view, vd->canvas);

        eui_rect_t clip = { 0, 0, (uint16_t)W, (uint16_t)((float)H * progress) };
        eui_canvas_save(vd->canvas);
        eui_canvas_set_clip(vd->canvas, &clip);
        eui_view_send_draw(new_view, vd->canvas);
        eui_canvas_restore(vd->canvas);
    } else {
        /* Slide: offset both views */
        eui_rect_t old_save = vd->transition_prev_view->area;
        eui_rect_t new_save = new_view->area;

        switch (vd->transition_type) {
            case EUI_ANIM_SLIDE_LEFT:
                vd->transition_prev_view->area.x = (int16_t)(-(float)W * progress);
                new_view->area.x = (int16_t)((float)W * (1.0f - progress));
                break;
            case EUI_ANIM_SLIDE_RIGHT:
                vd->transition_prev_view->area.x = (int16_t)((float)W * progress);
                new_view->area.x = (int16_t)(-(float)W * (1.0f - progress));
                break;
            case EUI_ANIM_SLIDE_UP:
                vd->transition_prev_view->area.y = (int16_t)(-(float)H * progress);
                new_view->area.y = (int16_t)((float)H * (1.0f - progress));
                break;
            default:
                break;
        }

        eui_view_send_draw(vd->transition_prev_view, vd->canvas);
        eui_view_send_draw(new_view, vd->canvas);

        vd->transition_prev_view->area = old_save;
        new_view->area = new_save;
    }

    if (progress >= 1.0f) {
        eui_view_send_exit(vd->transition_prev_view);
        vd->transitioning = false;
    }
}

void eui_view_dispatcher_tick(eui_view_dispatcher_t *vd) {
    eui_canvas_clear(vd->canvas);

    if (vd->transitioning) {
        render_transition(vd);
    } else {
        eui_view_t *active = eui_view_dispatcher_get_active(vd);
        if (active) {
            eui_view_send_draw(active, vd->canvas);
        }
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
