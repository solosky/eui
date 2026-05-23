#include "eui/eui.h"
#include "eui/eui_view.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 65536
static uint8_t mem_pool[POOL_SIZE];

#define TW 128
#define TH 64

#if EUI_COLOR_DEPTH == 1
#define BUF_SIZE (TW * TH / 8)
#define COVER_COLOR 1
#define CLEAR_COLOR 0
#elif EUI_COLOR_DEPTH == 16
#define BUF_SIZE (TW * TH * 2)
#define COVER_COLOR 0xFFFF
#define CLEAR_COLOR 0
#else
#define BUF_SIZE (TW * TH)
#define COVER_COLOR 0xFF
#define CLEAR_COLOR 0
#endif

static uint8_t mock_buf[BUF_SIZE];
static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud) {
    (void)ud;
#if EUI_COLOR_DEPTH == 1
    int bpr = (int)r->w / 8;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * (TW / 8) + r->x / 8), b + row * bpr, bpr);
#elif EUI_COLOR_DEPTH == 16
    int bpr = (int)r->w * 2;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * TW * 2 + r->x * 2), b + row * bpr, bpr);
#else
    int bpr = (int)r->w;
    for (int row = 0; row < (int)r->h; row++)
        memcpy(mock_buf + ((r->y + row) * TW + r->x), b + row * bpr, bpr);
#endif
}

