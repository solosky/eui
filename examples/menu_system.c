#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

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

static int last_leaf_hit = 0;

static void on_brightness(void *ctx) { last_leaf_hit = 1; (void)ctx; }
static void on_volume(void *ctx)     { last_leaf_hit = 2; (void)ctx; }
static void on_about(void *ctx)      { last_leaf_hit = 3; (void)ctx; }
static void on_quit(void *ctx)       { last_leaf_hit = 4; (void)ctx; }

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas);

    eui_widget_t *menu = eui_menu_create(0, 0, 128, 64);
    menu->focus_policy = EUI_FOCUS_STRONG;

    eui_menu_item_t *settings = eui_menu_add_submenu(menu, "Settings");
    eui_menu_add_item(menu, "About", on_about);
    eui_menu_add_item(menu, "Exit", on_quit);

    if (settings && settings->submenu) {
        eui_widget_t *sub = (eui_widget_t*)settings->submenu;
        eui_menu_add_item(sub, "Brightness", on_brightness);
        eui_menu_add_item(sub, "Volume", on_volume);
    }

    eui_view_dispatcher_add(&vd, 1, &menu->view);
    eui_view_dispatcher_switch_to(&vd, 1, EUI_ANIM_NONE);
    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    eui_event_t evt;

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_DOWN;
    eui_view_dispatcher_send_input(&vd, &evt);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_BACK;
    eui_view_dispatcher_send_input(&vd, &evt);
    eui_view_dispatcher_tick(&vd);

    for (int i = 0; i < 3; i++) {
        evt.type = EUI_EVT_KEY_PRESS;
        evt.data.key = EUI_KEY_DOWN;
        eui_view_dispatcher_send_input(&vd, &evt);
    }

    evt.type = EUI_EVT_KEY_PRESS;
    evt.data.key = EUI_KEY_OK;
    eui_view_dispatcher_send_input(&vd, &evt);

    for (int i = 0; i < 3; i++)
        eui_view_dispatcher_tick(&vd);

    if (last_leaf_hit == 0) { printf("[FAIL] No menu action triggered\n"); return 1; }
    printf("[PASS] menu_system (leaf=%d)\n", last_leaf_hit);
    eui_canvas_destroy(canvas);
    return 0;
}
