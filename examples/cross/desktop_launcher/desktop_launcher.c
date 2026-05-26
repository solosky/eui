/* examples/cross/desktop_launcher/desktop_launcher.c
 * 400x300 2bpp Desktop Launcher Demo
 */
#include "eui/eui.h"
#include "eui/eui_font_wqy13.h"
#include "eui/eui_port_bootstrap.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define LAUNCHER_GET_TICK_MS() ((uint32_t)emscripten_get_now())
#elif defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#define LAUNCHER_GET_TICK_MS() ((uint32_t)(GetTime() * 1000.0f))
#else
#define LAUNCHER_GET_TICK_MS() 0
#endif
#else
#define LAUNCHER_GET_TICK_MS() 0
#endif

#define W 400
#define H 300

static bool desktop_view_handler(eui_view_event_t *evt, void *context);
static bool app_view_handler(eui_view_event_t *evt, void *context);

/* ─── App icon XBM 24x24 ─── */
static const uint8_t icon_reader[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFF,0xFF,0x00,0x80,0x00,0x01,0x80,0x00,0x01,0x80,0x00,0x01,
    0x80,0x00,0x01,0x80,0x00,0x01,0x80,0x00,0x01,0x80,0x00,0x01,
    0x80,0x00,0x01,0x80,0x00,0x01,0x80,0x00,0x01,0x80,0x00,0x01,
    0x80,0x00,0x01,0x80,0x00,0x01,0xFF,0xFF,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_game[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0xFF,0x00,0x03,0xFF,0xC0,0x07,0x00,0xE0,0x0C,0x00,0x30,
    0x18,0xFF,0x18,0x31,0xFF,0x8C,0x33,0x00,0xCC,0x33,0x00,0xCC,
    0x33,0x00,0xCC,0x33,0x00,0xCC,0x31,0xFF,0x8C,0x18,0xFF,0x18,
    0x0C,0x00,0x30,0x07,0x00,0xE0,0x03,0xFF,0xC0,0x00,0xFF,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_nfc[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x81,0xC0,
    0x04,0x7E,0x20,0x08,0x00,0x10,0x10,0x00,0x08,0x11,0x81,0x88,
    0x22,0x7E,0x44,0x22,0x00,0x44,0x22,0x00,0x44,0x22,0x00,0x44,
    0x22,0x00,0x44,0x22,0x00,0x44,0x11,0x81,0x88,0x10,0x7E,0x08,
    0x08,0x00,0x10,0x04,0x00,0x20,0x03,0x81,0xC0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_music[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x3F,0xFC,0x00,0x3F,0xFC,0x00,0x30,0x00,0x00,0x30,0x00,
    0x00,0x30,0x00,0x00,0x30,0x00,0x00,0x30,0x00,0x00,0x30,0x0C,
    0x00,0x30,0x1E,0x00,0x30,0x1E,0x00,0x30,0x1E,0x1E,0x30,0x1E,
    0x1E,0x30,0x0C,0x1E,0x30,0x00,0x0C,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_calc[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xF0,
    0x08,0x00,0x10,0x08,0x00,0x10,0x0F,0xFF,0xF0,0x08,0x00,0x10,
    0x08,0x00,0x10,0x08,0x00,0x10,0x0F,0xFF,0xF0,0x08,0x00,0x10,
    0x08,0x00,0x10,0x08,0x00,0x10,0x08,0x00,0x10,0x08,0x00,0x10,
    0x08,0x00,0x10,0x08,0x00,0x10,0x0F,0xFF,0xF0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_folder[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x07,0xFF,0x00,0x04,0x00,0x80,0x04,0x00,0x40,0x04,0x00,0x40,
    0x07,0xFF,0xC0,0x04,0x00,0x40,0x04,0x00,0x40,0x04,0x00,0x40,
    0x04,0x00,0x40,0x04,0x00,0x40,0x04,0x00,0x40,0x04,0x00,0x40,
    0x04,0x00,0x40,0x04,0x00,0x40,0x07,0xFF,0xC0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_calendar[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xFF,0xC0,
    0x02,0xAA,0x40,0x02,0xAA,0x40,0x03,0xFF,0xC0,0x02,0x00,0x40,
    0x02,0x00,0x40,0x02,0x00,0x40,0x02,0x0F,0x40,0x02,0x0F,0x40,
    0x02,0x00,0x40,0x02,0x0F,0x40,0x02,0x0F,0x40,0x02,0x0F,0x40,
    0x02,0x00,0x40,0x02,0x00,0x40,0x03,0xFF,0xC0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_amiibo[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x81,0xC0,
    0x04,0x00,0x20,0x08,0x00,0x10,0x10,0x00,0x08,0x10,0x00,0x08,
    0x20,0x00,0x04,0x20,0x00,0x04,0x20,0x00,0x04,0x20,0x00,0x04,
    0x20,0x00,0x04,0x20,0x00,0x04,0x10,0x00,0x08,0x10,0x00,0x08,
    0x08,0x00,0x10,0x04,0x00,0x20,0x03,0x81,0xC0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const uint8_t icon_settings[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,
    0x07,0xE0,0x00,0x0C,0x30,0x00,0x0F,0xF0,0x00,0x0C,0x30,0x00,
    0x08,0x10,0x00,0x18,0x18,0x00,0x33,0xCC,0x00,0x33,0xCC,0x00,
    0x18,0x18,0x00,0x08,0x10,0x00,0x0C,0x30,0x00,0x0F,0xF0,0x00,
    0x0C,0x30,0x00,0x07,0xE0,0x00,0x01,0x80,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* ─── App definitions ─── */
#define APP_COUNT 9
#define STATUSBAR_H 24

typedef struct {
    const char *name;
    const uint8_t *icon;
} app_def_t;

static const app_def_t g_apps[APP_COUNT] = {
    { "阅读器",      icon_reader },
    { "GB模拟器",    icon_game },
    { "NFC模拟器",   icon_nfc },
    { "音乐播放器",  icon_music },
    { "计算器",      icon_calc },
    { "文件管理器",  icon_folder },
    { "日历",        icon_calendar },
    { "amiibo模拟器",icon_amiibo },
    { "设置",        icon_settings },
};

/* ─── Grid layout ─── */
#define GRID_COLS 3
#define GRID_ROWS 3
#define CELL_W 120
#define CELL_H 80
#define CELL_GAP 16

static int16_t grid_left(void) {
    return (int16_t)((W - (CELL_W * GRID_COLS + CELL_GAP * (GRID_COLS - 1))) / 2);
}
static int16_t grid_top(void) {
    int16_t avail = H - STATUSBAR_H;
    int16_t grid_h = CELL_H * GRID_ROWS + CELL_GAP * (GRID_ROWS - 1);
    return (int16_t)(STATUSBAR_H + (avail - grid_h) / 2);
}
static int16_t cell_x(int8_t idx) {
    return (int16_t)(grid_left() + (idx % GRID_COLS) * (CELL_W + CELL_GAP));
}
static int16_t cell_y(int8_t idx) {
    return (int16_t)(grid_top() + (idx / GRID_COLS) * (CELL_H + CELL_GAP));
}

/* ─── Spring animation state ─── */
static int8_t g_selected = 0;
static mc_real_t g_hl_x, g_hl_y;
static mc_real_t g_hl_target_x, g_hl_target_y;
static mc_spring_state_t g_hl_spring_x, g_hl_spring_y;
static mc_spring_params_t g_hl_params;

static void update_highlight_target(void) {
    g_hl_target_x = MC_REAL_FROM_INT(cell_x(g_selected) + 2);
    g_hl_target_y = MC_REAL_FROM_INT(cell_y(g_selected) + 2);
}

static void init_highlight(void) {
    memset(&g_hl_spring_x, 0, sizeof(g_hl_spring_x));
    memset(&g_hl_spring_y, 0, sizeof(g_hl_spring_y));
    g_hl_params.stiffness = MC_FP_C(200);
    g_hl_params.damping   = MC_FP_C(15);
    g_hl_params.mass      = MC_FP_C(1);
    update_highlight_target();
    g_hl_x = g_hl_target_x;
    g_hl_y = g_hl_target_y;
}

/* ─── Status bar state ─── */
static uint32_t g_status_minutes = 9 * 60 + 41;
static uint32_t g_status_sub_tick = 0;

/* ─── Global pointers ─── */
static eui_view_dispatcher_t *g_vd = NULL;
static eui_view_t *g_desktop_view = NULL;

/* ─── Status bar ─── */
static void draw_status_bar(eui_canvas_t *c) {
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, 0, 0, W, STATUSBAR_H);
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_line(c, 0, STATUSBAR_H - 1, W - 1, STATUSBAR_H - 1);

    eui_canvas_set_color(c, 3);
    eui_canvas_set_font(c, &eui_font_wqy13);
    uint32_t hour = g_status_minutes / 60;
    uint32_t min  = g_status_minutes % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)hour, (unsigned long)min);
    eui_canvas_draw_str(c, 8, 20, buf);

    int16_t bx = W - 26;
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_rect(c, bx, 7, 14, 10);
    eui_canvas_fill_rect(c, bx + 14, 10, 3, 4);
    eui_canvas_set_color(c, 2);
    eui_canvas_fill_rect(c, bx + 3, 9, 6, 6);

    int16_t wx = W - 46;
    eui_canvas_set_color(c, 3);
    eui_canvas_fill_circle(c, wx, 16, 2);
    eui_canvas_set_color(c, 2);
    eui_canvas_fill_circle(c, wx, 13, 3);
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_circle(c, wx, 10, 4);
}

/* ─── App content drawing ─── */
static void draw_reader(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    const char *lines[] = {
        "第1章  开始",
        "夜色降临，小镇上的",
        "灯光一盏盏亮起。",
        "李明站在窗前，望",
        "着远处的山影，心",
        "中充满了期待。",
        "明天，就是新学期",
        "开学的日子。他将",
        "要前往省城的大学",
        "开始全新的生活。",
    };
    int16_t ly = y + 20;
    for (size_t i = 0; i < sizeof(lines)/sizeof(lines[0]); i++) {
        eui_canvas_set_color(c, (i == 0) ? 3 : 2);
        eui_canvas_draw_str(c, x + 8, ly, lines[i]);
        ly += 19;
    }
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, x + w - 6, y, 6, h);
    eui_canvas_set_color(c, 2);
    eui_canvas_fill_rect(c, x + w - 5, y + 8, 4, 40);
}

static void draw_gb_emu(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_color(c, 2);
    eui_canvas_fill_rect(c, x + 40, y + 8, 120, 100);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_rect(c, x + 40, y + 8, 120, 100);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_draw_str(c, x + 60, y + 24, "GAME BOY");
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, x + 64, y + 50, 8, 8);
    eui_canvas_fill_rect(c, x + 76, y + 58, 8, 8);
    eui_canvas_fill_rect(c, x + 88, y + 50, 8, 8);
    eui_canvas_set_color(c, 3);
    eui_canvas_fill_rect(c, x + 104, y + 56, 10, 10);
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_str(c, x + 60, y + 130, "A  B");
    eui_canvas_draw_str(c, x + 44, y + 148, "START");
    eui_canvas_draw_str(c, x + 100, y + 148, "SELECT");
}

static void draw_nfc(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 20, y + 22, "NFC 读写器");
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_str(c, x + 20, y + 46, "正在扫描...");
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_round_rect(c, x + 40, y + 64, 80, 60, 8);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_round_rect(c, x + 40, y + 64, 80, 60, 8);
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_str(c, x + 60, y + 88, "NFC TAG");
    eui_canvas_set_color(c, 1);
    eui_canvas_draw_str(c, x + 36, y + 150, "请将卡片放置在");
    eui_canvas_draw_str(c, x + 36, y + 170, "读卡区域");
}

static void draw_music(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    int16_t bar_y = y + h - 30;
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, x, bar_y, w, 30);
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_line(c, x, bar_y, x + w - 1, bar_y);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 8, bar_y + 8, "Classic Piano");
    eui_canvas_set_color(c, 2);
    eui_canvas_draw_str(c, x + 8, bar_y + 20, "03:42 - 05:18");
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, x + 110, bar_y + 6, 120, 16);
    eui_canvas_set_color(c, 3);
    eui_canvas_fill_rect(c, x + 110, bar_y + 6, 72, 16);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 8, y + 22, "播放列表");
    const char *songs[] = {"月半小夜曲", "致爱丽丝", "四季·春", "卡农"};
    int16_t sy = y + 46;
    for (size_t i = 0; i < sizeof(songs)/sizeof(songs[0]); i++) {
        eui_canvas_set_color(c, i == 0 ? 3 : 2);
        eui_canvas_draw_str(c, x + 16, sy, songs[i]);
        sy += 22;
    }
}