static eui_display_drv_t mock_display = {
    .caps = { .width = TW, .height = TH, .color_depth = EUI_COLOR_DEPTH,
              .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

static int tests_run = 0, tests_passed = 0;
#define TEST(n)  do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS()   do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m)  do { printf("FAIL: %s\n", m); return; } while(0)

static uint32_t g_tick_ms = 0;
static uint32_t test_get_tick(void) { return g_tick_ms; }

static bool cover_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    if (evt->type == EUI_VIEW_EVT_DRAW) {
        eui_canvas_t *c = evt->event.draw.canvas;
        eui_canvas_set_color(c, COVER_COLOR);
        eui_canvas_fill_rect(c, view->area.x, view->area.y,
                                view->area.w, view->area.h);
        return true;
    }
    return false;
}

static int count_clear_pixels(void) {
    int count = 0;
#if EUI_COLOR_DEPTH == 1
    for (int y = 0; y < TH; y++) {
        for (int x = 0; x < TW; x++) {
            int byte_idx = y * (TW / 8) + (x / 8);
            int bit_pos  = x % 8;
            if (((mock_buf[byte_idx] >> bit_pos) & 1) == CLEAR_COLOR) count++;
        }
    }
#elif EUI_COLOR_DEPTH == 16
    uint16_t *p = (uint16_t*)mock_buf;
    for (int i = 0; i < TW * TH; i++)
        if (p[i] == CLEAR_COLOR) count++;
#else
    for (int i = 0; i < TW * TH; i++)
        if (mock_buf[i] == CLEAR_COLOR) count++;
#endif
    return count;
}

/* Run one transition frame.  Returns number of uncovered pixels (0 = perfect). */
static int check_transition(eui_anim_type_t anim, uint32_t elapsed_ms) {
    memset(mock_buf, 0xFF, sizeof(mock_buf)); /* poison: any leftover 0xFF = bug */

    eui_view_dispatcher_t vd;
    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    if (!canvas) return -1;

    eui_view_dispatcher_init(&vd, canvas, test_get_tick);

    eui_view_t v1, v2;
    eui_view_init(&v1, cover_view_handler, &v1);
    v1.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_init(&v2, cover_view_handler, &v2);
    v2.area = (eui_rect_t){ 0, 0, TW, TH };

    eui_view_dispatcher_add(&vd, 1, &v1);
    eui_view_dispatcher_add(&vd, 2, &v2);

    vd.transitioning        = true;
    vd.transition_type      = anim;
    vd.transition_prev_view = &v1;
    vd.current_view_idx     = 1;
    g_tick_ms               = 1000 + elapsed_ms;
    vd.transition_start_ms  = 1000;

    eui_view_dispatcher_tick(&vd);

    int clear = count_clear_pixels();
    eui_canvas_destroy(canvas);
    return clear;
}

static const char *anim_name(eui_anim_type_t a) {
    switch (a) {
    case EUI_ANIM_SLIDE_LEFT:  return "SLIDE_LEFT";
    case EUI_ANIM_SLIDE_RIGHT: return "SLIDE_RIGHT";
    case EUI_ANIM_SLIDE_UP:    return "SLIDE_UP";
    case EUI_ANIM_FADE:        return "FADE";
    default:                   return "?";
    }
}

static int test_all_progress(eui_anim_type_t anim, int duration_ms) {
    int worst = 0;
    for (int e = 10; e <= duration_ms; e += 10) {
        int c = check_transition(anim, (uint32_t)e);
        if (c < 0) return -1;
        if (c > worst) worst = c;
    }
    return worst;
}

/* ---- individual tests ---- */
static void test_slide_left_gap(void) {
    TEST("SLIDE_LEFT no uncovered pixels across all progress");
    int worst = test_all_progress(EUI_ANIM_SLIDE_LEFT, 300);
    if (worst < 0) FAIL("allocation failed");
    if (worst > 0) {
        printf("FAIL: max %d uncovered pixels (%.2f%% of screen)\n",
               worst, (float)worst * 100.0f / (TW * TH));
        tests_run--; return;
    }
    PASS();
}

static void test_slide_right_gap(void) {
    TEST("SLIDE_RIGHT no uncovered pixels across all progress");
    int worst = test_all_progress(EUI_ANIM_SLIDE_RIGHT, 300);
    if (worst < 0) FAIL("allocation failed");
    if (worst > 0) {
        printf("FAIL: max %d uncovered pixels (%.2f%% of screen)\n",
               worst, (float)worst * 100.0f / (TW * TH));
        tests_run--; return;
    }
    PASS();
}

static void test_slide_up_gap(void) {
    TEST("SLIDE_UP no uncovered pixels across all progress");
    int worst = test_all_progress(EUI_ANIM_SLIDE_UP, 300);
    if (worst < 0) FAIL("allocation failed");
    if (worst > 0) {
        printf("FAIL: max %d uncovered pixels (%.2f%% of screen)\n",
               worst, (float)worst * 100.0f / (TW * TH));
        tests_run--; return;
    }
    PASS();
}

static void test_fade_gap(void) {
    TEST("FADE no uncovered pixels across all progress");
    int worst = test_all_progress(EUI_ANIM_FADE, 400);
    if (worst < 0) FAIL("allocation failed");
    if (worst > 0) {
        printf("FAIL: max %d uncovered pixels (%.2f%% of screen)\n",
               worst, (float)worst * 100.0f / (TW * TH));
        tests_run--; return;
    }
    PASS();
}

static void test_critical_progress_points(void) {
    TEST("critical progress points (50%%, 51%%, 75%%, 99%%)");
    eui_anim_type_t types[] = { EUI_ANIM_SLIDE_LEFT, EUI_ANIM_SLIDE_RIGHT,
                                EUI_ANIM_SLIDE_UP, EUI_ANIM_FADE };
    int durations[] = { 300, 300, 300, 400 };
    int crit_progress[] = { 50, 51, 75, 99 };
    for (int ti = 0; ti < 4; ti++) {
        for (int pi = 0; pi < 4; pi++) {
            int elapsed = durations[ti] * crit_progress[pi] / 100;
            int c = check_transition(types[ti], (uint32_t)elapsed);
            if (c < 0) FAIL("allocation failed");
            if (c > 0) {
                printf("FAIL: %s at %d%% progress: %d uncovered\n",
                       anim_name(types[ti]), crit_progress[pi], c);
                tests_run--; return;
            }
        }
    }
    PASS();
}

/* ---- test: raylib-style 1bpp buffer conversion ---- */
static void test_raylib_1bpp_conversion(void) {
    TEST("raylib 1bpp buffer conversion correctness");
    /* Create a 128x64 1bpp canvas buffer manually */
    uint8_t buf[128 * 64 / 8];
    memset(buf, 0, sizeof(buf));

    /* Set all bits to 1 (white) like both views filling with COVER_COLOR */
    memset(buf, 0xFF, sizeof(buf));

    /* Simulate raylib disp_write_buffer 1bpp conversion */
    int W = 128, H = 64;
    uint8_t *rgba = (uint8_t*)malloc((size_t)W * H * 4);
    memset(rgba, 0xAA, (size_t)W * H * 4); /* poison */

    int bytes_per_row = W / 8;
    for (int y = 0; y < H; y++) {
        int flipped_y = H - 1 - y;
        for (int x = 0; x < W; x++) {
            int byte_idx = y * bytes_per_row + (x / 8);
            int bit_pos  = x % 8;
            uint8_t pixel = (buf[byte_idx] >> bit_pos) & 1;
            uint8_t *dst = rgba + (size_t)(flipped_y * W + x) * 4;
            if (pixel) {
                dst[0] = 255; dst[1] = 255; dst[2] = 255; dst[3] = 255;
            } else {
                dst[0] = 0;   dst[1] = 0;   dst[2] = 0;   dst[3] = 255;
            }
        }
    }

    /* Verify every pixel is white RGBA(255,255,255,255) */
    for (int i = 0; i < W * H; i++) {
        if (rgba[i*4+0] != 255 || rgba[i*4+1] != 255 ||
            rgba[i*4+2] != 255 || rgba[i*4+3] != 255) {
            printf("FAIL: pixel %d (x=%d,y=%d) = RGBA(%d,%d,%d,%d), expected (255,255,255,255)\n",
                   i, i % W, i / W,
                   rgba[i*4+0], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]);
            free(rgba);
            FAIL("conversion error");
        }
    }
    free(rgba);
    PASS();
}

