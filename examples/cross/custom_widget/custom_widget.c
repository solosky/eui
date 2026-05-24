#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

typedef struct {
    eui_widget_t widget;
    int count;
} counter_t;

static void counter_draw(eui_widget_t *w, eui_canvas_t *c) {
    counter_t *cw = (counter_t*)w;
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_draw_round_rect(c, w->area.x, w->area.y, w->area.w, w->area.h, 4);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", cw->count);
    eui_canvas_draw_str_aligned(c, w->area.x + (int16_t)(w->area.w / 2),
                                  w->area.y + (int16_t)(w->area.h / 2),
                                  EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE, buf);
}

static bool counter_input(eui_widget_t *w, const eui_event_t *evt) {
    if (evt->type == EUI_EVT_KEY_PRESS && evt->data.key == EUI_KEY_OK) {
        ((counter_t*)w)->count++;
        return true;
    }
    return false;
}

static eui_widget_vtable_t counter_vt = {
    .draw = counter_draw,
    .input = counter_input
};

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    counter_t cw;
    memset(&cw, 0, sizeof(cw));
    eui_widget_init(&cw.widget, &counter_vt, 40, 20, 48, 24);
    cw.widget.focus_policy = EUI_FOCUS_STRONG;
    eui_view_dispatcher_add(vd, 1, &cw.widget.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
