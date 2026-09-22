#include "eui/eui_scene.h"
#include "eui/eui_view_dispatcher.h"
#include <string.h>

int eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count) {
    if (!sm || !scenes || count > EUI_SCENE_MAX) return -1;
    memcpy(sm->scenes, scenes, count * sizeof(eui_scene_t));
    sm->count = count;
    sm->current = -1;
    sm->previous = -1;
    sm->depth = 0;
    return 0;
}

void eui_scene_manager_attach(eui_scene_manager_t *sm, struct eui_view_dispatcher_t *vd) {
    if (!sm) return;
    sm->dispatcher = vd;
}

static int scene_find(const eui_scene_manager_t *sm, uint32_t scene_id) {
    for (uint8_t i = 0; i < sm->count; i++) {
        if (sm->scenes[i].scene_id == scene_id) return (int)i;
    }
    return -1;
}

/* Tear down every scene above the root, top-down: scene on_exit, then
 * the view EXIT (attached mode goes through pop_overlay, which also
 * re-ENTERs the exposed level — mirroring the app-level teardown the
 * dispatcher itself performs). */
static void teardown_to_root(eui_scene_manager_t *sm) {
    while (sm->depth > 1) {
        eui_scene_t *top = &sm->scenes[sm->stack[sm->depth - 1]];
        if (top->on_exit) top->on_exit(top->context);
        if (sm->dispatcher) {
            eui_view_dispatcher_pop_overlay(sm->dispatcher, EUI_ANIM_NONE);
        } else {
            if (top->view) eui_view_send_exit(top->view);
            eui_scene_t *exp = &sm->scenes[sm->stack[sm->depth - 2]];
            if (exp->view) eui_view_send_enter(exp->view);
        }
        sm->depth--;
    }
}

void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id) {
    if (!sm || sm->count == 0) return;
    int target = scene_find(sm, scene_id);
    if (target < 0) return;

    teardown_to_root(sm);

    /* No-op when the root scene is already the target. */
    if (sm->depth == 1 && (int)sm->stack[0] == target) {
        sm->current = (int8_t)target;
        return;
    }

    /* Publish the previous root before running the lifecycle callbacks:
     * on_enter of the target scene must observe the root it is coming from
     * (e.g. app-level fresh-entry checks read previous_id() there). The
     * value after the switch completes is unchanged. */
    sm->previous = (sm->depth > 0) ? (int8_t)sm->stack[0] : -1;

    eui_scene_t *cur = (sm->depth > 0) ? &sm->scenes[sm->stack[0]] : NULL;
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

    sm->stack[0] = (uint8_t)target;
    sm->depth = 1;
    sm->current = (int8_t)target;
}

int eui_scene_manager_push(eui_scene_manager_t *sm, uint32_t scene_id) {
    if (!sm || sm->count == 0 || sm->depth == 0) return -1;
    if (sm->depth >= EUI_SCENE_STACK_MAX) return -1;
    if (sm->dispatcher && sm->dispatcher->overlay_count >= EUI_MAX_OVERLAYS) return -1;
    int target = scene_find(sm, scene_id);
    if (target < 0) return -1;

    eui_scene_t *cur = &sm->scenes[sm->stack[sm->depth - 1]];
    eui_scene_t *next = &sm->scenes[target];

    if (cur->on_exit) cur->on_exit(cur->context);
    if (next->on_enter) next->on_enter(next->context);

    if (sm->dispatcher) {
        /* push_overlay fires EXIT on the previously active view (the
         * scene below) and ENTER on the pushed scene's view. */
        if (eui_view_dispatcher_push_overlay(sm->dispatcher, next->view, EUI_ANIM_NONE) != 0)
            return -1;
    } else {
        if (cur->view) eui_view_send_exit(cur->view);
        if (next->view) eui_view_send_enter(next->view);
    }

    sm->stack[sm->depth++] = (uint8_t)target;
    sm->current = (int8_t)target;
    return 0;
}

int eui_scene_manager_pop(eui_scene_manager_t *sm) {
    if (!sm || sm->depth <= 1) return -1;

    eui_scene_t *top = &sm->scenes[sm->stack[sm->depth - 1]];
    sm->depth--;
    eui_scene_t *exp = &sm->scenes[sm->stack[sm->depth - 1]];

    if (top->on_exit) top->on_exit(top->context);

    if (sm->dispatcher) {
        /* pop_overlay fires EXIT on the top view and ENTER on the
         * newly exposed one. */
        eui_view_dispatcher_pop_overlay(sm->dispatcher, EUI_ANIM_NONE);
    } else {
        if (top->view) eui_view_send_exit(top->view);
        if (exp->view) eui_view_send_enter(exp->view);
    }

    if (exp->on_resume) exp->on_resume(exp->context);

    sm->current = (int8_t)sm->stack[sm->depth - 1];
    return 0;
}

void eui_scene_manager_back(eui_scene_manager_t *sm) {
    (void)eui_scene_manager_pop(sm);
}

uint8_t eui_scene_manager_depth(const eui_scene_manager_t *sm) {
    return sm ? sm->depth : 0;
}

uint32_t eui_scene_manager_current_id(const eui_scene_manager_t *sm) {
    if (!sm || sm->depth == 0) return (uint32_t)-1;
    return sm->scenes[sm->stack[sm->depth - 1]].scene_id;
}

bool eui_scene_manager_send_event(eui_scene_manager_t *sm, uint32_t event_id, int32_t arg) {
    if (!sm || sm->depth == 0 || sm->current < 0) return false;
    eui_scene_t *top = &sm->scenes[sm->stack[sm->depth - 1]];
    if (!top->on_event) return false;
    return top->on_event(top->context, event_id, arg);
}

uint32_t eui_scene_manager_previous_id(const eui_scene_manager_t *sm) {
    if (!sm || sm->previous < 0 || sm->previous >= (int8_t)sm->count) return (uint32_t)-1;
    return sm->scenes[sm->previous].scene_id;
}

const eui_scene_t *eui_scene_manager_scene_at(const eui_scene_manager_t *sm, uint8_t level) {
    if (!sm || level >= sm->depth) return NULL;
    return &sm->scenes[sm->stack[level]];
}
