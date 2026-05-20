#include <stdint.h>

#include "test_u8g2_profont10_data.h"

#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include <stdio.h>
#include <string.h>

#define POOL_SIZE 65536
static uint8_t mem_pool[POOL_SIZE];

static const eui_font_t profont10_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 10,
    .baseline = 8,
    .flags = 0,
    .data = u8g2_font_profont10_tf_data,
    .lookup_glyph = NULL,
};

#define IMG_W 800
#define IMG_H 200

#if EUI_COLOR_DEPTH == 1
#define BUF_SIZE (IMG_W * IMG_H / 8)
#else
#define BUF_SIZE (IMG_W * IMG_H)
#endif

static uint8_t img_buf[BUF_SIZE];

static int count_pixels_1bpp(void)
{
    int count = 0;
    for (int i = 0; i < (int)sizeof(img_buf); i++) {
        uint8_t b = img_buf[i];
        while (b) { count += (b & 1); b >>= 1; }
    }
    return count;
}

static void write_pbm(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) { printf("FAIL: cannot open %s\n", filename); return; }
    fprintf(f, "P4\n%d %d\n", IMG_W, IMG_H);
#if EUI_COLOR_DEPTH == 1
    fwrite(img_buf, 1, BUF_SIZE, f);
#else
    for (int y = 0; y < IMG_H; y++) {
        for (int x = 0; x < IMG_W; x += 8) {
            uint8_t byte = 0;
            for (int b = 0; b < 8 && x + b < IMG_W; b++) {
                int idx = y * IMG_W + x + b;
#if EUI_COLOR_DEPTH == 8
                if (img_buf[idx] > 128) byte |= (1 << (7 - b));
#else
                uint16_t *p16 = (uint16_t *)img_buf;
                if (p16[idx] > 0) byte |= (1 << (7 - b));
#endif
            }
            fwrite(&byte, 1, 1, f);
        }
    }
#endif
    fclose(f);
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Real U8G2 Font Render Test ===\n");

    memset(img_buf, 0, sizeof(img_buf));

    /* Verify font can be read */
    uint8_t test_w = eui_font_get_char_width(&profont10_font, 'A');
    printf("  profont10 'A' width: %d\n", test_w);
    if (test_w == 0) {
        printf("FAIL: font not readable\n");
        return 1;
    }

    /* Render all printable ASCII chars */
    int cur_x = 10, cur_y = 20;
    int chars_per_row = 20;
    int char_count = 0;
    int fail_count = 0;

    for (int c = 32; c <= 126; c++) {
        uint8_t cw = eui_font_get_char_width(&profont10_font, (char)c);
        if (cw == 0) {
            printf("  Char %d (0x%02X) has 0 width\n", c, c);
            fail_count++;
            continue;
        }

        uint8_t glyph_buf[128] = {0};
        uint8_t adv = eui_font_draw_char(&profont10_font, (char)c,
                                          glyph_buf, 1, 1);
        if (adv == 0) {
            printf("  Char %d (0x%02X) draw returned 0\n", c, c);
            fail_count++;
            continue;
        }

        /* Estimate glyph height from buffer (not actual height) */
        for (int row = 0; row < 10; row++) {
            for (int col = 0; col < 10; col++) {
                uint8_t byte = glyph_buf[row];
                if (byte & (1 << (7 - col))) {
                    int px = cur_y + row;
                    int py = cur_x + col;
                    if (px >= 0 && px < IMG_H && py >= 0 && py < IMG_W) {
                        int idx = px * (IMG_W / 8) + py / 8;
                        img_buf[idx] |= (1 << (7 - (py % 8)));
                    }
                }
            }
        }

        cur_x += adv + 1;
        char_count++;
        if (char_count % chars_per_row == 0) {
            cur_x = 10;
            cur_y += 12;
        }
    }

    printf("  Total chars: %d, render errors: %d\n", char_count, fail_count);
    printf("  Total pixels: %d\n", count_pixels_1bpp());

    write_pbm("font_render.pbm");

    if (count_pixels_1bpp() == 0) {
        printf("FAIL: no pixels rendered\n");
        return 1;
    }
    if (fail_count > 0) {
        printf("FAIL: %d characters had rendering errors\n", fail_count);
        return 1;
    }
    printf("PASS: ALL %d characters rendered successfully\n", char_count);
    return 0;
}
