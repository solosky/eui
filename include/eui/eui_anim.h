#ifndef EUI_ANIM_H
#define EUI_ANIM_H

#include "eui/widget/eui_widget.h"
#include "mc.h"
#include <stdint.h>

typedef uint8_t eui_anim_handle_t;

typedef enum {
    EUI_ANIM_TARGET_X,
    EUI_ANIM_TARGET_Y,
    EUI_ANIM_TARGET_WIDTH,
    EUI_ANIM_TARGET_HEIGHT,
    EUI_ANIM_TARGET_OPACITY,
    EUI_ANIM_TARGET_PROGRESS,
    EUI_ANIM_TARGET_CUSTOM,
} eui_anim_target_t;

void eui_anim_init(void);

eui_anim_handle_t eui_anim_start(eui_widget_t *target,
                                  eui_anim_target_t prop,
                                  int16_t from, int16_t to,
                                  uint16_t duration_ms,
                                  mc_easing_fn_t easing,
                                  void *ctx,
                                  void (*on_done)(void *ctx));

eui_anim_handle_t eui_anim_start_spring(eui_widget_t *target,
                                         eui_anim_target_t prop,
                                         int16_t to,
                                         float stiffness,
                                         float damping,
                                         void (*on_done)(void *ctx));

void eui_anim_stop(eui_anim_handle_t handle);
void eui_anim_stop_all(eui_widget_t *target);
bool eui_anim_is_running(eui_anim_handle_t handle);
void eui_anim_update(uint32_t delta_ms);

#endif /* EUI_ANIM_H */
