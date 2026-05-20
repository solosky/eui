#include <stdint.h>

#include "test_u8g2_wqy12_ch1_data.h"

#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "eui/eui_types.h"
#include "../src/eui_font_u8g2_internal.h"
#include "../src/eui_font_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define POOL_SIZE 65536
static uint8_t mem_pool[POOL_SIZE];

static const eui_font_t wqy12_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 12,
    .baseline = 10,
    .flags = 0,
    .data = u8g2_font_wqy12_ch1_data,
    .lookup_glyph = NULL,
};

#define IMG_W 800
#define IMG_H 900

#if EUI_COLOR_DEPTH == 1
#define BUF_SIZE (IMG_W * IMG_H / 8)
#else
#define BUF_SIZE (IMG_W * IMG_H)
#endif

static uint8_t img_buf[BUF_SIZE];

static uint16_t font_get_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static int glyph_stride = 2;

static void render_glyph(int img_x, int img_y, uint16_t encoding)
{
    uint8_t buf[256] = {0};
    uint8_t adv = eui_font_u8g2_draw_glyph(&wqy12_font, encoding, buf, glyph_stride, 1);
    if (adv == 0) return;

    for (int r = 0; r < wqy12_font.line_height + 4; r++) {
        for (int b = 0; b < glyph_stride; b++) {
            uint8_t byte = buf[r * glyph_stride + b];
            if (byte == 0) continue;
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << (7 - bit))) {
                    int px = img_y + r;
                    int py = img_x + b * 8 + bit;
                    if (px >= 0 && px < IMG_H && py >= 0 && py < IMG_W) {
                        int idx = px * (IMG_W / 8) + py / 8;
                        img_buf[idx] |= (1 << (7 - (py % 8)));
                    }
                }
            }
        }
    }
}

static int discover_8bit_glyphs(const eui_font_t *font, uint8_t *encodings, int max_count)
{
    const uint8_t *p = font->data;
    int count = 0;
    int upper_a_off = font_get_be16(p + 17);
    int lower_a_off = font_get_be16(p + 19);
    int sections[3][2] = {
        {23, 23 + upper_a_off},
        {23 + upper_a_off, 23 + lower_a_off},
        {23 + lower_a_off, 65535}
    };
    for (int s = 0; s < 3 && count < max_count; s++) {
        int off = sections[s][0];
        int end = sections[s][1];
        while (off + 1 < end && count < max_count) {
            uint8_t enc = p[off];
            uint8_t size = p[off + 1];
            if (size == 0) break;
            encodings[count++] = enc;
            off += size;
        }
    }
    return count;
}

static int discover_unicode_glyphs(const eui_font_t *font, uint16_t *codes, int max_count)
{
    const uint8_t *p = font->data;
    uint16_t unicode_off = font_get_be16(p + 21);
    if (unicode_off == 0) return 0;

    const uint8_t *jump_table = p + 23 + unicode_off;
    const uint8_t *lt = jump_table;
    int count = 0;

    for (;;) {
        uint16_t block_off = font_get_be16(lt);
        uint16_t last_unicode = font_get_be16(lt + 2);
        if (last_unicode == 0xFFFF) break;

        const uint8_t *block = jump_table + block_off;
        lt += 4;
        uint16_t next_off = font_get_be16(lt);
        const uint8_t *block_end = jump_table + next_off;

        const uint8_t *entry = block;
        while (entry + 3 <= block_end) {
            uint16_t code = font_get_be16(entry);
            uint8_t jump = entry[2];
            if (jump < 3 || jump > 60) break;
            if (count < max_count) codes[count++] = code;
            entry += jump;
        }
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

    uint16_t bfType = 0x4D42;
    uint32_t bfSize = file_size;
    uint32_t bfOffBits = offset;
    fwrite(&bfType, 2, 1, f);
    fwrite(&bfSize, 4, 1, f);
    uint32_t zero32 = 0;
    fwrite(&zero32, 4, 1, f);
    fwrite(&bfOffBits, 4, 1, f);

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

    uint8_t black[4] = {0, 0, 0, 0};
    uint8_t white[4] = {255, 255, 255, 0};
    fwrite(black, 4, 1, f);
    fwrite(white, 4, 1, f);

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
    memset(img_buf, 0, sizeof(img_buf));

    int margin_x = 12;
    int margin_y = 12;
    int cell_w = 18;
    int cell_h = 20;

    /* Section 1: all 8-bit glyphs */
    uint8_t encodings[256];
    int n8 = discover_8bit_glyphs(&wqy12_font, encodings, 256);
    printf("8-bit glyphs: %d\n", n8);

    int chars_per_row = 16;
    int cur_y = margin_y;
    int cur_x;
    int count = 0;

    for (int i = 0; i < n8; i++) {
        int col = count % chars_per_row;
        int row = count / chars_per_row;
        cur_x = margin_x + col * cell_w;
        cur_y = margin_y + row * cell_h;
        render_glyph(cur_x, cur_y, encodings[i]);
        count++;
    }

    /* Section 2: all unicode glyphs */
    uint16_t codes[256];
    int nu = discover_unicode_glyphs(&wqy12_font, codes, 256);
    printf("Unicode glyphs: %d\n", nu);

    int n8_rows = (n8 + chars_per_row - 1) / chars_per_row;
    cur_y = margin_y + n8_rows * cell_h + 8;
    chars_per_row = 10;
    count = 0;

    for (int i = 0; i < nu; i++) {
        int col = count % chars_per_row;
        int row = count / chars_per_row;
        cur_x = margin_x + col * cell_w;
        cur_y = margin_y + n8_rows * cell_h + 8 + row * cell_h;
        render_glyph(cur_x, cur_y, codes[i]);
        count++;
    }

    printf("Total: %d + %d = %d glyphs rendered\n", n8, nu, n8 + nu);
    write_bmp("font_wqy12_chinese.bmp");
    printf("PASS\n");
    return 0;
}
