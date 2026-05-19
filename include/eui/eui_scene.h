#ifndef EUI_SCENE_H
#define EUI_SCENE_H

#include "eui_view.h"
#include <stdint.h>

typedef void (*eui_scene_on_enter_t)(void *context);
typedef void (*eui_scene_on_exit_t)(void *context);
typedef bool (*eui_scene_on_event_t)(void *context, uint32_t event_id);

typedef struct {
    uint32_t scene_id;
    eui_view_t *view;
    eui_scene_on_enter_t on_enter;
    eui_scene_on_exit_t  on_exit;
    eui_scene_on_event_t on_event;
} eui_scene_t;

#define EUI_SCENE_MAX 16

typedef struct {
    eui_scene_t scenes[EUI_SCENE_MAX];
    uint8_t count;
    int8_t current;
    int8_t previous;
} eui_scene_manager_t;

int  eui_scene_manager_register(eui_scene_manager_t *sm, const eui_scene_t *scenes, uint8_t count);
void eui_scene_manager_switch(eui_scene_manager_t *sm, uint32_t scene_id);
void eui_scene_manager_back(eui_scene_manager_t *sm);

#endif /* EUI_SCENE_H */
