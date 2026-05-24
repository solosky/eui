#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <file.png>\n", argv[0]); return 1; }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(f); return 1; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); fclose(f); return 1; }

    png_init_io(png, f);
    png_read_info(png, info);

    int w = png_get_image_width(png, info);
    int h = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    printf("PNG: %dx%d, color_type=%d, bit_depth=%d\n", w, h, color_type, bit_depth);

    /* Convert to 8-bit RGB */
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    int rowbytes = png_get_rowbytes(png, info);
    png_bytep *rows = malloc(h * sizeof(png_bytep));
    png_bytep data = malloc(h * rowbytes);
    for (int y = 0; y < h; y++) rows[y] = data + y * rowbytes;
    png_read_image(png, rows);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);

    /* Now data is RGBA, analyze */
    int white_count = 0;
    for (int i = 0; i < w * h; i++) {
        unsigned char *p = data + i * 4;
        if (p[0] >= 250 && p[1] >= 250 && p[2] >= 250) white_count++;
    }
    printf("White pixels: %d / %d (%.1f%%)\n",
           white_count, w * h, 100.0f * white_count / (w * h));

    printf("\nHorizontal stripes (rows >80%% white):\n");
    int stripe_rows = 0;
    for (int y = 0; y < h; y++) {
        int row_white = 0;
        for (int x = 0; x < w; x++) {
            unsigned char *p = data + (y * w + x) * 4;
            if (p[0] >= 250 && p[1] >= 250 && p[2] >= 250) row_white++;
        }
        if (row_white > w * 80 / 100) {
            printf("  row %3d: %d/%d white (%.0f%%)\n",
                   y, row_white, w, 100.0f * row_white / w);
            stripe_rows++;
        }
    }
    if (stripe_rows == 0) printf("  (none)\n");

    printf("\nVertical stripes (cols >80%% white):\n");
    int stripe_cols = 0;
    for (int x = 0; x < w; x++) {
        int col_white = 0;
        for (int y = 0; y < h; y++) {
            unsigned char *p = data + (y * w + x) * 4;
            if (p[0] >= 250 && p[1] >= 250 && p[2] >= 250) col_white++;
        }
        if (col_white > h * 80 / 100) {
            printf("  col %3d: %d/%d white (%.0f%%)\n",
                   x, col_white, h, 100.0f * col_white / h);
            stripe_cols++;
        }
    }
    if (stripe_cols == 0) printf("  (none)\n");

    printf("\nASCII preview (first 20 rows, every 2nd pixel):\n");
    for (int y = 0; y < 20 && y < h; y++) {
        printf("%3d: ", y);
        for (int x = 0; x < w; x += 2) {
            unsigned char *p = data + (y * w + x) * 4;
            int b = (p[0] + p[1] + p[2]) / 3;
            if (b > 200) putchar('#');
            else if (b > 100) putchar('+');
            else if (b > 40) putchar('.');
            else putchar(' ');
        }
        putchar('\n');
    }

    free(rows);
    free(data);
    return 0;
}
