#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/hal/eui_hal_raylib.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define W 128
#define H 64
#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

enum { SCENE_MAIN = 1, SCENE_RED, SCENE_BLUE, SCENE_GREEN };
static eui_view_dispatcher_t *g_vd;
static eui_scene_manager_t g_sm;

/* ---- Scene data ---- */
typedef struct {
    const char *name;
    eui_color_t bg_color;
    void (*draw_pattern)(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h);
    eui_anim_type_t anim_in;
    eui_anim_type_t anim_out;
} scene_data_t;

static void draw_vertical(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t px = x + 4; px < x + w; px += 8)
        eui_canvas_draw_line(c, px, y + 2, px, y + h - 2);
}
static void draw_diagonal(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t px = x; px < x + w; px += 6)
        eui_canvas_draw_line(c, px, y, px + 6, y + h);
}
static void draw_horizontal(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t py = y + 2; py < y + h; py += 6)
        eui_canvas_draw_line(c, x + 2, py, x + w - 2, py);
}

static scene_data_t g_red_data   = { "Red Scene",   0,   draw_vertical,   EUI_ANIM_SLIDE_LEFT, EUI_ANIM_SLIDE_RIGHT };
static scene_data_t g_blue_data  = { "Blue Scene",  0,   draw_diagonal,   EUI_ANIM_SLIDE_UP,   EUI_ANIM_SLIDE_RIGHT };
static scene_data_t g_green_data = { "Green Scene", 0,   draw_horizontal, EUI_ANIM_FADE,       EUI_ANIM_SLIDE_RIGHT };

/* ---- Main menu state ---- */
typedef struct { const char *name; uint32_t scene_id; eui_anim_type_t anim; } menu_item_t;
static menu_item_t g_menu_items[] = {
    { "1. Red   (SLIDE_LEFT)", SCENE_RED,   EUI_ANIM_SLIDE_LEFT },
    { "2. Blue  (SLIDE_UP)",   SCENE_BLUE,  EUI_ANIM_SLIDE_UP   },
    { "3. Green (FADE)",       SCENE_GREEN, EUI_ANIM_FADE       },
};
#define MENU_COUNT 3
static int g_menu_sel = 0;

/* ---- Coordinate scene manager + dispatcher ---- */
static void navigate_to(uint32_t scene_id, eui_anim_type_t anim) {
    if (g_sm.current >= 0 && g_sm.current < (int8_t)g_sm.count) {
        if (g_sm.scenes[g_sm.current].on_exit)
            g_sm.scenes[g_sm.current].on_exit(NULL);
    }
    eui_view_dispatcher_switch_to(g_vd, scene_id, anim);
    for (uint8_t i = 0; i < g_sm.count; i++) {
        if (g_sm.scenes[i].scene_id == scene_id) {
            if (g_sm.scenes[i].on_enter)
                g_sm.scenes[i].on_enter(NULL);
            g_sm.previous = g_sm.current;
            g_sm.current = (int8_t)i;
            return;
        }
    }
}

/* ---- Scene callbacks ---- */
static void on_enter_main(void *ctx) { (void)ctx; printf("[SCENE_MGR] Enter Main\n"); }
static void on_exit_main(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Exit Main\n"); }
static void on_enter_red(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Enter Red\n"); }
static void on_exit_red(void *ctx)   { (void)ctx; printf("[SCENE_MGR] Exit Red\n"); }
static void on_enter_blue(void *ctx) { (void)ctx; printf("[SCENE_MGR] Enter Blue\n"); }
static void on_exit_blue(void *ctx)  { (void)ctx; printf("[SCENE_MGR] Exit Blue\n"); }
static void on_enter_green(void *ctx){ (void)ctx; printf("[SCENE_MGR] Enter Green\n"); }
static void on_exit_green(void *ctx) { (void)ctx; printf("[SCENE_MGR] Exit Green\n"); }

/* ---- View event handlers ---- */
static bool main_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, EUI_COLOR_WHITE);
        eui_canvas_set_font(c, &eui_font_builtin);
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + 2, "Scene+View Demo");
        eui_canvas_draw_line(c, view->area.x + 4, view->area.y + 12,
                                view->area.x + W - 4, view->area.y + 12);
        for (int i = 0; i < MENU_COUNT; i++) {
            int y = view->area.y + 18 + i * 14;
            if (i == g_menu_sel) {
                eui_canvas_fill_rect(c, view->area.x + 4, y, W - 8, 12);
                eui_canvas_set_color(c, EUI_COLOR_BLACK);
                eui_canvas_draw_str(c, view->area.x + 8, y + 2, g_menu_items[i].name);
                eui_canvas_set_color(c, EUI_COLOR_WHITE);
            } else {
                eui_canvas_draw_rect(c, view->area.x + 4, y, W - 8, 12);
                eui_canvas_draw_str(c, view->area.x + 8, y + 2, g_menu_items[i].name);
            }
        }
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + H - 10,
                            "UP/DOWN:Nav  OK:Go");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS) {
            if (e->data.key == EUI_KEY_UP && g_menu_sel > 0) {
                g_menu_sel--; eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_DOWN && g_menu_sel < MENU_COUNT - 1) {
                g_menu_sel++; eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_OK) {
                menu_item_t *item = &g_menu_items[g_menu_sel];
                navigate_to(item->scene_id, item->anim);
                return true;
            }
        }
        return false;
    }
    case EUI_VIEW_EVT_ENTER: printf("[VIEW] Enter Main\n"); return true;
    case EUI_VIEW_EVT_EXIT:  printf("[VIEW] Exit Main\n");  return true;
    default: return false;
    }
}