static void draw_calc(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_color(c, 1);
    eui_canvas_fill_rect(c, x + 20, y + 8, w - 40, 28);
    eui_canvas_set_color(c, 3);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_draw_str(c, x + w - 44, y + 18, "42");
    const char *btns[] = {
        "7","8","9","+",
        "4","5","6","-",
        "1","2","3","*",
        "C","0","=","/"
    };
    int16_t bx = x + 12, by = y + 44;
    int bw = 36, bh = 22, bg = 4;
    for (int i = 0; i < 16; i++) {
        int col = i % 4;
        int row = i / 4;
        int16_t px = bx + col * (bw + bg);
        int16_t py = by + row * (bh + bg);
        eui_canvas_set_color(c, col == 3 ? 2 : 1);
        eui_canvas_fill_rect(c, px, py, bw, bh);
        eui_canvas_set_color(c, col == 3 ? 0 : 3);
        eui_canvas_draw_str(c, px + 12, py + 4, btns[i]);
    }
}

static void draw_filemgr(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 8, y + 20, "文件管理器");
    const char *items[] = {
        " | SD卡   内置存储",
        " | 下载   下载文件",
        " | 文档   重要文档",
        " | 图片   相册照片",
        " | 音乐   音乐文件",
        " | 视频   视频文件",
        " | 应用   已安装",
        " | 数据   应用数据",
    };
    int16_t sy = y + 42;
    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        eui_canvas_set_color(c, i == 0 ? 3 : 2);
        eui_canvas_draw_str(c, x + 16, sy, items[i]);
        sy += 20;
    }
}

