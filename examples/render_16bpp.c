/*
 * 16-bit color (RGB565) rendering example.
 * Self-contained — no library dependency.
 * Compile:  gcc -o render_16bpp examples/render_16bpp.c -lm
 * Run:      ./render_16bpp
 * Output:   render_16bpp.bmp  (320x400, 24-bit BMP)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define W 320
#define H 400

/* ---------- RGB565 pixel ---------- */
typedef uint16_t pixel_t;
static pixel_t fb[W * H];

#define RGB565(r, g, b)  ((pixel_t)((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3)))
#define MIN(a,b)         (((a)<(b))?(a):(b))
#define MAX(a,b)         (((a)>(b))?(a):(b))

/* ---------- pixel ops ---------- */
static void px(int x, int y, pixel_t c) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    fb[y * W + x] = c;
}

static pixel_t get_px(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return 0;
    return fb[y * W + x];
}

static void clear(pixel_t c) {
    for (int i = 0; i < W * H; i++) fb[i] = c;
}

/* ---------- 8×8 monospace bitmap font (ASCII 0x20-0x7F) ---------- */
static const uint8_t font8x8[128][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* space */
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},  /* ! */
    {0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},  /* " */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},  /* # */
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},  /* $ */
    {0x00,0x66,0xAC,0xD8,0x36,0x6A,0xCC,0x00},  /* % */
    {0x38,0x6C,0x68,0x76,0xDC,0xCC,0x76,0x00},  /* & */
    {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00},  /* ' */
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},  /* ( */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},  /* ) */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},  /* * */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},  /* + */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},  /* , */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},  /* - */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},  /* . */
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},  /* / */
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},  /* 0 */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},  /* 1 */
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},  /* 2 */
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},  /* 3 */
    {0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00},  /* 4 */
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},  /* 5 */
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00},  /* 6 */
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},  /* 7 */
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},  /* 8 */
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00},  /* 9 */
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},  /* : */
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},  /* ; */
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},  /* < */
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},  /* = */
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},  /* > */
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},  /* ? */
    {0x3C,0x66,0x6E,0x6E,0x60,0x66,0x3C,0x00},  /* @ */
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},  /* A */
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},  /* B */
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},  /* C */
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},  /* D */
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},  /* E */
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},  /* F */
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},  /* G */
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},  /* H */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},  /* I */
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},  /* J */
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},  /* K */
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},  /* L */
    {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},  /* M */
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},  /* N */
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},  /* O */
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},  /* P */
    {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0x00},  /* Q */
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},  /* R */
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},  /* S */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},  /* T */
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},  /* U */
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},  /* V */
    {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},  /* W */
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},  /* X */
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},  /* Y */
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},  /* Z */
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},  /* [ */
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00},  /* \ */
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},  /* ] */
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},  /* ^ */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},  /* _ */
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},  /* ` */
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},  /* a */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},  /* b */
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},  /* c */
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},  /* d */
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},  /* e */
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00},  /* f */
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},  /* g */
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},  /* h */
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},  /* i */
    {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x6C,0x38},  /* j */
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},  /* k */
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},  /* l */
    {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},  /* m */
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},  /* n */
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},  /* o */
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},  /* p */
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},  /* q */
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},  /* r */
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00},  /* s */
    {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},  /* t */
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},  /* u */
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},  /* v */
    {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00},  /* w */
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},  /* x */
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C},  /* y */
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},  /* z */
};

static void draw_str(int x, int y, const char *s, pixel_t fg) {
    for (; *s; s++) {
        int ch = (unsigned char)*s;
        if (ch < 32) ch = 32;        /* control → space */
        if (ch > 127) ch = 32;       /* non-ASCII → space */
        ch -= 32;                    /* font array starts at space */
        const uint8_t *g = font8x8[ch];
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (g[row] & (1 << (7 - col)))
                    px(x + col, y + row, fg);
        x += 8;
    }
}

/* ---------- kerning support ---------- */
typedef struct { char prev; char curr; int8_t adj; } kern_pair_t;
static const kern_pair_t kern_pairs[] = {
    {'T','A',-2}, {'T','o',-2}, {'T','e',-2}, {'T','u',-2},
    {'A','V',-2}, {'A','W',-2}, {'A','Y',-2}, {'A','v',-1},
    {'V','A',-2}, {'W','A',-2}, {'Y','A',-2},
    {'L','T',-1}, {'L','y',-2}, {'L','Y',-1},
    {'r','y',-2}, {'r','e',-1},
    {'Y','o',-1}, {'Y','u',-1},
    {'A','T',-1},
    {0,0,0}
};

