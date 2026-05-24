#include "eui/eui_canvas.h"
#include "eui/eui_font.h"
#include "eui/eui_font_builtin.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

#define CANVAS_W 256
#define CANVAS_H 750

#if EUI_COLOR_DEPTH == 1
#define BUF_SIZE (CANVAS_W * CANVAS_H / 8)
#elif EUI_COLOR_DEPTH == 8
#define BUF_SIZE (CANVAS_W * CANVAS_H)
#else
#define BUF_SIZE (CANVAS_W * CANVAS_H * 2)
#endif

static uint8_t mock_buf[BUF_SIZE];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud;
#if EUI_COLOR_DEPTH == 1
    int bytes_per_row = r->w / 8;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * (CANVAS_W / 8) + r->x / 8),
               b + row * bytes_per_row, bytes_per_row);
    }
#elif EUI_COLOR_DEPTH == 8
    int bytes_per_row = r->w;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * CANVAS_W + r->x),
               b + row * bytes_per_row, bytes_per_row);
    }
#else
    int bytes_per_row = r->w * 2;
    for (int row = 0; row < (int)r->h; row++) {
        memcpy(mock_buf + ((r->y + row) * CANVAS_W * 2 + r->x * 2),
               b + row * bytes_per_row, bytes_per_row);
    }
#endif
}

static eui_display_drv_t mock_display = {
    .caps = { .width = CANVAS_W, .height = CANVAS_H, .color_depth = EUI_COLOR_DEPTH, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

/* ---- helpers for non-overlapping layout ---- */
/* built-in font: line_height=8, baseline=7.
 * Text drawn at y occupies rows [y-7, y].
 * We use _y as the baseline for each row's label,
 * and draw graphical content at y_ref below the label.
 * GAP    = blank rows between sections.
 * LABEL_GAP = extra rows between label text and its content.             */
#define FONT_H      8
#define GAP        10
#define LABEL_GAP   8

/* Draw a label then advance y past it (label + gap) */
#define LABEL(str) do { \
    eui_canvas_draw_str(c, 0, _y, str); \
    _y += FONT_H + LABEL_GAP; \
} while(0)

/* Advance _y so the next label sits FONT_H+GAP rows below content bottom. */
#define ADVANCE(y_ref, up, down) do { \
    int _bot = (y_ref) + (down); \
    int _target = _bot + FONT_H + GAP; \
    if (_target > _y) _y = _target; \
} while(0)

static int write_bmp(const char *filename)
{
    int row_bytes = (CANVAS_W + 7) / 8;
    int row_padded = (row_bytes + 3) & ~3;
    int pixel_data_size = row_padded * CANVAS_H;
    int file_size = 14 + 40 + 8 + pixel_data_size;

    uint8_t header[14];
    memset(header, 0, 14);
    header[0] = 'B';
    header[1] = 'M';
    header[2] = (uint8_t)(file_size);
    header[3] = (uint8_t)(file_size >> 8);
    header[4] = (uint8_t)(file_size >> 16);
    header[5] = (uint8_t)(file_size >> 24);
    header[10] = 62;

    uint8_t dib[40];
    memset(dib, 0, 40);
    dib[0] = 40;
    dib[4] = (uint8_t)(CANVAS_W);
    dib[5] = (uint8_t)(CANVAS_W >> 8);
    dib[8] = (uint8_t)(CANVAS_H);
    dib[9] = (uint8_t)(CANVAS_H >> 8);
    dib[12] = 1;
    dib[14] = 1;
    dib[36] = 2;

    uint8_t palette[8] = { 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0 };

    FILE *f = fopen(filename, "wb");
    if (!f) return -1;

    fwrite(header, 1, 14, f);
    fwrite(dib, 1, 40, f);
    fwrite(palette, 1, 8, f);

    for (int y = CANVAS_H - 1; y >= 0; y--) {
        uint8_t row_buf[64];
        memset(row_buf, 0, row_padded);
        for (int x = 0; x < CANVAS_W; x++) {
            int bi = y * (CANVAS_W / 8) + x / 8;
            int bp = x % 8;
            if ((mock_buf[bi] >> bp) & 1) {
                row_buf[x / 8] |= (1u << (7 - (x % 8)));
            }
        }
        fwrite(row_buf, 1, row_padded, f);
    }

    fclose(f);
    return 0;
}

static const uint8_t test_xbm_data[] = {
    0x80, 0x01, 0xC0, 0x03, 0xE0, 0x07, 0xF0, 0x0F,
    0xF8, 0x1F, 0xFC, 0x3F, 0xFE, 0x7F, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFE, 0x7F, 0xFC, 0x3F, 0xF8, 0x1F,
    0xF0, 0x0F, 0xE0, 0x07, 0xC0, 0x03, 0x80, 0x01,
};

