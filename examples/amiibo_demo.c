#include "eui/eui.h"
#include "eui/eui_font_builtin.h"
#include "eui/driver/eui_drv_raylib.h"
#include "amiibo_font.h"
#include "amiibo_icons.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define W 240
#define H 240
#define POOL_SIZE 262144
static uint8_t mem_pool[POOL_SIZE];
static uint32_t get_tick(void) { return (uint32_t)(GetTime() * 1000.0); }

/* ─── Carousel direction ─── */
typedef enum { CAROUSEL_HORIZONTAL, CAROUSEL_VERTICAL } carousel_dir_t;

/* ─── Item type ─── */
typedef struct {
    const char *label;
    const eui_bitmap_t *icon;
    void *user_data;
} item_t;

/* ─── Carousel state ─── */
typedef struct {
    const item_t *items;
    uint8_t count;
    int8_t selected;
    mc_real_t scroll_pos;
    mc_real_t target_pos;
    mc_spring_state_t spring_state;
    mc_spring_params_t spring_params;
    carousel_dir_t dir;
    uint16_t item_step;
    uint8_t icon_size;
} carousel_t;

static void carousel_init(carousel_t *c, const item_t *items, uint8_t count,
                           carousel_dir_t dir, uint16_t item_step, uint8_t icon_size) {
    c->items = items;
    c->count = count;
    c->selected = 0;
    c->dir = dir;
    c->item_step = item_step;
    c->icon_size = icon_size;
    c->scroll_pos = MC_FP_C(0);
    c->target_pos = MC_FP_C(0);
    memset(&c->spring_state, 0, sizeof(c->spring_state));
    c->spring_params.stiffness = MC_FP_C(180);
    c->spring_params.damping = MC_FP_C(12);
    c->spring_params.mass = MC_FP_C(1);
    /* target_pos = selected * item_step. scroll_pos animates toward target.
       In carousel_draw, center_offset + i*step - scroll_pos places selected item at center. */
    c->target_pos = MC_REAL_FROM_INT(c->selected * c->item_step);
    c->scroll_pos = c->target_pos;
}

static void carousel_select(carousel_t *c, int8_t idx) {
    if (idx < 0 || idx >= (int8_t)c->count || idx == c->selected) return;
    c->selected = idx;
    c->target_pos = MC_REAL_FROM_INT(c->selected * c->item_step);
}

static void carousel_update(carousel_t *c, uint32_t dt_ms) {
    mc_real_t dt = MC_FP_C((float)dt_ms / 1000.0f);
    mc_spring_step(&c->scroll_pos, &c->spring_state, &c->spring_params, c->target_pos, dt);
}

static void carousel_draw(carousel_t *c, eui_canvas_t *canvas, eui_rect_t area) {
    int16_t center_x = area.x + (int16_t)(area.w / 2);
    int16_t center_y = area.y + (int16_t)(area.h / 2);
    int16_t screen_center = (c->dir == CAROUSEL_HORIZONTAL) ? center_x : center_y;
    int16_t perp_center = (c->dir == CAROUSEL_HORIZONTAL) ? center_y : center_x;
    int16_t half_step = (int16_t)(c->item_step / 2);
    int16_t half_size = (int16_t)(c->icon_size / 2);
    int16_t center_offset = screen_center - half_step;  /* makes item 0 center at screen center */
    float scroll_f = MC_REAL_TO_FLOAT(c->scroll_pos);

    for (uint8_t i = 0; i < c->count; i++) {
        int16_t item_left = (int16_t)(center_offset + (float)(i * c->item_step) - scroll_f);
        int16_t draw_pos = area.x + item_left;
        int16_t perp_pos = perp_center;

        /* Visibility check */
        if (draw_pos + c->item_step < area.x || draw_pos > area.x + (int16_t)area.w)
            continue;

        int16_t ix, iy;
        if (c->dir == CAROUSEL_HORIZONTAL) {
            ix = draw_pos;
            iy = perp_pos - half_size - 10;
        } else {
            ix = perp_pos - half_size;
            iy = draw_pos;
        }

        /* Selection highlight */
        if (i == c->selected) {
            eui_color_t gold = eui_color_from_rgb(255, 200, 0);
            eui_canvas_set_color(canvas, gold);
            if (c->dir == CAROUSEL_HORIZONTAL) {
                eui_canvas_draw_round_rect(canvas, ix - 4, iy - 4,
                    c->item_step, c->icon_size + 28, 6);
            } else {
                eui_canvas_draw_round_rect(canvas, ix - 4, iy - 4,
                    c->icon_size + 28, c->item_step, 6);
            }
        }

        /* Draw icon */
        if (c->items[i].icon) {
            eui_canvas_draw_bitmap(canvas, ix, iy, c->items[i].icon);
        }

        /* Label */
        eui_canvas_set_color(canvas, EUI_COLOR_WHITE);
        eui_canvas_set_font(canvas, &wqy13_font);
        if (c->items[i].label) {
            uint16_t lw = eui_canvas_str_width(canvas, c->items[i].label);
            if (c->dir == CAROUSEL_HORIZONTAL) {
                eui_canvas_draw_str(canvas, draw_pos + half_step - (int16_t)(lw / 2),
                                    perp_pos + half_size + 2, c->items[i].label);
            } else {
                eui_canvas_draw_str(canvas, perp_pos + half_size + 4,
                                    draw_pos + half_step - 6, c->items[i].label);
            }
        }
    }
}

