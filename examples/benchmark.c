#include "eui/eui.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define W 128
#define H 64
#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static uint8_t mock_buf[W * H / 8];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud) {
    (void)ud;
    int bpr = r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (W / 8) + r->x / 8),
               b + row * bpr, bpr);
}

static eui_display_hal_t mock_display = {
    .caps = { .width = W, .height = H, .color_depth = 1, .buffer_mode = EUI_BUFFER_FULL },
    .init = NULL, .deinit = NULL, .write_buffer = mock_write_buffer,
    .user_data = NULL
};

static int mock_poll(eui_event_t *e, void *d) { (void)e; (void)d; return 0; }
static eui_input_hal_t mock_input = {
    .poll = mock_poll, .user_data = NULL
};

static void stub_draw(eui_widget_t *w, eui_canvas_t *c) {
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_fill_rect(c, w->area.x, w->area.y, w->area.w, w->area.h);
}

static bool stub_input(eui_widget_t *w, const eui_event_t *e) { (void)w; (void)e; return false; }
static eui_widget_vtable_t stub_vt = { .draw = stub_draw, .input = stub_input };

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t scene;
    eui_widget_init(&scene, &stub_vt, 0, 0, 128, 64);

    eui_widget_t kids[10];
    for (int i = 0; i < (int)(sizeof(kids)/sizeof(kids[0])); i++) {
        eui_widget_init(&kids[i], &stub_vt,
                         (int16_t)((i * 12) % 100), (int16_t)((i * 8) % 50), 16, 10);
        eui_widget_add_child(&scene, &kids[i]);
    }

    eui_view_dispatcher_add(&vd, 1, &scene.view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);

    int frames = 300;
    clock_t start = clock();
    for (int i = 0; i < frames; i++) {
        eui_view_dispatcher_tick(&vd);
    }
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    double fps = elapsed > 0 ? frames / elapsed : 0;

    printf("[PASS] benchmark (%d frames in %.3fs = %.0f FPS)\n", frames, elapsed, fps);
    eui_canvas_destroy(canvas);
    return 0;
}