static int8_t get_kern(char prev, char curr) {
    for (const kern_pair_t *kp = kern_pairs; kp->prev; kp++)
        if (kp->prev == prev && kp->curr == curr)
            return kp->adj;
    return 0;
}

static void draw_str_kern(int x, int y, const char *s, pixel_t fg) {
    char prev = 0;
    while (*s) {
        char cur = *s++;
        int ch = (unsigned char)cur;
        if (ch < 32) ch = 32;
        if (ch > 127) ch = 32;
        ch -= 32;
        if (prev) x += get_kern(prev, cur);
        const uint8_t *g = font8x8[ch];
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if (g[row] & (1 << (7 - col)))
                    px(x + col, y + row, fg);
        x += 8;
        prev = cur;
    }
}

/* ---------- drawing primitives ---------- */
static void draw_line(int x0, int y0, int x1, int y1, pixel_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        px(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_rect(int x, int y, int w, int h, pixel_t c) {
    draw_line(x, y, x + w - 1, y, c);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, c);
    draw_line(x, y + h - 1, x + w - 1, y + h - 1, c);
    draw_line(x, y, x, y + h - 1, c);
}

static void fill_rect(int x, int y, int w, int h, pixel_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            px(x + i, y + j, c);
}

static void draw_circle(int cx, int cy, int r, pixel_t c) {
    int f = 1 - r, ddF_x = 0, ddF_y = -2 * r, x = 0, y = r;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x + 1;
        px(cx + x, cy + y, c); px(cx - x, cy + y, c);
        px(cx + x, cy - y, c); px(cx - x, cy - y, c);
        px(cx + y, cy + x, c); px(cx - y, cy + x, c);
        px(cx + y, cy - x, c); px(cx - y, cy - x, c);
    }
}

static void fill_circle(int cx, int cy, int r, pixel_t c) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r)
                px(cx + x, cy + y, c);
}

static void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, pixel_t c) {
    draw_line(x0, y0, x1, y1, c);
    draw_line(x1, y1, x2, y2, c);
    draw_line(x2, y2, x0, y0, c);
}

static void draw_round_rect(int x, int y, int w, int h, int r, pixel_t c) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    int l = x + r, t = y + r, ri = x + w - r - 1, b = y + h - r - 1;
    draw_line(l, y, ri, y, c);
    draw_line(l, y + h - 1, ri, y + h - 1, c);
    draw_line(x, t, x, b, c);
    draw_line(x + w - 1, t, x + w - 1, b, c);
    int f = 1 - r, ddF_x = 0, ddF_y = -2 * r, cx = 0, cy = r;
    while (cx < cy) {
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x + 1;
        px(l - cx, t - cy, c); px(ri + cx, t - cy, c);
        px(l - cx, b + cy, c); px(ri + cx, b + cy, c);
        px(l - cy, t - cx, c); px(ri + cy, t - cx, c);
        px(l - cy, b + cx, c); px(ri + cy, b + cx, c);
    }
}

static void fill_round_rect(int x, int y, int w, int h, int r, pixel_t c) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    int l = x + r, t = y + r, ri = x + w - r - 1, b = y + h - r - 1;
    fill_rect(l, y, ri - l + 1, h, c);
    if (b > t) {
        fill_rect(x, t, r, b - t + 1, c);
        fill_rect(ri + 1, t, r, b - t + 1, c);
    }
    int f = 1 - r, ddF_x = 0, ddF_y = -2 * r, cx = 0, cy = r;
    while (cx < cy) {
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x + 1;
        draw_line(l - cx, t - cy, ri + cx, t - cy, c);
        draw_line(l - cx, b + cy, ri + cx, b + cy, c);
        draw_line(l - cy, t - cx, ri + cy, t - cx, c);
        draw_line(l - cy, b + cx, ri + cy, b + cx, c);
    }
}

static void draw_xbm(int x, int y, int w, int h, const uint8_t *data, pixel_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (data[j * ((w + 7) / 8) + i / 8] & (1u << (7 - i % 8)))
                px(x + i, y + j, c);
}

