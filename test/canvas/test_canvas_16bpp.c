/*
 * 16bpp canvas test — kerning is handled by canvas internally via
 * the font's lookup_glyph callback; no external API needed.
 *
 * Compile:
 *   gcc -o test_canvas_16bpp                   \
 *       -include test/config_16bpp.h            \
 *       -I include -I src -I third_party/tlsf   \
 *       test/test_canvas_16bpp.c                \
 *       src/eui_canvas.c src/eui_font.c         \
 *       src/eui_font_bdf.c src/eui_font_builtin.c \
 *       src/eui_font_u8g2.c                     \
 *       src/eui_allocator.c third_party/tlsf/tlsf.c \
 *       -lm
 * Run:   ./test_canvas_16bpp
 * Output: test_canvas_16bpp.bmp
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Library headers */
#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"

/* Internal header for lookup function */
#include "eui/eui_font_internal.h"

/* Kerning font data */
#include "data/test_font_kerning.h"
#include "common/eui_test.h"

/* ---- TLSF memory pool ---- */
#define POOL_SIZE 393216
static uint8_t mem_pool[POOL_SIZE];

/* ---- Mock display (16bpp) ---- */
#define CANVAS_W 320
#define CANVAS_H 400
#define BUF_SIZE (CANVAS_W * CANVAS_H * 2)

static uint8_t mock_buf[BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
    int row_stride = CANVAS_W * 2;
    int src_stride = r->w * 2;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * row_stride + r->x * 2),
               b + row * src_stride, src_stride);
    }
}