static void draw_calendar(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 20, y + 20, "2026年5月");
    const char *wd[] = {"日","一","二","三","四","五","六"};
    int16_t gx = x + 12, gy = y + 30;
    int cw = 24, ch = 20;
    for (int i = 0; i < 7; i++) {
        eui_canvas_set_color(c, i >= 5 ? 2 : 3);
        eui_canvas_draw_str(c, gx + i * cw + 6, gy, wd[i]);
    }
    gy += 22;
    uint8_t days[] = {0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
                      18,19,20,21,22,23,24,25,26,27,28,29,30,31,0,0};
    uint8_t di = 0;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 7; col++) {
            uint8_t d = days[di++];
            if (d > 0) {
                bool today = (d == 25);
                eui_canvas_set_color(c, today ? 2 : 1);
                eui_canvas_fill_rect(c, gx + col * cw, gy + row * ch, cw - 2, ch - 2);
                eui_canvas_set_color(c, today ? 0 : 3);
                char db[4];
                snprintf(db, sizeof(db), "%d", d);
                eui_canvas_draw_str(c, gx + col * cw + 6, gy + row * ch + 4, db);
            }
        }
    }
}

static void draw_amiibo(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w; (void)h;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 8, y + 20, "amiibo 列表");
    const char *chars[] = {
        " Mario    马力欧",
        " Link     林克",
        " Zelda    塞尔达",
        " Pikachu  皮卡丘",
        " Kirby    卡比",
        " Samus    萨姆斯",
        " Yoshi    耀西",
        " DK       森喜刚",
    };
    int16_t ay = y + 42;
    for (size_t i = 0; i < sizeof(chars)/sizeof(chars[0]); i++) {
        eui_canvas_set_color(c, i == 0 ? 3 : 2);
        eui_canvas_draw_str(c, x + 16, ay, chars[i]);
        ay += 20;
    }
}

