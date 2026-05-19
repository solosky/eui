#include "eui/eui_anim.h"
#include <string.h>

typedef struct {
    eui_widget_t *target;
    eui_anim_target_t prop;
    mc_animate_t anim;
    eui_anim_handle_t handle;
    bool active;
    void (*on_done)(void *ctx);
    void *ctx;
} eui_anim_slot_t;

static eui_anim_slot_t g_anim_slots[EUI_MAX_ANIMATIONS];
static eui_anim_handle_t g_next_handle = 0;
static bool g_anim_inited = false;

void eui_anim_init(void) {
    if (g_anim_inited) return;
    memset(g_anim_slots, 0, sizeof(g_anim_slots));
    g_anim_inited = true;
}

static eui_anim_slot_t* find_free_slot(void) {
    for (int i = 0; i < EUI_MAX_ANIMATIONS; i++) {
        if (!g_anim_slots[i].active) return &g_anim_slots[i];
    }
    return NULL;
}

static eui_anim_slot_t* find_slot_by_handle(eui_anim_handle_t handle) {
    if (handle == 0) return NULL;
    for (int i = 0; i < EUI_MAX_ANIMATIONS; i++) {
        if (g_anim_slots[i].active && g_anim_slots[i].handle == handle) {
            return &g_anim_slots[i];
        }
    }
    return NULL;
}

static int16_t get_widget_prop(eui_widget_t *w, eui_anim_target_t prop) {
    switch (prop) {
        case EUI_ANIM_TARGET_X:      return w->area.x;
        case EUI_ANIM_TARGET_Y:      return w->area.y;
        case EUI_ANIM_TARGET_WIDTH:  return (int16_t)w->area.w;
        case EUI_ANIM_TARGET_HEIGHT: return (int16_t)w->area.h;
        default: return 0;
    }
}

static void set_widget_prop(eui_widget_t *w, eui_anim_target_t prop, int16_t val) {
    switch (prop) {
        case EUI_ANIM_TARGET_X:      w->area.x = val; break;
        case EUI_ANIM_TARGET_Y:      w->area.y = val; break;
        case EUI_ANIM_TARGET_WIDTH:  w->area.w = (uint16_t)val; break;
        case EUI_ANIM_TARGET_HEIGHT: w->area.h = (uint16_t)val; break;
        default: break;
    }
    w->style |= EUI_STYLE_DIRTY;
    eui_view_mark_dirty(&w->view);
}

static eui_anim_handle_t alloc_handle(void) {
    g_next_handle++;
    if (g_next_handle == 0) g_next_handle = 1;
    return g_next_handle;
}

eui_anim_handle_t eui_anim_start(eui_widget_t *target,
                                  eui_anim_target_t prop,
                                  int16_t from, int16_t to,
                                  uint16_t duration_ms,
                                  mc_easing_fn_t easing,
                                  void *ctx,
                                  void (*on_done)(void *ctx)) {
    if (!target) return 0;

    eui_anim_slot_t *slot = find_free_slot();
    if (!slot) return 0;

    mc_animate_init_easing(&slot->anim,
        MC_FP_C((float)from), MC_FP_C((float)to),
        MC_FP_C((float)duration_ms / 1000.0f));
    if (easing) slot->anim.config.easing.easing = easing;

    slot->target   = target;
    slot->prop     = prop;
    slot->active   = true;
    slot->on_done  = on_done;
    slot->ctx      = ctx;
    slot->handle   = alloc_handle();

    return slot->handle;
}

eui_anim_handle_t eui_anim_start_spring(eui_widget_t *target,
                                         eui_anim_target_t prop,
                                         int16_t to,
                                         float stiffness,
                                         float damping,
                                         void (*on_done)(void *ctx)) {
    if (!target) return 0;

    eui_anim_slot_t *slot = find_free_slot();
    if (!slot) return 0;

    int16_t from = get_widget_prop(target, prop);

    mc_animate_init_spring(&slot->anim,
        MC_FP_C((float)from), MC_FP_C((float)to));

    if (stiffness > 0.0f) slot->anim.config.spring.stiffness = MC_FP_C(stiffness);
    if (damping > 0.0f)   slot->anim.config.spring.damping   = MC_FP_C(damping);

    slot->target   = target;
    slot->prop     = prop;
    slot->active   = true;
    slot->on_done  = on_done;
    slot->ctx      = (void*)target;
    slot->handle   = alloc_handle();

    return slot->handle;
}

void eui_anim_update(uint32_t delta_ms) {
    for (int i = 0; i < EUI_MAX_ANIMATIONS; i++) {
        eui_anim_slot_t *slot = &g_anim_slots[i];
        if (!slot->active || !slot->target) continue;

        bool done = mc_animate_update(&slot->anim, MC_FP_C((float)delta_ms / 1000.0f));
        int16_t val = MC_REAL_TO_INT(slot->anim.current);
        set_widget_prop(slot->target, slot->prop, val);

        if (done) {
            slot->active = false;
            if (slot->on_done) slot->on_done(slot->ctx);
        }
    }
}

void eui_anim_stop(eui_anim_handle_t handle) {
    eui_anim_slot_t *slot = find_slot_by_handle(handle);
    if (slot) slot->active = false;
}

void eui_anim_stop_all(eui_widget_t *target) {
    for (int i = 0; i < EUI_MAX_ANIMATIONS; i++) {
        if (g_anim_slots[i].target == target) {
            g_anim_slots[i].active = false;
        }
    }
}

bool eui_anim_is_running(eui_anim_handle_t handle) {
    return find_slot_by_handle(handle) != NULL;
}
