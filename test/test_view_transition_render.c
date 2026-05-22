#include "eui/eui.h"
#include "eui/eui_view.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include "eui/driver/eui_drv_raylib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 65536
static uint8_t mem_pool[POOL_SIZE];

#define TW 128
#define TH 64

/* ── tick mock: return controlled value ── */
static uint32_t g_mock_tick = 1000;
static uint32_t mock_get_tick(void) { return g_mock_tick; }

static int tests_run = 0, tests_passed = 0;
#define TEST(n)  do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS()   do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m)  do { printf("FAIL: %s\n", m); return; } while(0)

/* ── view handler: fill entire area with white ── */
static bool cover_view_handler(eui_view_event_t *evt, void *context) {
    eui_view_t *view = (eui_view_t*)context;
    if (evt->type == EUI_VIEW_EVT_DRAW) {
        eui_canvas_t *c = evt->event.draw.canvas;
#if EUI_COLOR_DEPTH == 1
        eui_canvas_set_color(c, 1);
#else
        eui_canvas_set_color(c, 0xFFFF);
#endif
        eui_canvas_fill_rect(c, view->area.x, view->area.y,
                                view->area.w, view->area.h);
        return true;
    }
    return false;
}

/* ── pixel counting ── */
static int count_black_rgba(const uint8_t *rgba, int w, int h) {
    int n = 0;
    for (int i = 0; i < w * h; i++)
        if (rgba[i*4] == 0 && rgba[i*4+1] == 0 && rgba[i*4+2] == 0) n++;
    return n;
}

static int count_white_rgba(const uint8_t *rgba, int w, int h) {
    int n = 0;
    for (int i = 0; i < w * h; i++)
        if (rgba[i*4] == 255 && rgba[i*4+1] == 255 && rgba[i*4+2] == 255) n++;
    return n;
}

/* ── edge-wrap check ──
 * If GL_REPEAT causes texcoord 1.0→0.0 wrapping, right-edge pixels will
 * match LEFT-edge pixels instead of their interior neighbors.
 * With TEXTURE_WRAP_CLAMP this should never happen. */
static int check_edge_wrap(const uint8_t *rgba, int w, int h) {
    int err = 0;
    for (int y = 0; y < h; y++) {
        uint8_t *r = (uint8_t*)rgba + (size_t)(y * w + w - 1) * 4;
        uint8_t *p = (uint8_t*)rgba + (size_t)(y * w + w - 2) * 4;
        uint8_t *l = (uint8_t*)rgba + (size_t)(y * w) * 4;
        if (memcmp(r, l, 3) == 0 && memcmp(r, p, 3) != 0) err++;
    }
    return err;
}

/* ── run one transition, return # black pixels ── */
static int run_transition_frame(eui_view_dispatcher_t *vd,
                                 eui_anim_type_t anim, float progress) {
    uint32_t duration = (anim == EUI_ANIM_FADE) ? 400 : 300;
    g_mock_tick = 1000 + (uint32_t)(progress * (float)duration);
    vd->transitioning        = true;
    vd->transition_type      = anim;
    vd->transition_start_ms  = 1000;
    vd->current_view_idx     = 1;

    /* register two views (idempotent, but safe) */
    eui_view_t *v1 = vd->views[0].view;
    eui_view_t *v2 = vd->views[1].view;
    vd->transition_prev_view = v1;

    eui_view_dispatcher_tick(vd);
    eui_drv_raylib_refresh();

    uint16_t rw, rh;
    const uint8_t *rgba = eui_drv_raylib_get_rgba_buffer(&rw, &rh);
    if (!rgba) return -1;
    return count_black_rgba(rgba, rw, rh);
}

/* ══════════════════════════════════════════════════════════════════ */

