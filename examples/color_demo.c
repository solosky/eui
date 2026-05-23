#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/driver/eui_drv_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 240
#define H 240
#define POOL_SIZE 262144
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

static void color_draw(eui_widget_t *w, eui_canvas_t *c) {
    int cy;

    /* Background: dark blue gradient bands */
    for (cy = 0; cy < H; cy += 8) {
        uint8_t b = (uint8_t)(cy * 60 / H);
        eui_canvas_set_color(c, eui_color_from_rgb(0, 0, b));
        eui_canvas_fill_rect(c, 0, cy, W, 8);
    }

    /* Color bars */
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

    /* Colored shapes */
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

    /* Colored text */
    eui_canvas_set_font(c, &eui_font_builtin);

    eui_canvas_set_color(c, eui_color_from_rgb(255, 255, 0));
    eui_canvas_draw_str(c, 6, 164, "Color Demo");

    eui_canvas_set_color(c, eui_color_from_rgb(0, 255, 255));
    eui_canvas_draw_str(c, 6, 178, "16-bit RGB565");

    eui_canvas_set_color(c, eui_color_from_rgb(180, 120, 255));
    eui_canvas_draw_str(c, 6, 192, "Hello Color!");

    /* Draw heart icon in pink */
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

static bool color_input(eui_widget_t *w, const eui_event_t *e) { (void)w;(void)e; return false; }
static eui_widget_vtable_t color_vt = { .draw=color_draw, .input=color_input };

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_drv_t *display = eui_drv_raylib_create_display(W, H, 16);
    eui_input_drv_t *input = eui_drv_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t color_widget;
    eui_widget_init(&color_widget, &color_vt, 0, 0, W, H);

    eui_view_dispatcher_add(vd, 1, &color_widget.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    while (!eui_drv_raylib_window_should_close()) {
        eui_tick();
        eui_drv_raylib_refresh();
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(input);
    eui_drv_raylib_destroy_display(display);
    return 0;
}