static const uint8_t test_bmp_data[] = {
    0x00, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x3C, 0x00,
    0x3C, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const eui_bitmap_t test_bitmap = {
    .width = 16,
    .height = 16,
    .color_depth = 1,
    .data = test_bmp_data,
};

/* ---- Functional tests (no visual output) ---- */
static void test_lifecycle(void)
{
    TEST("eui_canvas_create");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) FAIL("create returned NULL");
    PASS();

    TEST("eui_canvas_width / height");
    uint16_t w = eui_canvas_width(c);
    uint16_t h = eui_canvas_height(c);
    if (w != CANVAS_W) FAIL("width mismatch");
    if (h != CANVAS_H) FAIL("height mismatch");
    PASS();

    TEST("eui_canvas_set_color");
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    if (c->fg_color != EUI_COLOR_WHITE) FAIL("fg not set");
    PASS();

    TEST("eui_canvas_set_bg_color");
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    if (c->bg_color != EUI_COLOR_BLACK) FAIL("bg not set");
    PASS();

    TEST("eui_canvas_set_font");
    eui_canvas_set_font(c, &eui_font_builtin);
    if (c->font != &eui_font_builtin) FAIL("font not set");
    PASS();

    TEST("eui_canvas_str_width / font_height");
    uint16_t sw = eui_canvas_str_width(c, "Hello");
    uint16_t fh = eui_canvas_font_height(c);
    if (sw != 40) FAIL("str_width should be 40 for 'Hello'");
    if (fh != 8) FAIL("font_height should be 8");
    PASS();

    TEST("eui_canvas_set_clip / clear_clip");
    eui_rect_t clip = { 10, 20, 100, 50 };
    eui_canvas_set_clip(c, &clip);
    if (c->clip.x != 10 || c->clip.w != 100) FAIL("clip not set");
    eui_canvas_clear_clip(c);
    if (c->clip.x != 0 || c->clip.w != CANVAS_W) FAIL("clip not cleared");
    PASS();

    TEST("eui_canvas_save / restore");
    eui_canvas_set_color(c, EUI_COLOR_BLACK);
    eui_canvas_save(c);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_restore(c);
    if (c->fg_color != EUI_COLOR_BLACK) FAIL("fg not restored");
    PASS();

    TEST("eui_canvas_reset");
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_reset(c);
    if (c->fg_color != EUI_COLOR_WHITE) FAIL("reset fg wrong");
    if (c->font != NULL) FAIL("reset font not NULL");
    PASS();

    eui_canvas_destroy(c);
    TEST("eui_canvas_destroy");
    PASS();
}

