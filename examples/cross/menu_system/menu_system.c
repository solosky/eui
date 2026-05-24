/* examples/cross/menu_system/menu_system.c */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>

static void on_brightness(void *ctx) { printf("[MENU] Brightness\n"); (void)ctx; }
static void on_volume(void *ctx)     { printf("[MENU] Volume\n"); (void)ctx; }
static void on_about(void *ctx)      { printf("[MENU] About\n"); (void)ctx; }
static void on_quit(void *ctx)       { printf("[MENU] Quit\n"); (void)ctx; }

void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;
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
}