/* ---------- BMP writer (24-bit output) ---------- */
static int write_bmp(const char *fn) {
    int row_bytes = W * 3;
    int pad = (4 - row_bytes % 4) % 4;
    int stride = row_bytes + pad;
    int pix_off = 14 + 40;
    int file_sz = pix_off + stride * H;

    unsigned char hdr[14] = { 'B','M', 0,0,0,0, 0,0,0,0, 0,0,0,0 };
    hdr[2] = file_sz & 0xFF; hdr[3] = (file_sz>>8)&0xFF;
    hdr[4] = (file_sz>>16)&0xFF; hdr[5] = (file_sz>>24)&0xFF;
    hdr[10] = pix_off & 0xFF; hdr[11] = (pix_off>>8)&0xFF;

    unsigned char dib[40] = { 40,0,0,0 };
    dib[4] = W & 0xFF; dib[5] = (W>>8)&0xFF;
    dib[8] = H & 0xFF; dib[9] = (H>>8)&0xFF;
    dib[12] = 1; dib[14] = 24;

    FILE *f = fopen(fn, "wb");
    if (!f) return -1;
    fwrite(hdr, 1, 14, f);
    fwrite(dib, 1, 40, f);

    for (int y = H - 1; y >= 0; y--) {
        unsigned char row[W * 3 + 4];
        for (int x = 0; x < W; x++) {
            pixel_t p = fb[y * W + x];
            int r = ((p >> 11) & 0x1F) << 3;
            int g = ((p >> 5) & 0x3F) << 2;
            int b = (p & 0x1F) << 3;
            row[x * 3 + 0] = b;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = r;
        }
        fwrite(row, 1, stride, f);
    }
    fclose(f);
    return 0;
}

/* ---------- demo: gradient background ---------- */
static void draw_gradient() {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            fb[y * W + x] = RGB565(
                (x * 40) / W,
                (y * 30) / H,
                ((W - x) * 60 + x * 10) / W
            );
}