/* ---- Visual render test: each method gets an independent region ---- */
static void render_all_methods(eui_canvas_t *c, const char *bmp_path)
{
    int _y = 10;

    eui_canvas_set_font(c, &eui_font_builtin);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);

    /* ==================== Title ==================== */
    LABEL("eui_canvas Full API Render Test (256x750 1bpp)");
    LABEL("================================================");
    _y += GAP;

    /* ==================== Drawing Primitives ==================== */
    LABEL("=== Drawing Primitives ===");
    _y += GAP;

    /* --- draw_dot --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_dot: 8 dots at various positions");
        for (int i = 0; i < 8; i++)
            eui_canvas_draw_dot(c, 80 + i * 12, y0);
        ADVANCE(y0, 0, 0);
    }

    /* --- draw_line --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_line: diagonal + horizontal");
        eui_canvas_draw_line(c, 80, y0, 180, y0 - 6);
        eui_canvas_draw_line(c, 80, y0, 220, y0);
        ADVANCE(y0, 6, 0);
    }

    /* --- draw_rect + fill_rect --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_rect / fill_rect:");
        eui_canvas_draw_rect(c, 85, y0, 30, 16);
        eui_canvas_fill_rect(c, 135, y0, 30, 16);
        ADVANCE(y0, 0, 16);
    }

    /* --- draw_circle + fill_circle --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP + 8;                 /* center of circles */
        LABEL("draw_circle / fill_circle (r=8):");
        eui_canvas_draw_circle(c, 90, y0, 8);
        eui_canvas_fill_circle(c, 145, y0, 8);
        ADVANCE(y0, 8, 8);
    }

    /* --- draw_triangle + draw_round_rect --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP + 6;
        LABEL("draw_triangle / draw_round_rect:");
        eui_canvas_draw_triangle(c, 90, y0, 75, y0 - 12, 105, y0 - 12);
        eui_canvas_draw_round_rect(c, 140, y0 - 12, 30, 16, 5);
        ADVANCE(y0, 12, 0);
    }

    /* --- fill_round_rect --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("fill_round_rect:");
        eui_canvas_fill_round_rect(c, 85, y0, 60, 16, 5);
        ADVANCE(y0, 0, 16);
    }

    /* ==================== Text Operations ==================== */
    _y += GAP + 4;
    LABEL("=== Text Operations ===");
    _y += GAP;

    /* --- draw_str --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str:");
        eui_canvas_draw_str(c, 80, y0, "Hello, Canvas World!");
        ADVANCE(y0, 0, 0);
    }

    /* --- str_width / font_height --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "str_width: %d  font_height: %d  canvas: %dx%d",
                 eui_canvas_str_width(c, "Hello, Canvas World!"),
                 eui_canvas_font_height(c),
                 eui_canvas_width(c), eui_canvas_height(c));
        LABEL("str_width + font_height:");
        eui_canvas_draw_str(c, 80, y0, buf);
        ADVANCE(y0, 0, 0);
    }

    /* --- draw_str_aligned --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str_aligned (center):");
        eui_canvas_draw_str_aligned(c, 170, y0, EUI_ALIGN_CENTER, EUI_ALIGN_TOP, "Centered");
        ADVANCE(y0, 0, 0);
    }

    /* --- draw_str_clipped --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str_clipped:");
        eui_rect_t clip = { 90, (uint16_t)(y0 - 7), 64, 10 };
        eui_canvas_draw_str_clipped(c, &clip, 90, y0, "ClippedLongTextHere");
        ADVANCE(y0, 0, 0);
    }

    /* --- draw_str_ellipsis --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str_ellipsis:");
        eui_canvas_draw_str_ellipsis(c, 90, y0, "VeryLongTextThatExceeds", 80);
        ADVANCE(y0, 0, 0);
    }

#if EUI_FONT_ENABLE_MULTILINE
    /* --- draw_str_in_rect --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str_in_rect:");
        eui_rect_t r = { 90, (uint16_t)(y0), 80, 20 };
        eui_canvas_draw_rect(c, 90, y0, 80, 20);
        eui_canvas_draw_str_in_rect(c, &r, "InRect!", EUI_ALIGN_CENTER, EUI_ALIGN_MIDDLE);
        ADVANCE(y0, 0, 20);
    }

    /* --- draw_str_multiline --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_str_multiline:");
        eui_rect_t mr = { 90, (uint16_t)(y0), 120, 28 };
        eui_canvas_draw_rect(c, 90, y0, 120, 28);
        eui_canvas_draw_str_multiline(c, &mr, "Line1 Line2 Line3", 8, EUI_ALIGN_LEFT);
        ADVANCE(y0, 0, 28);
    }
#endif

    /* ==================== Images & Advanced ==================== */
    _y += GAP + 4;
    LABEL("=== Images & Advanced ===");
    _y += GAP;

    /* --- draw_xbm --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_xbm (16x16 arrow):");
        eui_canvas_draw_xbm(c, 85, y0, 16, 16, test_xbm_data);
        ADVANCE(y0, 0, 16);
    }

    /* --- draw_bitmap --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("draw_bitmap (16x16 face):");
        eui_canvas_draw_bitmap(c, 85, y0, &test_bitmap);
        ADVANCE(y0, 0, 16);
    }

    /* --- invert_rect --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("invert_rect:");
        eui_canvas_fill_rect(c, 85, y0, 64, 16);
        eui_canvas_invert_rect(c, 97, y0 + 2, 20, 12);
        eui_canvas_invert_rect(c, 125, y0 + 4, 16, 8);
        ADVANCE(y0, 0, 16);
    }

    /* --- set_clip / clear_clip visual --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("set_clip / clear_clip:");
        eui_rect_t cr = { 30, (uint16_t)(y0), 100, 16 };
        eui_canvas_set_clip(c, &cr);
        eui_canvas_fill_rect(c, 0, y0, 200, 16);
        eui_canvas_clear_clip(c);
        eui_canvas_draw_rect(c, 30, y0, 100, 16);
        ADVANCE(y0, 0, 16);
    }

    /* --- save / restore visual --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("save / restore:");
        eui_canvas_fill_rect(c, 30, y0, 80, 16);
        eui_canvas_save(c);
        eui_canvas_set_color(c, EUI_COLOR_BLACK);
        eui_canvas_fill_rect(c, 42, y0 + 2, 56, 12);
        eui_canvas_restore(c);
        eui_canvas_fill_rect(c, 54, y0 + 4, 32, 8);
        ADVANCE(y0, 0, 16);
    }

    /* --- begin_page / next_page --- */
    {
        int y0 = _y + FONT_H + LABEL_GAP;
        LABEL("begin_page / next_page:");
        bool ok = true;
        ok = ok && eui_canvas_begin_page(c);
        ok = ok && eui_canvas_next_page(c);
        eui_canvas_draw_str(c, 80, y0, "page API OK");
        (void)ok;
        ADVANCE(y0, 0, 0);
    }

    /* ---- Commit and write BMP ---- */
    memset(mock_buf, 0, sizeof(mock_buf));
    eui_canvas_commit(c);
    TEST("BMP file output");
    if (write_bmp(bmp_path) != 0) {
        FAIL("could not write BMP file");
    }
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== eui_canvas Full API Render Test ===\n");
    printf("Canvas: %dx%d, color_depth=%d, font=builtin(8x8)\n\n",
           CANVAS_W, CANVAS_H, EUI_COLOR_DEPTH);

    test_lifecycle();

    printf("\n--- Render Test (methods below produce visual output) ---\n");
    eui_canvas_t *c = eui_canvas_create(&mock_display);
    if (!c) { printf("FATAL: create failed\n"); return 1; }
    render_all_methods(c, "test_canvas_render.bmp");
    eui_canvas_destroy(c);

    return eui_test_summary();
}