static void draw_settings(eui_canvas_t *c, int16_t x, int16_t y, int16_t w, int16_t h) {
    (void)w;
    eui_canvas_set_color(c, 0);
    eui_canvas_fill_rect(c, x, y, w, h);
    eui_canvas_set_font(c, &eui_font_wqy13);
    eui_canvas_set_color(c, 3);
    eui_canvas_draw_str(c, x + 8, y + 20, "设置");
    const char *items[] = {
        "WiFi           [开启]",
        "蓝牙           [关闭]",
        "亮度        ▓▓▓▓░░░░",
        "音量        ▓▓▓▓▓░░░",
        "深色模式     [关闭]",
        "存储   78% 已用",
        "关于本机",
    };
    int16_t sy = y + 44;
    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        eui_canvas_set_color(c, i == 0 ? 3 : 2);
        eui_canvas_draw_str(c, x + 16, sy, items[i]);
        sy += 22;
    }
}

typedef void (*app_draw_fn)(eui_canvas_t *, int16_t, int16_t, int16_t, int16_t);
static const app_draw_fn g_app_draw[APP_COUNT] = {
    draw_reader, draw_gb_emu, draw_nfc,
    draw_music, draw_calc, draw_filemgr,
    draw_calendar, draw_amiibo, draw_settings,
};

