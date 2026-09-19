/* Scene manager unit tests: standalone mode (direct view events) and
 * attached mode (view switches routed through a view dispatcher). */
#include "eui/eui_scene.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_canvas.h"
#include "eui/eui_allocator.h"
#include "common/eui_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define W 32
#define H 32

static uint8_t mock_buf[W * H * 2];

static void mock_write_buffer(const uint8_t *b, const eui_rect_t *r, void *ud)
{
    (void)ud; (void)b;
    (void)r;
}

static eui_display_drv_t mock_display = {
    .caps = { .width = W, .height = H, .color_depth = 16, .buffer_mode = EUI_BUFFER_FULL, .has_gram = false },
    .init = NULL,
    .write_buffer = mock_write_buffer,
};

/* event log: one char per observed callback */
static char log_buf[64];
static size_t log_len;

static void log_reset(void) { log_len = 0; memset(log_buf, 0, sizeof(log_buf)); }
static void log_push(char c)
{
    if (log_len + 1 < sizeof(log_buf)) log_buf[log_len++] = c;
}

typedef struct {
    char id;        /* 'A' / 'B' */
    int enters;     /* view ENTER events */
    int exits;      /* view EXIT events */
    int draws;      /* view DRAW events */
} view_counter_t;

static bool counting_handler(eui_view_event_t *e, void *context)
{
    view_counter_t *cnt = context;
    switch (e->type) {
    case EUI_VIEW_EVT_ENTER: cnt->enters++; log_push(cnt->id); log_push('e'); return true;
    case EUI_VIEW_EVT_EXIT:  cnt->exits++;  log_push(cnt->id); log_push('x'); return true;
    case EUI_VIEW_EVT_DRAW:  cnt->draws++;  log_push(cnt->id); log_push('d'); return true;
    default: return false;
    }
}

/* scene-level callbacks */
static void sc_enter_a(void *ctx) { (void)ctx; log_push('A'); log_push('E'); }
static void sc_exit_a(void *ctx)  { (void)ctx; log_push('A'); log_push('X'); }
static void sc_enter_b(void *ctx) { (void)ctx; log_push('B'); log_push('E'); }

enum { SID_A = 1, SID_B };

static eui_view_t view_a, view_b;
static view_counter_t cnt_a = { 'A', 0, 0, 0 }, cnt_b = { 'B', 0, 0, 0 };

static void setup_views(void)
{
    eui_view_init(&view_a, counting_handler, &cnt_a);
    eui_view_init(&view_b, counting_handler, &cnt_b);
    cnt_a.enters = cnt_a.exits = cnt_a.draws = 0;
    cnt_b.enters = cnt_b.exits = cnt_b.draws = 0;
}

static void test_standalone_switch_and_back(void)
{
    TEST("standalone: switch fires scene callbacks and view events");
    setup_views();
    log_reset();

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t scenes[2] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a },
        { SID_B, &view_b, sc_enter_b, NULL,      NULL, &cnt_b },
    };
    assert(eui_scene_manager_register(&sm, scenes, 2) == 0);

    eui_scene_manager_switch(&sm, SID_A);
    /* first switch: no previous scene to exit; view A ENTER then scene on_enter */
    if (strcmp(log_buf, "AeAE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (sm.current != 0 || sm.previous != -1) { printf("cur=%d prev=%d\n", sm.current, sm.previous); FAIL("bookkeeping"); }

    log_reset();
    eui_scene_manager_switch(&sm, SID_B);
    /* exit order: scene A on_exit, view A EXIT, view B ENTER, scene B on_enter */
    if (strcmp(log_buf, "AXAxBeBE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (sm.previous != 0 || sm.current != 1) { printf("cur=%d prev=%d\n", sm.current, sm.previous); FAIL("bookkeeping"); }

    log_reset();
    eui_scene_manager_back(&sm);
    if (strcmp(log_buf, "BxAeAE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (sm.current != 0) FAIL("back should land on A");

    log_reset();
    eui_scene_manager_switch(&sm, SID_A); /* no-op: already current */
    if (log_len != 0) FAIL("same-scene switch should be a no-op");
    PASS();
}

static void test_attached_routes_via_dispatcher(void)
{
    TEST("attached: switch routes through the view dispatcher");
    setup_views();
    log_reset();

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    assert(canvas);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas, NULL);

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t scenes[2] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a },
        { SID_B, &view_b, sc_enter_b, NULL,      NULL, &cnt_b },
    };
    assert(eui_scene_manager_register(&sm, scenes, 2) == 0);
    eui_scene_manager_attach(&sm, &vd);
    assert(eui_view_dispatcher_add(&vd, SID_A, &view_a) == 0);
    assert(eui_view_dispatcher_add(&vd, SID_B, &view_b) == 0);

    eui_scene_manager_switch(&sm, SID_A);
    /* dispatcher made view A active (immediate draw) and fired its ENTER;
     * scene callbacks ran around the dispatcher call */
    if (strcmp(log_buf, "AEAeAd") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (eui_view_dispatcher_get_active(&vd) != &view_a) FAIL("active view");

    log_reset();
    eui_scene_manager_switch(&sm, SID_B);
    if (strcmp(log_buf, "AXBEAxBeBd") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (eui_view_dispatcher_get_active(&vd) != &view_b) FAIL("active view");
    if (cnt_a.exits != 1 || cnt_b.enters != 1) FAIL("exactly one EXIT/ENTER per view");

    /* dispatcher keeps routing draws to the scene manager's current view */
    eui_view_dispatcher_tick(&vd);
    assert(cnt_b.draws == 2);

    log_reset();
    eui_scene_manager_back(&sm);
    if (eui_view_dispatcher_get_active(&vd) != &view_a) FAIL("back via dispatcher");

    eui_canvas_destroy(canvas);
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Scene Manager Tests ===\n");
    test_standalone_switch_and_back();
    test_attached_routes_via_dispatcher();
    return eui_test_summary();
}