/* ---- test: raylib-style 1bpp with checkerboard pattern ---- */
static void test_raylib_1bpp_checkerboard(void) {
    TEST("raylib 1bpp checkerboard conversion");
    uint8_t buf[128 * 64 / 8];
    int W = 128, H = 64;

    /* Set checkerboard: even (x+y) = white, odd = black */
    memset(buf, 0, sizeof(buf));
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (((x + y) & 1) == 0) {
                int byte_idx = y * (W / 8) + (x / 8);
                int bit_pos  = x % 8;
                buf[byte_idx] |= (1u << bit_pos);
            }
        }
    }

    uint8_t *rgba = (uint8_t*)malloc((size_t)W * H * 4);
    int bytes_per_row = W / 8;
    for (int y = 0; y < H; y++) {
        int flipped_y = H - 1 - y;
        for (int x = 0; x < W; x++) {
            int byte_idx = y * bytes_per_row + (x / 8);
            int bit_pos  = x % 8;
            uint8_t pixel = (buf[byte_idx] >> bit_pos) & 1;
            uint8_t *dst = rgba + (size_t)(flipped_y * W + x) * 4;
            if (pixel) {
                dst[0] = 255; dst[1] = 255; dst[2] = 255; dst[3] = 255;
            } else {
                dst[0] = 0;   dst[1] = 0;   dst[2] = 0;   dst[3] = 255;
            }
        }
    }

    /* Verify checkerboard in RGBA space (with Y flip) */
    int errors = 0;
    for (int y = 0; y < H; y++) {
        int flipped_y = H - 1 - y;
        for (int x = 0; x < W; x++) {
            int expected_white = ((x + y) & 1) == 0;
            uint8_t *dst = rgba + (size_t)(flipped_y * W + x) * 4;
            if (expected_white) {
                if (dst[0] != 255) errors++;
            } else {
                if (dst[0] != 0) errors++;
            }
        }
    }
    free(rgba);
    if (errors > 0) {
        printf("FAIL: %d conversion errors in checkerboard\n", errors);
        FAIL("checkerboard conversion");
    }
    PASS();
}