static bool carousel_input(carousel_t *c, const eui_event_t *e) {
    if (e->type != EUI_EVT_KEY_PRESS) return false;
    if (c->dir == CAROUSEL_HORIZONTAL) {
        if (e->data.key == EUI_KEY_LEFT && c->selected > 0) {
            carousel_select(c, c->selected - 1); return true;
        }
        if (e->data.key == EUI_KEY_RIGHT && c->selected < (int8_t)(c->count - 1)) {
            carousel_select(c, c->selected + 1); return true;
        }
    } else {
        if (e->data.key == EUI_KEY_UP && c->selected > 0) {
            carousel_select(c, c->selected - 1); return true;
        }
        if (e->data.key == EUI_KEY_DOWN && c->selected < (int8_t)(c->count - 1)) {
            carousel_select(c, c->selected + 1); return true;
        }
    }
    return false;
}

/* ─── Amiibo icon array ─── */
static const eui_bitmap_t* amiibo_icons[] = {
    &g_icon_mario, &g_icon_link, &g_icon_zelda, &g_icon_pikachu,
    &g_icon_kirby, &g_icon_samus, &g_icon_yoshi, &g_icon_dk,
};

/* ─── Amiibo detail data ─── */
typedef struct {
    const char *name;
    const char *series;
    const char *drops[6];
    uint8_t drop_count;
} amiibo_detail_t;

static amiibo_detail_t amiibo_data[] = {
    { "马力欧", "超级马力欧系列", {"超级蘑菇", "火焰花", "无敌星", "超级铃铛"}, 4 },
    { "林克", "塞尔达传说系列", {"大师剑", "海利亚盾", "滑翔伞", "时之笛"}, 4 },
    { "塞尔达", "塞尔达传说系列", {"光之箭", "时之笛", "三角力量", "塞尔达盾"}, 4 },
    { "皮卡丘", "宝可梦系列", {"电气球", "雷之石", "十万伏特", "电珠"}, 4 },
    { "卡比", "星之卡比系列", {"复制能力", "星星杖", "番茄", "M番茄"}, 4 },
    { "萨姆斯", "银河战士系列", {"能量罐", "导弹", "超炸", "加速器"}, 4 },
    { "耀西", "耀西系列", {"耀西蛋", "水果", "快乐花", "星星"}, 4 },
    { "森喜刚", "大金刚系列", {"大金刚桶", "香蕉", "矿车", "DK徽章"}, 4 },
};

/* ─── Navigation ─── */
enum { SCENE_APP_LIST = 1, SCENE_AMIIBO_LIST, SCENE_AMIIBO_DETAIL, SCENE_NFC, SCENE_SETTINGS };
static eui_view_dispatcher_t *g_vd;
static eui_scene_manager_t g_sm;
static carousel_t g_app_carousel, g_amiibo_carousel;
static int8_t g_detail_index = 0;
static int16_t g_detail_scroll = 0;
static int16_t g_detail_max_scroll = 0;

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

/* ─── App list items ─── */
static item_t app_items[] = {
    { "Amiibo", &g_icon_amiibo_app, NULL },
    { "NFC",    &g_icon_nfc,        NULL },
    { "设置",    &g_icon_settings,   NULL },
};

/* ─── Amiibo list items ─── */
static item_t amiibo_items[8];

/* ─── View handlers ─── */
static bool app_list_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = "应用列表";
        uint16_t tw = eui_canvas_str_width(c, title);
        uint8_t fh = eui_canvas_font_height(c);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2),
                             view->area.y + fh, title);
        carousel_draw(&g_app_carousel, c, (eui_rect_t){ 0, 30, 240, 200 });
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (carousel_input(&g_app_carousel, e)) {
            eui_view_mark_dirty(view); return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_OK) {
            switch (g_app_carousel.selected) {
                case 0: navigate_to(SCENE_AMIIBO_LIST, EUI_ANIM_SLIDE_LEFT); break;
                case 1: navigate_to(SCENE_NFC, EUI_ANIM_SLIDE_LEFT); break;
                case 2: navigate_to(SCENE_SETTINGS, EUI_ANIM_SLIDE_LEFT); break;
            }
            return true;
        }
        return false;
    }
    default: return false;
    }
}

