/* Scene manager unit tests: navigation-stack semantics in standalone
 * mode (direct view events) and attached mode (switch routes through
 * the dispatcher; push/pop route through its overlay stack). */
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

/* event log: one char pair per observed callback */
static char log_buf[64];
static size_t log_len;

static void log_reset(void) { log_len = 0; memset(log_buf, 0, sizeof(log_buf)); }
static void log_push(char c)
{
    if (log_len + 1 < sizeof(log_buf)) log_buf[log_len++] = c;
}

typedef struct {
    char id;        /* 'A' / 'B' / 'C' */
    int enters;     /* view ENTER events */
    int exits;      /* view EXIT events */
    int draws;      /* view DRAW events */
    int resumes;    /* scene on_resume callbacks */
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
static void sc_exit_b(void *ctx)  { (void)ctx; log_push('B'); log_push('X'); }
static void sc_resume(void *ctx) { view_counter_t *cnt = ctx; cnt->resumes++; log_push(cnt->id); log_push('R'); }


enum { SID_A = 1, SID_B, SID_C };

static eui_view_t view_a, view_b, view_c;
static view_counter_t cnt_a = { 'A', 0, 0, 0, 0 }, cnt_b = { 'B', 0, 0, 0, 0 },
                      cnt_c = { 'C', 0, 0, 0, 0 };

static void setup_views(void)
{
    eui_view_init(&view_a, counting_handler, &cnt_a);
    eui_view_init(&view_b, counting_handler, &cnt_b);
    eui_view_init(&view_c, counting_handler, &cnt_c);
    memset(&cnt_a, 0, sizeof(cnt_a)); cnt_a.id = 'A';
    memset(&cnt_b, 0, sizeof(cnt_b)); cnt_b.id = 'B';
    memset(&cnt_c, 0, sizeof(cnt_c)); cnt_c.id = 'C';
}

static void test_standalone_switch(void)
{
    TEST("standalone: switch fires scene callbacks and view events");
    setup_views();
    log_reset();

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t scenes[3] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a, sc_resume },
        { SID_B, &view_b, sc_enter_b, sc_exit_b, NULL, &cnt_b, sc_resume },
        { SID_C, &view_c, NULL,       NULL,      NULL, &cnt_c, sc_resume },
    };
    assert(eui_scene_manager_register(&sm, scenes, 3) == 0);

    eui_scene_manager_switch(&sm, SID_A);
    /* first switch: no previous scene to exit; view A ENTER then scene on_enter */
    if (strcmp(log_buf, "AeAE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (sm.current != 0 || sm.depth != 1) { printf("cur=%d depth=%u\n", sm.current, sm.depth); FAIL("bookkeeping"); }
    if (eui_scene_manager_depth(&sm) != 1) FAIL("depth accessor");
    if (eui_scene_manager_current_id(&sm) != SID_A) FAIL("current id accessor");
    if (eui_scene_manager_scene_at(&sm, 0)->scene_id != SID_A) FAIL("scene_at root");

    log_reset();
    eui_scene_manager_switch(&sm, SID_B);
    /* exit order: scene A on_exit, view A EXIT, view B ENTER, scene B on_enter */
    if (strcmp(log_buf, "AXAxBeBE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected order"); }
    if (sm.depth != 1 || eui_scene_manager_current_id(&sm) != SID_B) FAIL("bookkeeping");

    /* switch() resets the stack, so back() at the root is a no-op */
    log_reset();
    eui_scene_manager_back(&sm);
    if (log_len != 0 || eui_scene_manager_current_id(&sm) != SID_B) FAIL("back at root should be a no-op");

    log_reset();
    eui_scene_manager_switch(&sm, SID_A);
    if (strcmp(log_buf, "BXBxAeAE") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected swap-back order"); }
    if (sm.depth != 1 || eui_scene_manager_current_id(&sm) != SID_A) FAIL("bookkeeping");

    log_reset();
    eui_scene_manager_switch(&sm, SID_A); /* no-op: already current */
    if (log_len != 0) FAIL("same-scene switch should be a no-op");
    PASS();
}

static void test_standalone_push_pop(void)
{
    TEST("standalone: push/pop drive the stack and fire on_resume");
    setup_views();
    log_reset();

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t scenes[3] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a, sc_resume },
        { SID_B, &view_b, sc_enter_b, sc_exit_b, NULL, &cnt_b, sc_resume },
        { SID_C, &view_c, NULL,       NULL,      NULL, &cnt_c, sc_resume },
    };
    assert(eui_scene_manager_register(&sm, scenes, 3) == 0);

    eui_scene_manager_switch(&sm, SID_A);
    log_reset();

    /* push B on top of A: A exit (scene then view), B scene enter, then
     * view ENTER (scene state is prepared before activation, matching
     * the attached mode's ordering) */
    assert(eui_scene_manager_push(&sm, SID_B) == 0);
    if (strcmp(log_buf, "AXBEAxBe") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected push order"); }
    if (sm.depth != 2 || eui_scene_manager_current_id(&sm) != SID_B) FAIL("push bookkeeping");
    if (eui_scene_manager_scene_at(&sm, 0)->scene_id != SID_A) FAIL("root stays at level 0");
    if (eui_scene_manager_scene_at(&sm, 1)->scene_id != SID_B) FAIL("pushed scene at level 1");
    if (eui_scene_manager_scene_at(&sm, 2) != NULL) FAIL("out-of-range level should be NULL");

    /* pop B: B exit (scene then view), A re-enter, then A on_resume */
    log_reset();
    assert(eui_scene_manager_pop(&sm) == 0);
    if (strcmp(log_buf, "BXBxAeAR") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected pop order"); }
    if (sm.depth != 1 || eui_scene_manager_current_id(&sm) != SID_A) FAIL("pop bookkeeping");
    if (cnt_a.resumes != 1) FAIL("on_resume not fired on exposed scene");

    /* pop at the root is a no-op */
    log_reset();
    assert(eui_scene_manager_pop(&sm) == -1);
    if (log_len != 0 || sm.depth != 1) FAIL("pop at root should be a no-op");

    /* push() before the first switch() is rejected */
    eui_scene_manager_t sm2;
    memset(&sm2, 0, sizeof(sm2));
    assert(eui_scene_manager_register(&sm2, scenes, 3) == 0);
    assert(eui_scene_manager_push(&sm2, SID_B) == -1);
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
    eui_scene_t scenes[3] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a, sc_resume },
        { SID_B, &view_b, sc_enter_b, sc_exit_b, NULL, &cnt_b, sc_resume },
        { SID_C, &view_c, NULL,       NULL,      NULL, &cnt_c, sc_resume },
    };
    assert(eui_scene_manager_register(&sm, scenes, 3) == 0);
    eui_scene_manager_attach(&sm, &vd);
    assert(eui_view_dispatcher_add(&vd, SID_A, &view_a) == 0);
    assert(eui_view_dispatcher_add(&vd, SID_B, &view_b) == 0);
    assert(eui_view_dispatcher_add(&vd, SID_C, &view_c) == 0);

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
    eui_scene_manager_switch(&sm, SID_A);
    if (eui_view_dispatcher_get_active(&vd) != &view_a) FAIL("root switch via dispatcher");

    eui_canvas_destroy(canvas);
    PASS();
}