static bool detail_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    scene_data_t *data = (scene_data_t*)view->model;
    if (!data) return false;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, data->bg_color);
        eui_canvas_fill_rect(c, view->area.x, view->area.y,
                                view->area.w, view->area.h);
        eui_canvas_set_color(c, EUI_COLOR_WHITE);
        if (data->draw_pattern)
            data->draw_pattern(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_draw_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_font(c, &eui_font_builtin);
        uint16_t tw = eui_canvas_str_width(c, data->name);
        eui_canvas_draw_str(c, view->area.x + (W - (int16_t)tw) / 2,
                            view->area.y + H / 2 - 4, data->name);
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + H - 10, "ESC/OK:Back");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS &&
            (e->data.key == EUI_KEY_BACK || e->data.key == EUI_KEY_OK)) {
            navigate_to(SCENE_MAIN, data->anim_out);
            return true;
        }
        return false;
    }
    case EUI_VIEW_EVT_ENTER: printf("[VIEW] Enter %s\n", data->name); return true;
    case EUI_VIEW_EVT_EXIT:  printf("[VIEW] Exit %s\n", data->name);  return true;
    default: return false;
    }
}

/* ---- Main ---- */
int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    g_red_data.bg_color   = eui_color_from_rgb(180, 20, 20);
    g_blue_data.bg_color  = eui_color_from_rgb(20, 40, 180);
    g_green_data.bg_color = eui_color_from_rgb(20, 140, 20);

    eui_display_hal_t *display = eui_hal_raylib_create_display(W, H, EUI_COLOR_DEPTH);
    eui_input_hal_t *input = eui_hal_raylib_create_input();
    eui_config_t cfg = { .display=display, .input=input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    g_vd = eui_get_view_dispatcher();

    /* Create 4 custom views */
    eui_view_t main_view, red_view, blue_view, green_view;

    eui_view_init(&main_view, main_view_handler, &main_view);
    main_view.area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(&red_view, detail_view_handler, &red_view);
    red_view.area = (eui_rect_t){ 0, 0, W, H };
    red_view.model = &g_red_data;

    eui_view_init(&blue_view, detail_view_handler, &blue_view);
    blue_view.area = (eui_rect_t){ 0, 0, W, H };
    blue_view.model = &g_blue_data;

    eui_view_init(&green_view, detail_view_handler, &green_view);
    green_view.area = (eui_rect_t){ 0, 0, W, H };
    green_view.model = &g_green_data;

    /* Register views with dispatcher */
    eui_view_dispatcher_add(g_vd, SCENE_MAIN,  &main_view);
    eui_view_dispatcher_add(g_vd, SCENE_RED,   &red_view);
    eui_view_dispatcher_add(g_vd, SCENE_BLUE,  &blue_view);
    eui_view_dispatcher_add(g_vd, SCENE_GREEN, &green_view);

    /* Register scenes with manager */
    eui_scene_t scenes[4] = {
        { SCENE_MAIN,  &main_view,  on_enter_main,  on_exit_main,  NULL },
        { SCENE_RED,   &red_view,   on_enter_red,   on_exit_red,   NULL },
        { SCENE_BLUE,  &blue_view,  on_enter_blue,  on_exit_blue,  NULL },
        { SCENE_GREEN, &green_view, on_enter_green, on_exit_green, NULL },
    };
    eui_scene_manager_register(&g_sm, scenes, 4);

    /* Switch to initial scene */
    navigate_to(SCENE_MAIN, EUI_ANIM_NONE);

    /* Main loop */
    while (!eui_hal_raylib_window_should_close()) {
        eui_tick();
        eui_hal_raylib_refresh();

        if (IsKeyPressed(KEY_F2)) {
            eui_hal_raylib_save_screenshot("screenshot.ppm");
            printf("Screenshot saved to screenshot.ppm\n");
        }
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_hal_raylib_destroy_input(input);
    eui_hal_raylib_destroy_display(display);
    return 0;
}
