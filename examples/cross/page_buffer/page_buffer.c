#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_display_drv_t *display = eui_get_display();
    if (!display) return;

    (void)cfg;
    eui_canvas_t *canvas = eui_canvas_create(display);

    eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
    eui_canvas_clear(canvas);
    eui_canvas_fill_rect(canvas, 10, 10, 40, 30);
    eui_canvas_draw_rect(canvas, 60, 15, 50, 30);
    eui_canvas_draw_circle(canvas, 30, 45, 10);
    eui_canvas_commit(canvas);

    eui_canvas_destroy(canvas);
}