/* ================================================================== */
int main(void) {
    printf("Rendering 16bpp color demo (%dx%d)...\n", W, H);

    /* dark blue background */
    clear(RGB565(10, 10, 30));

    pixel_t white = RGB565(255, 255, 255);
    pixel_t yellow = RGB565(255, 255, 0);

    int y = 10;
    draw_str(0, y, "16-bit Color (RGB565) Render Demo", white); y += 14;

    /* ====== Drawing Primitives ====== */
    draw_str(0, y, "=== Drawing Primitives ===", yellow); y += 14;

    /* draw_dot: colored dots */
    draw_str(0, y, "draw_dot:", white);
    pixel_t dot_colors[] = {
        RGB565(255,0,0), RGB565(0,255,0), RGB565(0,0,255),
        RGB565(255,255,0), RGB565(255,0,255), RGB565(0,255,255),
    };
    for (int i = 0; i < 6; i++)
        px(75 + i * 20, y + 7, dot_colors[i]);
    y += 14;

    /* draw_line: colored lines */
    draw_str(0, y, "draw_line:", white);
    draw_line(75, y + 7, 180, y, RGB565(255, 100, 100));
    draw_line(75, y + 7, 200, y + 7, RGB565(100, 255, 100));
    draw_line(75, y + 7, 180, y + 14, RGB565(100, 100, 255));
    y += 16;

    /* draw_rect / fill_rect */
    draw_str(0, y, "draw_rect / fill_rect:", white);
    draw_rect(95, y, 30, 20, RGB565(255, 80, 80));
    fill_rect(140, y, 36, 20, RGB565(80, 200, 80));
    draw_rect(190, y, 30, 20, RGB565(80, 80, 255));
    y += 24;

    /* draw_circle / fill_circle */
    draw_str(0, y, "draw_circle / fill_circle:", white);
    draw_circle(95, y + 12, 12, RGB565(255, 200, 0));
    fill_circle(150, y + 12, 12, RGB565(200, 50, 200));
    draw_circle(210, y + 12, 12, RGB565(0, 200, 255));
    y += 28;

    /* draw_triangle / draw_round_rect */
    draw_str(0, y, "draw_triangle / draw_round_rect:", white);
    draw_triangle(100, y + 14, 75, y, 125, y, RGB565(255, 180, 0));
    draw_round_rect(145, y, 36, 20, 6, RGB565(0, 200, 180));
    y += 24;

    /* fill_round_rect */
    draw_str(0, y, "fill_round_rect:", white);
    fill_round_rect(85, y, 80, 22, 6, RGB565(60, 120, 255));
    y += 28;

    /* ====== Color Tests ====== */
    draw_str(0, y, "=== Color Tests ===", yellow); y += 14;

    /* Color bars */
    draw_str(0, y, "R/G/B/W gradients:", white);
    int bar_y = y;
    for (int i = 0; i < W - 85; i++) {
        int v = i * 255 / (W - 85);
        px(85 + i, bar_y + 0, RGB565(v, 0, 0));
        px(85 + i, bar_y + 8, RGB565(0, v, 0));
        px(85 + i, bar_y + 16, RGB565(0, 0, v));
        px(85 + i, bar_y + 24, RGB565(v, v, v));
    }
    y += 30;

    /* Color wheel dots */
    draw_str(0, y, "Color wheel dots:", white);
    for (int i = 0; i < 32; i++) {
        double ang = 3.14159 * 2 * i / 32;
        int r = (int)(128 + 127 * sin(ang));
        int g = (int)(128 + 127 * sin(ang + 2.094));
        int b = (int)(128 + 127 * sin(ang + 4.188));
        px(85 + i * 6, y + 7, RGB565(r, g, b));
    }
    y += 14;

    /* ====== Text with Kerning ====== */
    draw_str_kern(0, y, "=== Kerning Demo (8x8 font) ===", yellow); y += 14;
    draw_str(0, y, "No kerning:  TAVATAR  AVERY  YEARLY", RGB565(200, 200, 200)); y += 10;
    draw_str_kern(0, y, "With kerning: TAVATAR  AVERY  YEARLY", RGB565(255, 255, 100)); y += 12;
    draw_str_kern(0, y, "Hello, 16-bit Color World!", RGB565(255, 200, 100)); y += 12;
    draw_str_kern(0, y, "RGB565: Red Green Blue Cyan Magenta Yellow",
                  RGB565(180, 200, 255)); y += 12;

    /* ====== XBM ====== */
    draw_str(0, y, "=== XBM Icon ===", yellow); y += 14;
    static const uint8_t heart[] = {
        0x00,0x00,0x00, 0x03,0x00,0xC0, 0x0F,0x00,0xF0,
        0x3F,0xC0,0xFC, 0x7F,0xF0,0xFE, 0xFF,0xF8,0xFF,
        0xFF,0xFC,0xFF, 0xFF,0xFC,0xFF, 0x7F,0xF8,0xFE,
        0x3F,0xF0,0xFC, 0x0F,0xC0,0xF0, 0x03,0x00,0xC0,
        0x00,0x00,0x00,
    };
    draw_xbm(10, y, 32, 13, heart, RGB565(255, 80, 120));
    draw_str(50, y + 2, "Heart icon in pink", RGB565(255, 150, 180));
    y += 18;

    /* additional color fills */
    draw_str(0, y, "Filled shapes (various colors):", white); y += 14;
    fill_rect(10, y, 30, 20, RGB565(200, 50, 50));
    fill_rect(50, y, 30, 20, RGB565(50, 200, 50));
    fill_rect(90, y, 30, 20, RGB565(50, 50, 200));
    fill_rect(130, y, 30, 20, RGB565(200, 200, 50));
    fill_rect(170, y, 30, 20, RGB565(200, 50, 200));
    fill_rect(210, y, 30, 20, RGB565(50, 200, 200));
    fill_rect(250, y, 30, 20, RGB565(200, 200, 200));
    y += 24;

    draw_str(0, y, "Circles with outlines + fills:", white); y += 14;
    fill_circle(40, y + 12, 12, RGB565(180, 50, 180));
    draw_circle(40, y + 12, 12, RGB565(255, 200, 0));
    fill_circle(90, y + 12, 12, RGB565(50, 180, 180));
    draw_circle(90, y + 12, 12, RGB565(255, 200, 0));
    fill_circle(140, y + 12, 12, RGB565(180, 180, 50));
    draw_circle(140, y + 12, 12, RGB565(255, 200, 0));
    y += 28;

    /* Write BMP */
    if (write_bmp("render_16bpp.bmp") == 0)
        printf("  -> render_16bpp.bmp (%d x %d, 24-bit BMP)\n", W, H);
    else
        printf("  FAIL: could not write BMP\n");

    return 0;
}
