/* test/canvas/test_canvas_1bpp.c — 1bpp/2bpp 抖动目检画廊 */
#include "eui/eui_canvas.h"
#include "eui/eui_canvas_internal.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define GAL_W 200
#define GAL_H 200
#define GAL_POOL 65536
static uint8_t gal_pool[GAL_POOL];
static uint8_t gal_buf[GAL_W * GAL_H * (EUI_COLOR_DEPTH == 16 ? 2 : 1)];

static void gal_write(const uint8_t *b, const eui_rect_t *r, void *ud)
{ (void)b; (void)r; (void)ud; }

static eui_display_drv_t gal_display = {
    .caps = { .width = GAL_W, .height = GAL_H, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL, .write_buffer = gal_write,
};

static void put32(uint8_t *p, uint32_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

/* 单像素灰度画布 → 24 位 BMP（自下而上、BGR、4 字节行对齐） */
static void write_gray_bmp(const char *path, const uint8_t *gray, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) { printf("  (无法写出 %s)\n", path); return; }
    int row_bytes = (w * 3 + 3) & ~3;
    uint32_t data_size = (uint32_t)row_bytes * (uint32_t)h;
    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    put32(hdr + 2, 54u + data_size);
    put32(hdr + 10, 54u);
    put32(hdr + 14, 40u);
    put32(hdr + 18, (uint32_t)w);
    put32(hdr + 22, (uint32_t)h);
    hdr[26] = 1; hdr[28] = 24;
    put32(hdr + 34, data_size);
    fwrite(hdr, 1, 54, f);
    uint8_t row[GAL_W * 3 + 4];
    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, (size_t)row_bytes);
        for (int x = 0; x < w; x++) {
            uint8_t g = gray[y * w + x];
            row[x * 3] = g; row[x * 3 + 1] = g; row[x * 3 + 2] = g;   /* BGR */
        }
        fwrite(row, 1, (size_t)row_bytes, f);
    }
    fclose(f);
}

/* 把 canvas 像素转成 0..255 灰度（低位深按级数放大） */
static uint8_t pixel_gray(eui_canvas_t *c, int x, int y)
{
    uint32_t v = (uint32_t)eui_canvas_px_get(c, x, y);
#if EUI_COLOR_DEPTH == 1
    return (uint8_t)(v ? 255u : 0u);
#elif EUI_COLOR_DEPTH == 2
    return (uint8_t)(v * 85u);
#elif EUI_COLOR_DEPTH == 4
    return (uint8_t)(v * 17u);
#else
    return (uint8_t)v;
#endif
}

int main(void)
{
    eui_allocator_init_tlsf(gal_pool, GAL_POOL);
    eui_canvas_t *c = eui_canvas_create(&gal_display);
    if (!c) { printf("FAIL: create\n"); return 1; }
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    /* 用"该色深的最亮级"而不是 EUI_COLOR_WHITE：8bpp 下后者是 1（近黑） */
    eui_canvas_set_color(c, eui_color_from_gray(255));

    /* 细弧（1px） / 粗环 / 扇形 / 极小半径 / 抖动斜坡 */
    eui_canvas_draw_arc(c, 50, 50, 40, 1, -90, 90);
    eui_canvas_draw_ring(c, 150, 50, 40, 30, 0, 270);
    eui_canvas_fill_pie(c, 50, 150, 40, 30, 210);
    eui_canvas_fill_circle(c, 155, 155, 2);
    eui_canvas_draw_circle(c, 170, 170, 5);
    for (int i = 0; i < 32; i++) {                 /* 覆盖度斜坡：目检抖动密度 */
        eui_canvas_draw_ring(c, 100, 100, 90, 80, (int16_t)(i * 11), (int16_t)(i * 11 + 8));
    }
    eui_canvas_commit(c);

    static uint8_t gray[GAL_W * GAL_H];
    for (int y = 0; y < GAL_H; y++)
        for (int x = 0; x < GAL_W; x++) gray[y * GAL_W + x] = pixel_gray(c, x, y);
    write_gray_bmp("test_canvas_1bpp.bmp", gray, GAL_W, GAL_H);
    printf("wrote test_canvas_1bpp.bmp\n");

    eui_canvas_destroy(c);
    return 0;
}
