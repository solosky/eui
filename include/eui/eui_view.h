#ifndef EUI_VIEW_H
#define EUI_VIEW_H

#include "eui_types.h"
#include "eui_canvas.h"
#include "eui_input_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EUI_VIEW_EVT_DRAW     = 0,
    EUI_VIEW_EVT_INPUT    = 1,
    EUI_VIEW_EVT_ENTER    = 2,
    EUI_VIEW_EVT_EXIT     = 3,
    EUI_VIEW_EVT_NAVIGATE = 4,
    EUI_VIEW_EVT_CUSTOM   = 0x1000,
} eui_view_event_type_t;

typedef struct {
    eui_view_event_type_t type;
    union {
        struct { eui_canvas_t *canvas; void *model; } draw;
        struct { const eui_event_t *input; } input;
        struct { uint32_t nav_id; } navigate;
        struct { uint32_t id; void *data; } custom;
    } event;
} eui_view_event_t;

typedef bool (*eui_view_handler_t)(eui_view_event_t *event, void *context);

typedef struct eui_view {
    eui_rect_t          area;
    eui_view_handler_t  handler;
    void               *context;
    void               *model;
    uint32_t            flags;
} eui_view_t;

/* View flags */
#define EUI_VIEW_FLAG_VISIBLE   (1u << 0)
#define EUI_VIEW_FLAG_DIRTY     (1u << 1)
#define EUI_VIEW_FLAG_ANIMATING (1u << 2)

/* Lifecycle helpers */
void eui_view_init(eui_view_t *view, eui_view_handler_t handler, void *context);
void eui_view_set_model(eui_view_t *view, void *model);
void eui_view_mark_dirty(eui_view_t *view);

/* Dispatch events */
bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas);
bool eui_view_send_input(eui_view_t *view, const eui_event_t *evt);
bool eui_view_send_enter(eui_view_t *view);
bool eui_view_send_exit(eui_view_t *view);
bool eui_view_send_navigate(eui_view_t *view, uint32_t nav_id);

#endif /* EUI_VIEW_H */