/* ---- test: raylib 1bpp conversion after transition rendering ---- */
static void test_raylib_1bpp_transition_conversion(void) {
    TEST("raylib 1bpp conversion after SLIDE_LEFT at 75%% progress");

    eui_view_dispatcher_t vd;
    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    if (!canvas) FAIL("canvas alloc");

    g_tick_ms = 1000;
    eui_view_dispatcher_init(&vd, canvas, test_get_tick);

    eui_view_t v1, v2;
    eui_view_init(&v1, cover_view_handler, &v1);
    v1.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_init(&v2, cover_view_handler, &v2);
    v2.area = (eui_rect_t){ 0, 0, TW, TH };

    eui_view_dispatcher_add(&vd, 1, &v1);
    eui_view_dispatcher_add(&vd, 2, &v2);

    /* 75% progress: off = 96, old=[0,32), new=[32,128) with my fix */
    vd.transitioning        = true;
    vd.transition_type      = EUI_ANIM_SLIDE_LEFT;
    vd.transition_prev_view = &v1;
    vd.current_view_idx     = 1;
    g_tick_ms               = 1000 + 225;  /* 225/300 = 0.75 */
    vd.transition_start_ms  = 1000;

    eui_view_dispatcher_tick(&vd);

    /* Now simulate raylib 1bpp conversion on mock_buf */
    int W = TW, H = TH;
    uint8_t *rgba = (uint8_t*)malloc((size_t)W * H * 4);
    int bytes_per_row = W / 8;
    for (int y = 0; y < H; y++) {
        int flipped_y = H - 1 - y;
        for (int x = 0; x < W; x++) {
#if EUI_COLOR_DEPTH == 1
            int byte_idx = y * bytes_per_row + (x / 8);
            int bit_pos  = x % 8;
            uint8_t pixel = (mock_buf[byte_idx] >> bit_pos) & 1;
#elif EUI_COLOR_DEPTH == 16
            uint16_t c = ((uint16_t*)mock_buf)[y * W + x];
            uint8_t pixel = (c != 0) ? 1 : 0;
#else
            uint8_t pixel = mock_buf[y * W + x] ? 1 : 0;
#endif
            uint8_t *dst = rgba + (size_t)(flipped_y * W + x) * 4;
            if (pixel) {
                dst[0] = 255; dst[1] = 255; dst[2] = 255; dst[3] = 255;
            } else {
                dst[0] = 0;   dst[1] = 0;   dst[2] = 0;   dst[3] = 255;
            }
        }
    }

    /* Verify: every pixel should be COVER_COLOR (white/1), no CLEAR_COLOR (black/0) */
    int errors = 0;
    for (int i = 0; i < W * H; i++) {
        if (rgba[i*4+0] != 255 || rgba[i*4+1] != 255 || rgba[i*4+2] != 255) {
            errors++;
            if (errors <= 5) {
                printf("  bad pixel %d (x=%d,y=%d): RGBA(%d,%d,%d,%d)\n",
                       i, i % W, i / W,
                       rgba[i*4+0], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]);
            }
        }
    }
    free(rgba);
    eui_canvas_destroy(canvas);

    if (errors > 0) {
        printf("FAIL: %d pixels not white after conversion\n", errors);
        FAIL("transition conversion");
    }
    PASS();
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== View Transition Uncovered-Pixel Tests ===\n");

    test_slide_left_gap();
    test_slide_right_gap();
    test_slide_up_gap();
    test_fade_gap();
    test_critical_progress_points();

    test_raylib_1bpp_conversion();
    test_raylib_1bpp_checkerboard();
    test_raylib_1bpp_transition_conversion();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