static void test_attached_push_pop(void)
{
    TEST("attached: push/pop route through the dispatcher overlay stack");
    setup_views();
    log_reset();

    eui_canvas_t *canvas = eui_canvas_create(&mock_display);
    assert(canvas);
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_init(&vd, canvas, NULL);

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t scenes[3] = {
        { SID_A, &view_a, sc_enter_a, sc_exit_a, NULL, &cnt_a, sc_resume },
        { SID_B, &view_b, sc_enter_b, sc_exit_b, NULL, &cnt_b, sc_resume },
        { SID_C, &view_c, NULL,       NULL,      NULL, &cnt_c, sc_resume },
    };
    assert(eui_scene_manager_register(&sm, scenes, 3) == 0);
    eui_scene_manager_attach(&sm, &vd);
    assert(eui_view_dispatcher_add(&vd, SID_A, &view_a) == 0);
    assert(eui_view_dispatcher_add(&vd, SID_B, &view_b) == 0);
    assert(eui_view_dispatcher_add(&vd, SID_C, &view_c) == 0);

    eui_scene_manager_switch(&sm, SID_A);
    log_reset();

    /* push B: A scene exit, B scene enter, then the dispatcher fires
     * view A EXIT and view B ENTER */
    assert(eui_scene_manager_push(&sm, SID_B) == 0);
    if (strcmp(log_buf, "AXBEAxBe") != 0) { printf("log=%s\n", log_buf); FAIL("unexpected push order"); }
    if (eui_view_dispatcher_get_active(&vd) != &view_b) FAIL("overlay becomes active");
    if (vd.overlay_count != 1) FAIL("dispatcher overlay stack out of sync");

    /* fill the dispatcher's overlay stack to capacity; the push past
     * the cap is rejected and neither stack grows */
    int pushed = 1;
    while (eui_scene_manager_push(&sm, SID_C) == 0)
        pushed++;
    if (pushed != EUI_MAX_OVERLAYS) { printf("pushed=%d cap=%d\n", pushed, EUI_MAX_OVERLAYS); FAIL("overlay capacity"); }
    if (sm.depth != (uint8_t)(1 + EUI_MAX_OVERLAYS)) FAIL("manager stack depth after cap");

    /* pop everything: each exposed level gets its view ENTER + on_resume
     * (stack was A,B,C,C,C — pops expose C, C, B, A) */
    while (sm.depth > 1)
        assert(eui_scene_manager_pop(&sm) == 0);
    if (eui_view_dispatcher_get_active(&vd) != &view_a) FAIL("back at root view");
    if (vd.overlay_count != 0) FAIL("overlay stack should be empty");
    { if (cnt_b.resumes != 1 || cnt_c.resumes != (EUI_MAX_OVERLAYS - 2)) FAIL("on_resume per exposure"); }
    if (cnt_a.resumes != 1) FAIL("final pop re-exposes the root");
    assert(eui_scene_manager_pop(&sm) == -1);

    /* switch() with overlays up tears them down top-down; when the
     * target IS the current root the swap itself is a no-op (the final
     * teardown pop already re-entered the root view) */
    assert(eui_scene_manager_push(&sm, SID_B) == 0);
    assert(eui_scene_manager_push(&sm, SID_C) == 0);
    int a_enters_before = cnt_a.enters, b_enters_before = cnt_b.enters;
    int a_resumes_before = cnt_a.resumes;
    eui_scene_manager_switch(&sm, SID_A);
    if (sm.depth != 1 || eui_scene_manager_current_id(&sm) != SID_A) FAIL("switch resets the stack");
    if (eui_view_dispatcher_get_active(&vd) != &view_a) FAIL("root active after teardown");
    if (vd.overlay_count != 0) FAIL("overlays torn down");
    if (cnt_a.resumes != a_resumes_before) FAIL("teardown must not fire on_resume");
    if (cnt_b.enters != b_enters_before + 1) FAIL("exposed B re-entered during teardown");
    if (cnt_a.enters != a_enters_before + 1) FAIL("root re-entered once by teardown");

    /* full root swap with overlays up: teardown, then root on_exit /
     * on_enter around the dispatcher switch */
    assert(eui_scene_manager_push(&sm, SID_B) == 0);
    assert(eui_scene_manager_push(&sm, SID_C) == 0);
    eui_scene_manager_switch(&sm, SID_B);
    if (sm.depth != 1 || eui_scene_manager_current_id(&sm) != SID_B) FAIL("target promoted to root");
    if (eui_view_dispatcher_get_active(&vd) != &view_b) FAIL("new root active");
    if (vd.overlay_count != 0) FAIL("overlays torn down on root swap");

    eui_canvas_destroy(canvas);
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Scene Manager Tests ===\n");
    test_standalone_switch();
    test_standalone_push_pop();
    test_attached_routes_via_dispatcher();
    test_attached_push_pop();
    return eui_test_summary();
}
