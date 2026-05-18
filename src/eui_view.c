#include "eui/eui_view.h"
#include <string.h>

void eui_view_init(eui_view_t *view, eui_view_handler_t handler, void *context) {
    memset(view, 0, sizeof(*view));
    view->handler = handler;
    view->context = context;
    view->flags = EUI_VIEW_FLAG_VISIBLE;
}

void eui_view_set_model(eui_view_t *view, void *model) {
    view->model = model;
}

void eui_view_mark_dirty(eui_view_t *view) {
    view->flags |= EUI_VIEW_FLAG_DIRTY;
}

bool eui_view_send_draw(eui_view_t *view, eui_canvas_t *canvas) {
    if (!view->handler) return false;
    eui_view_event_t event = { .type = EUI_VIEW_EVT_DRAW };
    event.event.draw.canvas = canvas;
    event.event.draw.model = view->model;
    view->flags &= ~EUI_VIEW_FLAG_DIRTY;
    return view->handler(&event, view->context);
}

bool eui_view_send_input(eui_view_t *view, const eui_event_t *evt) {
    if (!view->handler) return false;
    eui_view_event_t event = { .type = EUI_VIEW_EVT_INPUT };
    event.event.input.input = evt;
    return view->handler(&event, view->context);
}

bool eui_view_send_enter(eui_view_t *view) {
    if (!view->handler) return false;
    eui_view_event_t event = { .type = EUI_VIEW_EVT_ENTER };
    return view->handler(&event, view->context);
}

bool eui_view_send_exit(eui_view_t *view) {
    if (!view->handler) return false;
    eui_view_event_t event = { .type = EUI_VIEW_EVT_EXIT };
    return view->handler(&event, view->context);
}

bool eui_view_send_navigate(eui_view_t *view, uint32_t nav_id) {
    if (!view->handler) return false;
    eui_view_event_t event = { .type = EUI_VIEW_EVT_NAVIGATE };
    event.event.navigate.nav_id = nav_id;
    return view->handler(&event, view->context);
}
