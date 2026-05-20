#include <stdint.h>

#include "test_u8g2_profont10_data.h"

#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
#define IMG_H 260

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

static void write_bmp(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) { printf("FAIL: cannot open %s\n", filename); return; }

    int row_size = ((IMG_W + 31) / 32) * 4;
    int pixel_data_size = row_size * IMG_H;
    int palette_size = 8;
    int offset = 14 + 40 + palette_size;
    int file_size = offset + pixel_data_size;

    /* BMP file header */
    uint16_t bfType = 0x4D42;
    uint32_t bfSize = file_size;
    uint32_t bfOffBits = offset;
    fwrite(&bfType, 2, 1, f);
    fwrite(&bfSize, 4, 1, f);
    uint32_t zero32 = 0;
    fwrite(&zero32, 4, 1, f);
    fwrite(&bfOffBits, 4, 1, f);

    /* DIB header (BITMAPINFOHEADER) */
    uint32_t biSize = 40;
    int32_t  biWidth = IMG_W;
    int32_t  biHeight = IMG_H;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 1;
    uint32_t biCompression = 0;
    uint32_t biSizeImage = pixel_data_size;
    int32_t  biXPelsPerMeter = 2835;
    int32_t  biYPelsPerMeter = 2835;
    uint32_t biClrUsed = 2;
    uint32_t biClrImportant = 2;
    fwrite(&biSize, 4, 1, f);
    fwrite(&biWidth, 4, 1, f);
    fwrite(&biHeight, 4, 1, f);
    fwrite(&biPlanes, 2, 1, f);
    fwrite(&biBitCount, 2, 1, f);
    fwrite(&biCompression, 4, 1, f);
    fwrite(&biSizeImage, 4, 1, f);
    fwrite(&biXPelsPerMeter, 4, 1, f);
    fwrite(&biYPelsPerMeter, 4, 1, f);
    fwrite(&biClrUsed, 4, 1, f);
    fwrite(&biClrImportant, 4, 1, f);

    /* Palette: black (index 0) and white (index 1) */
    uint8_t black[4] = {0, 0, 0, 0};
    uint8_t white[4] = {255, 255, 255, 0};
    fwrite(black, 4, 1, f);
    fwrite(white, 4, 1, f);

    /* Pixel data: bottom-up, 1=bright (white=fg), 0=black (bg) */
    uint8_t *row = malloc(row_size);
    if (!row) { fclose(f); return; }

    for (int y = IMG_H - 1; y >= 0; y--) {
        memset(row, 0, row_size);
        for (int x = 0; x < IMG_W; x++) {
#if EUI_COLOR_DEPTH == 1
            int idx = y * (IMG_W / 8) + x / 8;
            int bit = (img_buf[idx] >> (7 - (x % 8))) & 1;
#elif EUI_COLOR_DEPTH == 8
            int bit = img_buf[y * IMG_W + x] > 128 ? 1 : 0;
#else
            uint16_t *p16 = (uint16_t *)img_buf;
            int bit = p16[y * IMG_W + x] > 0 ? 1 : 0;
#endif
            if (bit) row[x / 8] |= (1 << (7 - (x % 8)));
        }
        fwrite(row, 1, row_size, f);
    }
    free(row);
    fclose(f);
    printf("  Written: %s (%d bytes)\n", filename, file_size);
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
    int line_height = profont10_font.line_height;

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

        /* Scan glyph buffer rows (cover full glyph + baseline offset) */
        for (int row = 0; row < line_height + 4; row++) {
            for (int col = 0; col < 8; col++) {
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
            cur_y += line_height + 4;
        }
    }

    printf("  Total chars: %d, render errors: %d\n", char_count, fail_count);
    printf("  Total pixels: %d\n", count_pixels_1bpp());

    write_bmp("font_render.bmp");

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