/* ─── Desktop view ─── */
static bool desktop_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, 0);
        eui_canvas_fill_rect(c, view->area.x, view->area.y, view->area.w, view->area.h);
        draw_status_bar(c);
        eui_canvas_set_font(c, &eui_font_wqy13);

        for (int i = 0; i < APP_COUNT; i++) {
            int16_t cx = cell_x((int8_t)i);
            int16_t cy = cell_y((int8_t)i);
            eui_canvas_set_color(c, 1);
            eui_canvas_fill_round_rect(c, cx, cy, CELL_W, CELL_H, 4);
            eui_canvas_set_color(c, 3);
            int16_t ix = cx + (CELL_W - 24) / 2;
            int16_t iy = cy + 4;
            eui_canvas_draw_xbm(c, ix, iy, 24, 24, g_apps[i].icon);
            uint16_t tw = eui_canvas_str_width(c, g_apps[i].name);
            eui_canvas_draw_str(c, cx + (CELL_W - (int16_t)tw) / 2, cy + 8 + 24 + 12, g_apps[i].name);
        }

        int16_t hx = (int16_t)MC_REAL_TO_INT(g_hl_x);
        int16_t hy = (int16_t)MC_REAL_TO_INT(g_hl_y);
        eui_canvas_set_color(c, 3);
        eui_canvas_draw_round_rect(c, hx - 2, hy - 2, CELL_W - 2, CELL_H - 2, 6);
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS) {
            int8_t old = g_selected;
            switch (e->data.key) {
            case EUI_KEY_UP:
                if (g_selected >= GRID_COLS) g_selected -= GRID_COLS;
                break;
            case EUI_KEY_DOWN:
                if (g_selected < APP_COUNT - GRID_COLS) g_selected += GRID_COLS;
                break;
            case EUI_KEY_LEFT:
                if (g_selected % GRID_COLS > 0) g_selected--;
                break;
            case EUI_KEY_RIGHT:
                if (g_selected % GRID_COLS < GRID_COLS - 1 && g_selected < APP_COUNT - 1) g_selected++;
                break;
            case EUI_KEY_OK:
                eui_view_dispatcher_switch_to(g_vd, 2, EUI_ANIM_SLIDE_LEFT);
                return true;
            default:
                return false;
            }
            if (g_selected != old) {
                update_highlight_target();
                eui_view_mark_dirty(view);
            }
            return true;
        }
        return false;
    }
    case EUI_VIEW_EVT_ENTER: {
        init_highlight();
        eui_view_mark_dirty(view);
        return true;
    }
    default:
        return false;
    }
}

