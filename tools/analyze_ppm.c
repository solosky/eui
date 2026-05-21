#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <screenshot.ppm>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }

    char magic[3];
    int w, h, maxval;
    fscanf(f, "%2s %d %d %d", magic, &w, &h, &maxval);
    fgetc(f); /* consume newline after maxval */

    if (strcmp(magic, "P6") != 0) {
        fprintf(stderr, "Not a P6 PPM file (got '%s')\n", magic);
        fclose(f);
        return 1;
    }

    printf("Image: %dx%d, maxval=%d\n", w, h, maxval);

    unsigned char *data = malloc(w * h * 3);
    fread(data, 1, w * h * 3, f);
    fclose(f);

    /* Count white pixels (R,G,B all >= 250) */
    int white_count = 0;
    for (int i = 0; i < w * h; i++) {
        if (data[i*3] >= 250 && data[i*3+1] >= 250 && data[i*3+2] >= 250)
            white_count++;
    }
    printf("White pixels: %d / %d (%.1f%%)\n",
           white_count, w * h, 100.0f * white_count / (w * h));

    /* Check each row for horizontal white stripes (>80% white) */
    printf("\nHorizontal stripes (rows with >80%% white):\n");
    int stripe_rows = 0;
    for (int y = 0; y < h; y++) {
        int row_white = 0;
        for (int x = 0; x < w; x++) {
            unsigned char *p = data + (y * w + x) * 3;
            if (p[0] >= 250 && p[1] >= 250 && p[2] >= 250) row_white++;
        }
        if (row_white > w * 80 / 100) {
            printf("  row %3d: %d/%d white (%.0f%%)\n",
                   y, row_white, w, 100.0f * row_white / w);
            stripe_rows++;
        }
    }
    if (stripe_rows == 0) printf("  (none)\n");

    /* Check each column for vertical white stripes */
    printf("\nVertical stripes (columns with >80%% white):\n");
    int stripe_cols = 0;
    for (int x = 0; x < w; x++) {
        int col_white = 0;
        for (int y = 0; y < h; y++) {
            unsigned char *p = data + (y * w + x) * 3;
            if (p[0] >= 250 && p[1] >= 250 && p[2] >= 250) col_white++;
        }
        if (col_white > h * 80 / 100) {
            printf("  col %3d: %d/%d white (%.0f%%)\n",
                   x, col_white, h, 100.0f * col_white / h);
            stripe_cols++;
        }
    }
    if (stripe_cols == 0) printf("  (none)\n");

    /* Edge analysis: right edge vs left edge (GL_REPEAT check) */
    printf("\nEdge analysis (checking for GL_REPEAT wrap):\n");
    int edge_mismatch = 0;
    for (int y = 0; y < h; y++) {
        unsigned char *right = data + (y * w + w - 1) * 3;
        unsigned char *prev  = data + (y * w + w - 2) * 3;
        unsigned char *left  = data + (y * w) * 3;
        /* If right matches left but NOT prev, that's a wrap artifact */
        int right_left_diff = abs(right[0] - left[0]) + abs(right[1] - left[1]) + abs(right[2] - left[2]);
        int right_prev_diff = abs(right[0] - prev[0]) + abs(right[1] - prev[1]) + abs(right[2] - prev[2]);
        if (right_left_diff < 10 && right_prev_diff > 50) {
            edge_mismatch++;
        }
    }
    printf("  Right edge wrap artifacts: %d rows\n", edge_mismatch);

    int top_mismatch = 0;
    for (int x = 0; x < w; x++) {
        unsigned char *top    = data + x * 3;
        unsigned char *next   = data + (w + x) * 3;
        unsigned char *bottom = data + ((h - 1) * w + x) * 3;
        int top_bottom_diff = abs(top[0] - bottom[0]) + abs(top[1] - bottom[1]) + abs(top[2] - bottom[2]);
        int top_next_diff   = abs(top[0] - next[0]) + abs(top[1] - next[1]) + abs(top[2] - next[2]);
        if (top_bottom_diff < 10 && top_next_diff > 50) {
            top_mismatch++;
        }
    }
    printf("  Top edge wrap artifacts: %d columns\n", top_mismatch);

    /* ASCII art preview of first 20 rows */
    printf("\nASCII preview (first 20 rows, every 4th pixel):\n");
    for (int y = 0; y < 20 && y < h; y++) {
        printf("%3d: ", y);
        for (int x = 0; x < w; x += 4) {
            unsigned char *p = data + (y * w + x) * 3;
            int bright = (p[0] + p[1] + p[2]) / 3;
            if (bright > 200)      putchar('#');
            else if (bright > 100) putchar('+');
            else if (bright > 50)  putchar('.');
            else                    putchar(' ');
        }
        putchar('\n');
    }

    free(data);
    return 0;
}
