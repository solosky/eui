#include "eui/eui_scene.h"
#include "eui/eui_view_dispatcher.h"
#include <string.h>

int eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count) {
    if (!sm || !scenes || count > EUI_SCENE_MAX) return -1;
    memcpy(sm->scenes, scenes, count * sizeof(eui_scene_t));
    sm->count = count;
    sm->current = -1;
    sm->previous = -1;
    return 0;
}

void eui_scene_manager_attach(eui_scene_manager_t *sm, struct eui_view_dispatcher_t *vd) {
    if (!sm) return;
    sm->dispatcher = vd;
}

void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id) {
    if (!sm || sm->count == 0) return;
    int target = -1;
    for (uint8_t i = 0; i < sm->count; i++) {
        if (sm->scenes[i].scene_id == scene_id) { target = i; break; }
    }
    if (target < 0 || target == sm->current) return;

    eui_scene_t *cur = (sm->current >= 0 && sm->current < (int8_t)sm->count)
                           ? &sm->scenes[sm->current] : NULL;
    eui_scene_t *next = &sm->scenes[target];

    if (sm->dispatcher) {
        /* Attached mode: the view change is routed through the dispatcher
         * so its routing state (and transition animations) stay in sync —
         * it fires the views' EXIT/ENTER events itself. The scene manager
         * only runs the scene-level callbacks, with on_enter just before
         * the dispatcher makes the new view active so scene state is
         * fresh for the dispatcher's immediate draw. */
        if (cur && cur->on_exit) cur->on_exit(cur->context);
        if (next->on_enter) next->on_enter(next->context);
        eui_view_dispatcher_switch_to(sm->dispatcher, next->scene_id, EUI_ANIM_NONE);
    } else {
        /* Standalone mode: drive the views' ENTER/EXIT events directly. */
        if (cur) {
            if (cur->on_exit) cur->on_exit(cur->context);
            if (cur->view) eui_view_send_exit(cur->view);
        }

        if (next->view) eui_view_send_enter(next->view);
        if (next->on_enter) next->on_enter(next->context);
    }

    sm->previous = sm->current;
    sm->current = target;
}

void eui_scene_manager_back(eui_scene_manager_t *sm) {
    if (!sm || sm->previous < 0) return;
    eui_scene_manager_switch(sm, sm->scenes[sm->previous].scene_id);
}
