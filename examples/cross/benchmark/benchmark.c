/* examples/cross/benchmark/benchmark.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void scene_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool scene_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t scene_vt = { .draw = scene_draw, .input = scene_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *scene = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(scene, &scene_vt, 0, 0, 128, 64);

    for (int i = 0; i < 10; i++) {
        eui_widget_t *kid = eui_malloc(sizeof(eui_widget_t));
        eui_widget_init(kid, &scene_vt,
                         (int16_t)((i * 12) % 100), (int16_t)((i * 8) % 50), 16, 10);
        eui_widget_add_child(scene, kid);
    }

    eui_view_dispatcher_add(vd, 1, &scene->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
