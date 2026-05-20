#include <stdint.h>

#include "test_u8g2_profont10_data.h"
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

static const eui_font_t profont10_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 10,
    .baseline = 8,
    .flags = 0,
    .data = u8g2_font_profont10_tf_data,
    .lookup_glyph = NULL,
};

static const eui_font_t wqy12_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 12,
    .baseline = 10,
    .flags = 0,
    .data = u8g2_font_wqy12_ch1_data,
    .lookup_glyph = NULL,
};

#define IMG_W 800
#define IMG_H 400

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

static uint16_t font_get_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static void render_glyph_buf(const uint8_t *glyph_buf, uint8_t buf_stride,
                              int img_x, int img_y, int num_rows)
{
    for (int row = 0; row < num_rows; row++) {
        for (int byte_off = 0; byte_off < buf_stride; byte_off++) {
            uint8_t byte = glyph_buf[row * buf_stride + byte_off];
            if (byte == 0) continue;
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << (7 - bit))) {
                    int px = img_y + row;
                    int py = img_x + byte_off * 8 + bit;
                    if (px >= 0 && px < IMG_H && py >= 0 && py < IMG_W) {
                        int idx = px * (IMG_W / 8) + py / 8;
                        img_buf[idx] |= (1 << (7 - (py % 8)));
                    }
                }
            }
        }
    }
}

