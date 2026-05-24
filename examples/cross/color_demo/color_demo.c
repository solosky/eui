/* examples/cross/color_demo/color_demo.c */
#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_port_bootstrap.h"

static void color_draw(eui_widget_t *w, eui_canvas_t *c) {
    int16_t cy;
    int16_t W = w->area.w;
    int16_t H = w->area.h;

    for (cy = 0; cy < H; cy += 8) {
        uint8_t b = (uint8_t)(cy * 60 / H);
        eui_canvas_set_color(c, eui_color_from_rgb(0, 0, b));
        eui_canvas_fill_rect(c, 0, cy, W, 8);
    }

    for (cy = 0; cy < W; cy++) {
        eui_canvas_set_color(c, eui_color_from_rgb(cy * 255 / W, 0, 0));
        eui_canvas_draw_dot(c, cy, 6);
        eui_canvas_set_color(c, eui_color_from_rgb(0, cy * 255 / W, 0));
        eui_canvas_draw_dot(c, cy, 14);
        eui_canvas_set_color(c, eui_color_from_rgb(0, 0, cy * 255 / W));
        eui_canvas_draw_dot(c, cy, 22);
        eui_canvas_set_color(c, eui_color_from_rgb(cy * 255 / W, cy * 255 / W, cy * 255 / W));
        eui_canvas_draw_dot(c, cy, 30);
    }

    eui_canvas_set_color(c, eui_color_from_rgb(255, 80, 80));
    eui_canvas_fill_rect(c, 6, 44, 50, 50);

    eui_canvas_set_color(c, eui_color_from_rgb(80, 255, 80));
    eui_canvas_fill_circle(c, 125, 69, 26);

    eui_canvas_set_color(c, eui_color_from_rgb(80, 80, 255));
    eui_canvas_draw_triangle(c, 195, 104, 220, 48, 234, 104);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 200, 0));
    eui_canvas_fill_round_rect(c, 6, 110, 90, 36, 6);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 0, 255));
    eui_canvas_draw_rect(c, 120, 104, 48, 48);

    eui_canvas_set_font(c, &eui_font_builtin);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 255, 0));
    eui_canvas_draw_str(c, 6, 164, "Color Demo");

    eui_canvas_set_color(c, eui_color_from_rgb(0, 255, 255));
    eui_canvas_draw_str(c, 6, 178, "16-bit RGB565");

    eui_canvas_set_color(c, eui_color_from_rgb(180, 120, 255));
    eui_canvas_draw_str(c, 6, 192, "Hello Color!");

    eui_canvas_set_color(c, eui_color_from_rgb(255, 100, 150));
    static const uint8_t heart_icon[] = {
        0x00, 0x00, 0x00,
        0x03, 0x00, 0xC0,
        0x0F, 0x00, 0xF0,
        0xFF, 0xFF, 0xFF,
        0x7F, 0xFF, 0xFE,
        0x3F, 0xFF, 0xFC,
        0x1F, 0xFF, 0xF8,
        0x0F, 0xFF, 0xF0,
        0x00, 0x00, 0x00,
    };
    eui_canvas_draw_xbm(c, 108, 210, 24, 9, heart_icon);
}

static bool color_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t color_vt = { .draw = color_draw, .input = color_input };

void eui_example_setup(const eui_example_config_t *cfg) {
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t *widget = eui_malloc(sizeof(eui_widget_t));
    eui_widget_init(widget, &color_vt, 0, 0, cfg->display_width, cfg->display_height);

    eui_view_dispatcher_add(vd, 1, &widget->view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);
}
