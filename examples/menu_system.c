#include "eui/eui.h"
#include "eui/driver/eui_drv_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

static void on_brightness(void *ctx) { printf("[MENU] Brightness\n"); (void)ctx; }
static void on_volume(void *ctx)     { printf("[MENU] Volume\n"); (void)ctx; }
static void on_about(void *ctx)      { printf("[MENU] About\n"); (void)ctx; }
static void on_quit(void *ctx)       { printf("[MENU] Quit\n"); (void)ctx; }

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    eui_display_hal_t *display = eui_drv_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_drv_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();

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

    eui_view_dispatcher_add(vd, 1, &menu->view);
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