static bool amiibo_list_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = "Amiibo";
        uint16_t tw = eui_canvas_str_width(c, title);
        uint8_t fh = eui_canvas_font_height(c);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2),
                             view->area.y + fh, title);
        carousel_draw(&g_amiibo_carousel, c, (eui_rect_t){ 0, 30, 240, 200 });
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (carousel_input(&g_amiibo_carousel, e)) {
            eui_view_mark_dirty(view); return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_OK) {
            g_detail_index = (int8_t)g_amiibo_carousel.selected;
            g_detail_scroll = 0;
            navigate_to(SCENE_AMIIBO_DETAIL, EUI_ANIM_SLIDE_LEFT);
            return true;
        }
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_BACK) {
            navigate_to(SCENE_APP_LIST, EUI_ANIM_SLIDE_RIGHT);
            return true;
        }
        return false;
    }
    default: return false;
    }
}

static bool detail_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);

        int16_t y = view->area.y + 10 - g_detail_scroll;
        amiibo_detail_t *d = &amiibo_data[g_detail_index];

        /* Name */
        eui_canvas_set_color(c, eui_color_from_rgb(255, 220, 80));
        eui_canvas_set_font(c, &wqy13_font);
        uint16_t tw_name = eui_canvas_str_width(c, d->name);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw_name / 2), y, d->name);
        y += 18;

        /* Icon */
        eui_canvas_draw_bitmap(c, view->area.x + 60, y, amiibo_icons[g_detail_index]);
        y += 126;

        /* Series */
        eui_canvas_set_color(c, eui_color_from_rgb(180, 180, 200));
        eui_canvas_set_font(c, &wqy13_font);
        eui_canvas_draw_str(c, view->area.x + 10, y, d->series);
        y += 18;

        /* Drops header */
        eui_canvas_set_color(c, eui_color_from_rgb(255, 200, 100));
        eui_canvas_draw_str(c, view->area.x + 10, y, "游戏掉落物品");
        y += 18;

        /* Drops list */
        eui_canvas_set_color(c, eui_color_from_rgb(200, 220, 255));
        for (uint8_t i = 0; i < d->drop_count; i++) {
            eui_canvas_draw_str(c, view->area.x + 20, y, d->drops[i]);
            y += 16;
        }

        g_detail_max_scroll = (y > (int16_t)(view->area.y + (int16_t)view->area.h - 10))
                              ? (y - (view->area.y + (int16_t)view->area.h - 10)) : 0;

        /* Back hint */
        eui_canvas_set_color(c, eui_color_from_rgb(120, 120, 140));
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + view->area.h - 12, "BACK: 返回");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS) {
            if (e->data.key == EUI_KEY_BACK) {
                navigate_to(SCENE_AMIIBO_LIST, EUI_ANIM_SLIDE_RIGHT);
                return true;
            }
            if (e->data.key == EUI_KEY_UP && g_detail_scroll > 0) {
                g_detail_scroll -= 12;
                if (g_detail_scroll < 0) g_detail_scroll = 0;
                eui_view_mark_dirty(view); return true;
            }
            if (e->data.key == EUI_KEY_DOWN && g_detail_scroll < g_detail_max_scroll) {
                g_detail_scroll += 12;
                eui_view_mark_dirty(view); return true;
            }
        }
        return false;
    }
    default: return false;
    }
}

static bool placeholder_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, eui_color_from_rgb(26, 26, 46));
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, eui_color_from_rgb(200, 200, 220));
        eui_canvas_set_font(c, &wqy13_font);
        const char *title = (view->model) ? (const char*)view->model : "功能";
        uint16_t tw = eui_canvas_str_width(c, title);
        eui_canvas_draw_str(c, view->area.x + (int16_t)(view->area.w / 2) - (int16_t)(tw / 2), view->area.y + 100, title);
        eui_canvas_set_color(c, eui_color_from_rgb(120, 120, 140));
        eui_canvas_draw_str(c, view->area.x + 4, view->area.y + view->area.h - 12, "BACK: 返回");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_BACK) {
            navigate_to(SCENE_APP_LIST, EUI_ANIM_SLIDE_RIGHT);
            return true;
        }
        return false;
    }
    default: return false;
    }
}

/* ─── Scene callbacks ─── */
static void scene_enter(void *ctx) { (void)ctx; printf("[SCENE] Enter\n"); }
static void scene_exit(void *ctx)  { (void)ctx; printf("[SCENE] Exit\n"); }