static int discover_font_glyphs(const eui_font_t *font, uint8_t *encodings, int max_count)
{
    const uint8_t *p = font->data;
    int count = 0;
    int upper_a_off = font_get_be16(p + 17);
    int lower_a_off = font_get_be16(p + 19);
    int unicode_off = font_get_be16(p + 21);
    /* Offsets in header are relative to index base (byte 23) */
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
    const uint8_t *glyph_ptr = jump_table;
    int count = 0;

    for (;;) {
        uint16_t block_off = font_get_be16(lt);
        uint16_t last_unicode = font_get_be16(lt + 2);
        glyph_ptr += block_off;
        lt += 4;

        if (last_unicode == 0xFFFF) break;

        uint16_t next_off = font_get_be16(lt);
        const uint8_t *block_end = glyph_ptr + next_off;

        const uint8_t *entry = glyph_ptr;
        while (entry + 3 <= block_end) {
            uint16_t code = font_get_be16(entry);
            uint8_t jump = entry[2];
            if (jump < 3) break;
            if (count < max_count) {
                codes[count++] = code;
            }
            entry += jump;
        }
    }
    return count;
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

    /* === Diagnostic: Print font header values === */
    {
        const uint8_t *fd = profont10_font.data;
        printf("=== Font Header Diagnostics ===\n");
        printf("  glyph_cnt:        %d\n", fd[0]);
        printf("  bbx_mode:         %d\n", fd[1]);
        printf("  bits_per_0:       %d\n", fd[2]);
        printf("  bits_per_1:       %d\n", fd[3]);
        printf("  bits_per_char_w:  %d\n", fd[4]);
        printf("  bits_per_char_h:  %d\n", fd[5]);
        printf("  bits_per_char_x:  %d\n", fd[6]);
        printf("  bits_per_char_y:  %d\n", fd[7]);
        printf("  bits_per_delta_x: %d\n", fd[8]);
        printf("  max_char_width:   %d\n", fd[9]);
        printf("  max_char_height:  %d\n", fd[10]);
        printf("  hdr_x_offset:     %d (0x%02X as int8)\n", (int8_t)fd[11], fd[11]);
        printf("  hdr_y_offset:     %d (0x%02X as int8)\n", (int8_t)fd[12], fd[12]);
        printf("  ascent_A:         %d (0x%02X)\n", (int8_t)fd[13], fd[13]);
        printf("  descent_g:        %d (0x%02X)\n", (int8_t)fd[14], fd[14]);
        printf("  ascent_para:      %d (0x%02X)\n", (int8_t)fd[15], fd[15]);
        printf("  descent_para:     %d (0x%02X)\n", (int8_t)fd[16], fd[16]);
        printf("  baseline:         %d\n", profont10_font.baseline);
        printf("  line_height:      %d\n", profont10_font.line_height);
    }

    /* === Diagnostic: Per-glyph metrics for key characters === */
    {
        const char *test_chars = "Aabcdeghjpy";
        printf("\n=== Per-Glyph Metrics (Key Chars) ===\n");
        printf("  Char | glyph_w | glyph_h | x_off | y_off | row0(b-h-yo) | top_row | last_row\n");
        printf("  -----|---------|---------|-------|-------|--------------|---------|---------\n");
        for (const char *cp = test_chars; *cp; cp++) {
            uint16_t off = eui_font_u8g2_lookup_glyph(&profont10_font, (uint8_t)*cp, 0);
            if (off == (uint16_t)-1) {
                printf("  '%c'  NOT FOUND\n", *cp);
                continue;
            }
            u8g2_glyph_t g = {0};
            decode_glyph_at(&profont10_font, off, &g);
            int16_t row0 = (int16_t)profont10_font.baseline - (int16_t)g.height - (int16_t)g.y_offset;
            int16_t last_row = row0 + (int16_t)g.height - 1;
            printf("  '%c'  | %7d | %7d | %5d | %5d | %11d | %4d | %4d\n",
                   *cp, g.width, g.height, g.x_offset, g.y_offset,
                   row0, row0, last_row);
        }
    }

    /* === Diagnostic: Show which rows have pixels for 'a' vs 'b' === */
    {
        printf("\n=== Pixel Row Map for 'a' and 'b' ===\n");
        char cs[2] = {'a', 'b'};
        for (int i = 0; i < 2; i++) {
            uint8_t gbuf[128] = {0};
            eui_font_draw_char(&profont10_font, cs[i], gbuf, 1, 1);
            printf("  '%c' rows with pixels: ", cs[i]);
            int has_pixel = 0;
            for (int row = 0; row < 20; row++) {
                if (gbuf[row] != 0) {
                    printf("%d ", row);
                    has_pixel = 1;
                }
            }
            if (!has_pixel) printf("(none)");
            printf("\n");
        }
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

    int profont_fail = fail_count;
    int profont_count = char_count;

    printf("  Total chars: %d, render errors: %d\n", profont_count, profont_fail);
    printf("  Total pixels: %d\n", count_pixels_1bpp());

    /* === Wqy12 Chinese Font Test === */
    printf("\n=== Wqy12 (WenQuanYi 12px) Chinese Font Test ===\n");

    const uint8_t *fd = wqy12_font.data;
    printf("  glyph_cnt:        %d\n", fd[0]);
    printf("  max_char_width:   %d\n", fd[9]);
    printf("  max_char_height:  %d\n", fd[10]);
    printf("  line_height:      %d\n", wqy12_font.line_height);
    printf("  baseline:         %d\n", wqy12_font.baseline);

    uint8_t encodings[256];
    int total_glyphs = discover_font_glyphs(&wqy12_font, encodings, 256);
    printf("  Total glyphs found: %d\n", total_glyphs);

    uint16_t unicode_codes[256];
    int total_unicode = discover_unicode_glyphs(&wqy12_font, unicode_codes, 256);
    printf("  Unicode glyphs found: %d\n", total_unicode);

    int ascii_printable = 0, other = 0;
    for (int i = 0; i < total_glyphs; i++) {
        if (encodings[i] >= 32 && encodings[i] <= 126) ascii_printable++;
        else other++;
    }
    printf("  ASCII printable: %d, Extended/CJK: %d\n", ascii_printable, other + total_unicode);

    if (total_unicode > 0) {
        printf("  CJK unicode code points (first 20):");
        for (int i = 0; i < (total_unicode < 20 ? total_unicode : 20); i++) {
            if (i % 8 == 0) printf("\n    ");
            printf("U+%04X ", unicode_codes[i]);
        }
        printf("\n");
    }

    {
        printf("  Non-ASCII encoding values:");
        int printed = 0;
        for (int i = 0; i < total_glyphs; i++) {
            if (encodings[i] < 32 || encodings[i] > 126) {
                if (printed % 16 == 0) printf("\n    ");
                printf("0x%02X ", encodings[i]);
                printed++;
            }
        }
        if (printed == 0) printf(" (none)");
        printf("\n");
    }

    cur_x = 10;
    cur_y = 140;
    chars_per_row = 25;
    char_count = 0;
    fail_count = 0;

    /* Render all 8-bit glyphs (ASCII + extended) */
    {
        int glyph_stride = 2;
        for (int i = 0; i < total_glyphs; i++) {
            uint8_t enc = encodings[i];
            uint8_t cw = eui_font_get_char_width(&wqy12_font, (char)enc);
            if (cw == 0) {
                printf("  Enc 0x%02X (glyph %d/%d) has 0 width\n", enc, i+1, total_glyphs);
                fail_count++;
                continue;
            }

            uint8_t glyph_buf[256] = {0};
            uint8_t adv = eui_font_draw_char(&wqy12_font, (char)enc,
                                              glyph_buf, glyph_stride, 1);
            if (adv == 0) {
                printf("  Enc 0x%02X (glyph %d/%d) draw returned 0\n", enc, i+1, total_glyphs);
                fail_count++;
                continue;
            }

            render_glyph_buf(glyph_buf, glyph_stride, cur_x, cur_y,
                             wqy12_font.line_height + 4);

            cur_x += adv + 2;
            char_count++;
            if (char_count % chars_per_row == 0) {
                cur_x = 10;
                cur_y += wqy12_font.line_height + 4;
            }
        }
    }

    /* Render CJK unicode glyphs */
    if (total_unicode > 0) {
        cur_x = 10;
        cur_y += 4;
    }

    int unicode_rendered = 0;
    {
        int glyph_stride = 2;
        for (int i = 0; i < total_unicode; i++) {
            uint16_t code = unicode_codes[i];

            uint8_t glyph_buf[256] = {0};
            uint8_t adv = eui_font_u8g2_draw_glyph(&wqy12_font, code,
                                                    glyph_buf, glyph_stride, 1);
            if (adv == 0) {
                printf("  Warning: U+%04X draw returned 0 (placeholder glyph)\n", code);
                continue;
            }

            render_glyph_buf(glyph_buf, glyph_stride, cur_x, cur_y,
                             wqy12_font.line_height + 4);

            cur_x += adv + 2;
            unicode_rendered++;
            char_count++;
            if (char_count % chars_per_row == 0) {
                cur_x = 10;
                cur_y += wqy12_font.line_height + 4;
            }
        }
    }

    printf("  Wqy12 chars rendered: %d, errors: %d\n", char_count, fail_count);
    printf("  Total pixels (both fonts): %d\n", count_pixels_1bpp());

    write_bmp("font_render.bmp");

    if (count_pixels_1bpp() == 0) {
        printf("FAIL: no pixels rendered\n");
        return 1;
    }
    if (profont_fail > 0 || fail_count > 0) {
        printf("FAIL: profont10 errors=%d, wqy12 errors=%d\n", profont_fail, fail_count);
        return 1;
    }
    printf("PASS: profont10=%d chars + wqy12=%d chars = %d total, all rendered successfully\n",
           profont_count, char_count, profont_count + char_count);
    return 0;
}