static eui_display_drv_t mock_display = {
    .caps = { .width = CANVAS_W, .height = CANVAS_H, .color_depth = 16,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

#define RGB565(r,g,b) ((uint16_t)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3)))
#define WHITE  RGB565(255,255,255)
#define YELLOW RGB565(255,255,0)

/* ---- kerning font (u8g2 format) ---- */
static const eui_font_t kern_font = {
    .format        = EUI_FONT_FORMAT_U8G2,
    .line_height   = 8,
    .baseline      = 7,
    .flags         = EUI_FONT_HAS_KERNING | EUI_FONT_HAS_UNICODE,
    .data          = test_kern_font_data,
    .lookup_glyph  = eui_font_u8g2_lookup_glyph,
};

/* ---- BMP writer ---- */
static int write_bmp(const char *fn)
{
    int row = CANVAS_W * 3;
    int pad = (4 - row % 4) % 4;
    int stride = row + pad;
    int pix_off = 54;
    int file_sz = pix_off + stride * CANVAS_H;

    unsigned char h[14] = { 'B','M' };
    h[2]=file_sz&0xFF;h[3]=(file_sz>>8)&0xFF;
    h[4]=(file_sz>>16)&0xFF;h[5]=(file_sz>>24)&0xFF;
    h[10]=pix_off&0xFF;h[11]=(pix_off>>8)&0xFF;

    unsigned char d[40] = {40,0,0,0};
    d[4]=CANVAS_W&0xFF;d[5]=(CANVAS_W>>8)&0xFF;
    d[8]=CANVAS_H&0xFF;d[9]=(CANVAS_H>>8)&0xFF;
    d[12]=1;d[14]=24;

    FILE *f = fopen(fn,"wb");
    if (!f) return -1;
    fwrite(h,1,14,f); fwrite(d,1,40,f);
    for (int y = CANVAS_H-1; y >= 0; y--) {
        unsigned char rbuf[CANVAS_W*3+4];
        for (int x = 0; x < CANVAS_W; x++) {
            uint16_t p = ((uint16_t*)mock_buf)[y*CANVAS_W+x];
            rbuf[x*3+0] = (p & 0x1F) << 3;
            rbuf[x*3+1] = ((p>>5) & 0x3F) << 2;
            rbuf[x*3+2] = ((p>>11) & 0x1F) << 3;
        }
        fwrite(rbuf,1,stride,f);
    }
    fclose(f);
    return 0;
}

/* ================================================================== */
int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);

    printf("=== 16bpp Canvas Render Test (kerning via canvas API) ===\n");
    printf("Canvas: %dx%d  color_depth=16\n\n", CANVAS_W, CANVAS_H);

    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) { printf("FAIL: create\n"); return 1; }

    eui_canvas_set_bg_color(c, RGB565(8, 8, 28));
    eui_canvas_set_color(c, WHITE);
    eui_canvas_clear(c);

    int y = 0;

    /* Title — built-in font (BDF, no kerning) */
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_color(c, WHITE);
    eui_canvas_draw_str(c, 0, y, "16bpp Canvas: kerning via canvas API"); y += 14;
    eui_canvas_set_color(c, YELLOW);
    eui_canvas_draw_str(c, 0, y, "=== Built-in font (BDF) — no kerning ==="); y += 14;

    eui_canvas_set_color(c, RGB565(180, 200, 255));
    eui_canvas_draw_str(c, 0, y, "The quick brown fox jumps over the lazy dog."); y += 12;
    eui_canvas_draw_str(c, 0, y, "TAVATAR  AVERY  YEARLY  WAVY  TATTOO"); y += 14;

    /* Kerning font (u8g2 with TA kerning pair) */
    eui_canvas_set_font(c, &kern_font);
    eui_canvas_set_color(c, YELLOW);
    eui_canvas_draw_str(c, 0, y, "=== Kerning font (u8g2) — canvas handles internally ==="); y += 14;

    eui_canvas_set_color(c, RGB565(100, 255, 100));
    eui_canvas_draw_str(c, 0, y, "Kerning enabled: 'TA' pair gets -2px advance via lookup_glyph"); y += 12;

    eui_canvas_set_color(c, WHITE);
    eui_canvas_draw_str(c, 0, y, "TAVATAR  TATTOO  TANGO  TAN"); y += 14;

    /* Colored shapes demonstration */
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_color(c, YELLOW);
    eui_canvas_draw_str(c, 0, y, "=== Colored Shapes (16bpp) ==="); y += 14;

    eui_canvas_set_color(c, RGB565(255,80,80));
    eui_canvas_fill_rect(c, 10, y, 50, 40);
    eui_canvas_set_color(c, RGB565(80,200,80));
    eui_canvas_fill_circle(c, 100, y+20, 20);
    eui_canvas_set_color(c, RGB565(80,80,255));
    eui_canvas_draw_rect(c, 150, y, 50, 40);
    eui_canvas_set_color(c, RGB565(255,200,0));
    eui_canvas_draw_circle(c, 240, y+20, 20);

    y += 48;
    eui_canvas_set_color(c, RGB565(200,50,200));
    eui_canvas_fill_round_rect(c, 10, y, 70, 24, 6);
    eui_canvas_set_color(c, RGB565(0,220,220));
    eui_canvas_draw_triangle(c, 120, y+24, 100, y, 140, y);
    eui_canvas_set_color(c, RGB565(255,180,0));
    eui_canvas_draw_round_rect(c, 170, y, 60, 24, 6);

    y += 32;
    /* Color gradient */
    eui_canvas_set_color(c, YELLOW);
    eui_canvas_draw_str(c, 0, y, "=== Gradient (16bpp) ==="); y += 14;
    for (int i = 0; i < CANVAS_W; i++) {
        for (int j = 0; j < 24; j++) {
            eui_canvas_set_color(c, RGB565(i*200/CANVAS_W, j*10, (CANVAS_W-i)*200/CANVAS_W));
            eui_canvas_draw_dot(c, i, y+j);
        }
    }
    y += 30;

    /* Commit and write BMP */
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_commit(c);

    if (write_bmp("test_canvas_16bpp.bmp") == 0)
        printf("\n  -> test_canvas_16bpp.bmp  (%dx%d 24-bit BMP)\n", CANVAS_W, CANVAS_H);
    else
        printf("\n  FAIL: BMP write\n");

    eui_canvas_destroy(c);
    printf("Done.\n");
    return 0;
}