/* ─── Main ─── */
int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    eui_display_drv_t *display = eui_drv_raylib_create_display(W, H, 16);
    eui_input_drv_t *input = eui_drv_raylib_create_input();
    eui_config_t cfg = { .display = display, .input = input };
    eui_init(&cfg);
    eui_set_tick_callback(get_tick);
    display->init(display->user_data);
    g_vd = eui_get_view_dispatcher();

    /* Init carousels */
    carousel_init(&g_app_carousel, app_items, 3, CAROUSEL_HORIZONTAL, 140, 120);

    for (uint8_t i = 0; i < 8; i++) {
        amiibo_items[i].label = amiibo_data[i].name;
        amiibo_items[i].icon = amiibo_icons[i];
        amiibo_items[i].user_data = NULL;
    }
    carousel_init(&g_amiibo_carousel, amiibo_items, 8, CAROUSEL_HORIZONTAL, 140, 120);

    /* Create views */
    eui_view_t app_view, amiibo_view, detail_view, nfc_view, settings_view;

    eui_view_init(&app_view, app_list_handler, &app_view);
    app_view.area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(&amiibo_view, amiibo_list_handler, &amiibo_view);
    amiibo_view.area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(&detail_view, detail_handler, &detail_view);
    detail_view.area = (eui_rect_t){ 0, 0, W, H };

    eui_view_init(&nfc_view, placeholder_handler, &nfc_view);
    nfc_view.area = (eui_rect_t){ 0, 0, W, H };
    nfc_view.model = (void*)"NFC 读写器";

    eui_view_init(&settings_view, placeholder_handler, &settings_view);
    settings_view.area = (eui_rect_t){ 0, 0, W, H };
    settings_view.model = (void*)"设置";

    /* Register views */
    eui_view_dispatcher_add(g_vd, SCENE_APP_LIST, &app_view);
    eui_view_dispatcher_add(g_vd, SCENE_AMIIBO_LIST, &amiibo_view);
    eui_view_dispatcher_add(g_vd, SCENE_AMIIBO_DETAIL, &detail_view);
    eui_view_dispatcher_add(g_vd, SCENE_NFC, &nfc_view);
    eui_view_dispatcher_add(g_vd, SCENE_SETTINGS, &settings_view);

    /* Register scenes */
    eui_scene_t scenes[] = {
        { SCENE_APP_LIST, &app_view, scene_enter, scene_exit, NULL },
        { SCENE_AMIIBO_LIST, &amiibo_view, scene_enter, scene_exit, NULL },
        { SCENE_AMIIBO_DETAIL, &detail_view, scene_enter, scene_exit, NULL },
        { SCENE_NFC, &nfc_view, scene_enter, scene_exit, NULL },
        { SCENE_SETTINGS, &settings_view, scene_enter, scene_exit, NULL },
    };
    eui_scene_manager_register(&g_sm, scenes, 5);

    eui_anim_init();
    navigate_to(SCENE_APP_LIST, EUI_ANIM_NONE);

    uint32_t last_tick = get_tick();

    while (!eui_drv_raylib_window_should_close()) {
        eui_tick();

        uint32_t now = get_tick();
        uint32_t dt = now - last_tick;
        last_tick = now;

        carousel_update(&g_app_carousel, dt);
        carousel_update(&g_amiibo_carousel, dt);

        /* Mark views dirty during animation — check both velocity and position error */
        {
            float app_vel = MC_REAL_TO_FLOAT(g_app_carousel.spring_state.velocity);
            float app_err = MC_REAL_TO_FLOAT(g_app_carousel.scroll_pos - g_app_carousel.target_pos);
            if (fabsf(app_vel) > 0.5f || fabsf(app_err) > 0.5f) {
                eui_view_mark_dirty(&app_view);
            } else if (fabsf(app_err) > 0.01f) {
                /* Snap to target when very close to prevent sub-pixel jitter */
                g_app_carousel.scroll_pos = g_app_carousel.target_pos;
                g_app_carousel.spring_state.velocity = 0;
                eui_view_mark_dirty(&app_view);
            }
        }
        {
            float ami_vel = MC_REAL_TO_FLOAT(g_amiibo_carousel.spring_state.velocity);
            float ami_err = MC_REAL_TO_FLOAT(g_amiibo_carousel.scroll_pos - g_amiibo_carousel.target_pos);
            if (fabsf(ami_vel) > 0.5f || fabsf(ami_err) > 0.5f) {
                eui_view_mark_dirty(&amiibo_view);
            } else if (fabsf(ami_err) > 0.01f) {
                g_amiibo_carousel.scroll_pos = g_amiibo_carousel.target_pos;
                g_amiibo_carousel.spring_state.velocity = 0;
                eui_view_mark_dirty(&amiibo_view);
            }
        }

        eui_drv_raylib_refresh();

        if (IsKeyPressed(KEY_F2)) {
            eui_drv_raylib_save_screenshot("amiibo_demo.png");
            printf("Screenshot saved\n");
        }
    }

    display->deinit(display->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(input);
    eui_drv_raylib_destroy_display(display);
    return 0;
}
