#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

static void square_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool square_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t square_vt = { .draw = square_draw, .input = square_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t square;
    eui_widget_init(&square, &square_vt, 0, 30, 16, 16);

    eui_view_dispatcher_add(vd, 1, &square.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    eui_anim_init();
    eui_anim_start(&square, EUI_ANIM_TARGET_X, 0, 80, 500, mc_ease_linear, NULL, NULL);
}
