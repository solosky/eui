#include "eui/eui.h"
#include "eui/eui_view.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include <string.h>
#include <stdio.h>

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

static eui_display_hal_t mock_display = {
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

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== View Transition Uncovered-Pixel Tests ===\n");

    test_slide_left_gap();
    test_slide_right_gap();
    test_slide_up_gap();
    test_fade_gap();
    test_critical_progress_points();

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