/* ─── App content view ─── */
static bool app_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    switch (evt->type) {
    case EUI_VIEW_EVT_DRAW: {
        eui_canvas_t *c = evt->event.draw.canvas;
        app_draw_fn fn = g_app_draw[g_selected];
        fn(c, view->area.x, view->area.y, view->area.w, view->area.h);
        eui_canvas_set_color(c, 1);
        eui_canvas_set_font(c, &eui_font_wqy13);
        eui_canvas_draw_str(c, 4, H - 6, "BACK: 返回桌面");
        return true;
    }
    case EUI_VIEW_EVT_INPUT: {
        const eui_event_t *e = evt->event.input.input;
        if (e->type == EUI_EVT_KEY_PRESS && e->data.key == EUI_KEY_BACK) {
            eui_view_dispatcher_switch_to(g_vd, 1, EUI_ANIM_SLIDE_RIGHT);
            return true;
        }
        return false;
    }
    default:
        return false;
    }
}

/* ─── Per-frame animation tick ─── */
static uint32_t g_last_tick = 0;

static uint32_t launcher_tick(void) {
    uint32_t now = LAUNCHER_GET_TICK_MS();
    if (g_last_tick == 0) { g_last_tick = now; return now; }
    uint32_t dt = now - g_last_tick;
    g_last_tick = now;

    if (dt > 100) dt = 100;

    mc_real_t dt_s = MC_FP_C((float)dt / 1000.0f);
    mc_spring_step(&g_hl_x, &g_hl_spring_x, &g_hl_params, g_hl_target_x, dt_s);
    mc_spring_step(&g_hl_y, &g_hl_spring_y, &g_hl_params, g_hl_target_y, dt_s);

    g_status_sub_tick += dt;
    if (g_status_sub_tick >= 2000) {
        g_status_sub_tick = 0;
        g_status_minutes++;
        if (g_status_minutes >= 24 * 60) g_status_minutes = 0;
        if (g_desktop_view) eui_view_mark_dirty(g_desktop_view);
    }

    float vx = MC_REAL_TO_FLOAT(g_hl_spring_x.velocity);
    float vy = MC_REAL_TO_FLOAT(g_hl_spring_y.velocity);
    float ex = MC_REAL_TO_FLOAT(g_hl_x - g_hl_target_x);
    float ey = MC_REAL_TO_FLOAT(g_hl_y - g_hl_target_y);
    if (fabsf(vx) > 0.5f || fabsf(vy) > 0.5f || fabsf(ex) > 1.0f || fabsf(ey) > 1.0f) {
        if (g_desktop_view) eui_view_mark_dirty(g_desktop_view);
    }

    return now;
}

/* ─── Setup ─── */
void eui_example_setup(const eui_example_config_t *cfg) {
    (void)cfg;

    g_vd = eui_get_view_dispatcher();

    eui_view_t *dv = (eui_view_t*)eui_malloc(sizeof(eui_view_t));
    eui_view_init(dv, desktop_view_handler, dv);
    dv->area = (eui_rect_t){ 0, 0, W, H };
    eui_view_dispatcher_add(g_vd, 1, dv);
    g_desktop_view = dv;

    eui_view_t *av = (eui_view_t*)eui_malloc(sizeof(eui_view_t));
    eui_view_init(av, app_view_handler, av);
    av->area = (eui_rect_t){ 0, 0, W, H };
    eui_view_dispatcher_add(g_vd, 2, av);

    eui_set_tick_callback(launcher_tick);

    eui_anim_init();
    eui_view_dispatcher_switch_to(g_vd, 1, EUI_ANIM_NONE);
}
