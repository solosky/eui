#include "eui/eui_scene.h"
#include <string.h>

int eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count) {
    if (!sm || !scenes || count > EUI_SCENE_MAX) return -1;
    memcpy(sm->scenes, scenes, count * sizeof(eui_scene_t));
    sm->count = count;
    sm->current = -1;
    sm->previous = -1;
    return 0;
}

void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id) {
    if (!sm || sm->count == 0) return;
    int target = -1;
    for (uint8_t i = 0; i < sm->count; i++) {
        if (sm->scenes[i].scene_id == scene_id) { target = i; break; }
    }
    if (target < 0 || target == sm->current) return;

    /* Exit current scene */
    if (sm->current >= 0 && sm->current < (int8_t)sm->count) {
        eui_scene_t *cur = &sm->scenes[sm->current];
        if (cur->on_exit) cur->on_exit(NULL);
        if (cur->view) eui_view_send_exit(cur->view);
    }

    /* Enter new scene */
    eui_scene_t *next = &sm->scenes[target];
    if (next->view) eui_view_send_enter(next->view);
    if (next->on_enter) next->on_enter(NULL);

    sm->previous = sm->current;
    sm->current = target;
}

void eui_scene_manager_back(eui_scene_manager_t *sm) {
    if (!sm || sm->previous < 0) return;
    eui_scene_manager_switch(sm, sm->scenes[sm->previous].scene_id);
}
