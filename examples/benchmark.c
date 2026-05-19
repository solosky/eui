#include "eui/eui.h"
#include "eui/hal/eui_hal_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 8192
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

static void scene_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool scene_input(eui_widget_t *w, const eui_event_t *e) {
    (void)w; (void)e;
    return false;
}

static eui_widget_vtable_t scene_vt = { .draw = scene_draw, .input = scene_input };

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, 1);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

    eui_widget_t scene;
    eui_widget_init(&scene, &scene_vt, 0, 0, 128, 64);
    eui_widget_t kids[10];
    for (int i = 0; i < 10; i++) {
        eui_widget_init(&kids[i], &scene_vt,
                         (int16_t)((i * 12) % 100), (int16_t)((i * 8) % 50), 16, 10);
        eui_widget_add_child(&scene, &kids[i]);
    }
    eui_view_dispatcher_add(vd, 1, &scene.view);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    double start = GetTime();
    int frames = 300;
    for (int i = 0; i < frames; i++) {
        eui_tick();
        eui_hal_raylib_refresh();
        if (eui_hal_raylib_window_should_close())
            break;
    }
    double elapsed = GetTime() - start;
    printf("[BENCH] %d frames in %.3fs = %.0f FPS\n",
           frames, elapsed, elapsed > 0 ? frames / elapsed : 0);

    display->deinit(display->user_data);
    eui_deinit();
    eui_hal_raylib_destroy_input(input);
    eui_hal_raylib_destroy_display(display);
    return 0;
}
