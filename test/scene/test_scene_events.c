/* Scene event API unit tests: eui_scene_manager_send_event() dispatches
 * to the stack-top scene's on_event (false when unhandled or absent) and
 * eui_scene_manager_previous_id() reports the root scene before the last
 * switch() ((uint32_t)-1 with no history, unchanged by push()). */
#include "common/eui_test.h"
#include "eui/eui_scene.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_last_evt;
static int32_t s_last_arg;
static int s_hits;

static bool on_evt(void *ctx, uint32_t id, int32_t arg)
{
    (void)ctx;
    s_last_evt = id;
    s_last_arg = arg;
    s_hits++;
    return true;
}

static void on_enter(void *ctx) { (void)ctx; }

static void test_send_event_and_previous_id(void)
{
    TEST("send_event dispatches to stack top; previous_id tracks switch history");
    eui_view_t v;
    memset(&v, 0, sizeof(v)); /* handler-less view: ENTER/EXIT are no-ops */

    eui_scene_manager_t sm;
    memset(&sm, 0, sizeof(sm));
    eui_scene_t table[2] = {
        { 10, &v, on_enter, NULL, NULL,   NULL, NULL }, /* no on_event */
        { 11, &v, on_enter, NULL, on_evt, NULL, NULL },
    };
    assert(eui_scene_manager_register(&sm, table, 2) == 0);
    if (eui_scene_manager_previous_id(&sm) != (uint32_t)-1)
        FAIL("no history before the first switch");

    eui_scene_manager_switch(&sm, 10); /* from an empty stack: no previous */
    if (eui_scene_manager_previous_id(&sm) != (uint32_t)-1)
        FAIL("previous still unset after first switch");
    if (eui_scene_manager_send_event(&sm, 0x99, 7))
        FAIL("top without on_event must not handle");

    eui_scene_manager_switch(&sm, 11); /* previous becomes scene 10 */
    if (eui_scene_manager_previous_id(&sm) != 10)
        FAIL("previous id after switch");

    s_hits = 0;
    if (!eui_scene_manager_send_event(&sm, 0x42, -5))
        FAIL("top with on_event should handle");
    if (s_hits != 1 || s_last_evt != 0x42 || s_last_arg != -5)
        FAIL("event payload not forwarded to on_event");

    assert(eui_scene_manager_push(&sm, 10) == 0); /* top is now scene 10 */
    s_hits = 0;
    if (eui_scene_manager_send_event(&sm, 1, 0))
        FAIL("pushed top without on_event must not handle");
    if (eui_scene_manager_previous_id(&sm) != 10)
        FAIL("push must not change previous");
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Scene Event Tests ===\n");
    test_send_event_and_previous_id();
    return eui_test_summary();
}