static void test_all_transitions_zero_black(void) {
    TEST("e2e: all transitions 0 black pixels (full coverage)");

    eui_display_hal_t *d = eui_drv_raylib_create_display(TW, TH, EUI_COLOR_DEPTH);
    eui_input_hal_t *inp = eui_drv_raylib_create_input();
    eui_config_t cfg = { .display = d, .input = inp };
    eui_init(&cfg);
    eui_set_tick_callback(mock_get_tick);
    d->init(d->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_view_t v1, v2;
    eui_view_init(&v1, cover_view_handler, &v1);
    v1.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_init(&v2, cover_view_handler, &v2);
    v2.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_dispatcher_add(vd, 1, &v1);
    eui_view_dispatcher_add(vd, 2, &v2);

    struct { eui_anim_type_t a; const char *n; int dur; } tt[] = {
        { EUI_ANIM_SLIDE_LEFT,  "SLIDE_LEFT",  300 },
        { EUI_ANIM_SLIDE_RIGHT, "SLIDE_RIGHT", 300 },
        { EUI_ANIM_SLIDE_UP,    "SLIDE_UP",    300 },
        { EUI_ANIM_FADE,        "FADE",        400 },
    };
    int worst = 0;
    for (int ti = 0; ti < 4; ti++) {
        for (int pct = 5; pct <= 100; pct += 5) {
            int blk = run_transition_frame(vd, tt[ti].a, pct / 100.0f);
            if (blk < 0) goto cleanup;
            if (blk > worst) worst = blk;
        }
    }

    if (worst > 0) {
        printf("FAIL: max %d uncovered pixels\n", worst);
        tests_run--;
    } else {
        PASS();
    }

cleanup:
    d->deinit(d->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(inp);
    eui_drv_raylib_destroy_display(d);
}

static void test_no_edge_wrap_artifacts(void) {
    TEST("e2e: no GL_REPEAT edge-wrap artifacts");

    eui_display_hal_t *d = eui_drv_raylib_create_display(TW, TH, EUI_COLOR_DEPTH);
    eui_input_hal_t *inp = eui_drv_raylib_create_input();
    eui_config_t cfg = { .display = d, .input = inp };
    eui_init(&cfg);
    eui_set_tick_callback(mock_get_tick);
    d->init(d->user_data);

    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_view_t v1, v2;
    eui_view_init(&v1, cover_view_handler, &v1);
    v1.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_init(&v2, cover_view_handler, &v2);
    v2.area = (eui_rect_t){ 0, 0, TW, TH };
    eui_view_dispatcher_add(vd, 1, &v1);
    eui_view_dispatcher_add(vd, 2, &v2);

    eui_anim_type_t types[] = { EUI_ANIM_SLIDE_LEFT, EUI_ANIM_SLIDE_RIGHT,
                                EUI_ANIM_SLIDE_UP, EUI_ANIM_FADE };
    int total_err = 0;
    for (int ti = 0; ti < 4; ti++) {
        uint32_t dur = (types[ti] == EUI_ANIM_FADE) ? 400 : 300;
        g_mock_tick = 1000 + dur / 2; /* 50% progress */
        vd->transitioning       = true;
        vd->transition_type     = types[ti];
        vd->transition_start_ms = 1000;
        vd->current_view_idx    = 1;
        vd->transition_prev_view = vd->views[0].view;

        eui_view_dispatcher_tick(vd);
        eui_drv_raylib_refresh();

        uint16_t rw, rh;
        const uint8_t *rgba = eui_drv_raylib_get_rgba_buffer(&rw, &rh);
        if (rgba) {
            int e = check_edge_wrap(rgba, rw, rh);
            if (e > 0) { total_err += e; }
        }
    }

    if (total_err > 0) {
        printf("FAIL: %d edge-wrap rows\n", total_err);
        tests_run--;
    } else {
        PASS();
    }

    d->deinit(d->user_data);
    eui_deinit();
    eui_drv_raylib_destroy_input(inp);
    eui_drv_raylib_destroy_display(d);
}

int main(void) {
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== View Transition Render Integration Tests ===\n");
    test_all_transitions_zero_black();
    test_no_edge_wrap_artifacts();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
