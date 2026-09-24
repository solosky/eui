/* eui_input_mux：把多个 eui_input_drv_t 合成一个。
 * 真实板子（编码器 + 两颗按键）只有一个 eui_init(input) 入口。 */
#include "eui/eui_input_mux.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <string.h>

/* --- 假子驱动：脚本化产出 --- */
typedef struct {
    eui_event_t queue[8];
    int count;
    int index;
    int init_calls;
    int deinit_calls;
} fake_t;

static fake_t a, b;

static int fake_init(void *ud)   { ((fake_t *)ud)->init_calls++; return 0; }
static int fake_deinit(void *ud) { ((fake_t *)ud)->deinit_calls++; return 0; }

static int fake_poll(eui_event_t *evt, void *ud) {
    fake_t *f = (fake_t *)ud;
    if (f->index >= f->count) return 0;
    *evt = f->queue[f->index++];
    return 1;
}
static void fake_set_cb(void (*cb)(const eui_event_t *), void *ud) { (void)cb; (void)ud; }

static eui_input_drv_t drv_a = { fake_init, fake_deinit, fake_poll, fake_set_cb, &a };
static eui_input_drv_t drv_b = { fake_init, fake_deinit, fake_poll, fake_set_cb, &b };

static void reset_fakes(void) {
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    drv_a.user_data = &a; drv_b.user_data = &b;
}

static void test_mux_polls_in_order(void) {
    TEST("mux: 按序轮询，两侧事件都能取出");
    reset_fakes();
    a.queue[0] = (eui_event_t){ .type = EUI_EVT_ENCODER_CW, .data.enc_delta = 2 };
    a.count = 1;
    b.queue[0] = (eui_event_t){ .type = EUI_EVT_KEY_PRESS, .data.key_id = 3 };
    b.count = 1;

    eui_input_drv_t *subs[2] = { &drv_a, &drv_b };
    eui_input_mux_config_t cfg = { .drivers = subs, .count = 2 };
    eui_input_drv_t *mux = eui_input_mux_create(&cfg);
    if (!mux) FAIL("create returned NULL");

    eui_event_t e;
    if (mux->poll(&e, mux->user_data) <= 0) FAIL("first poll produced nothing");
    if (e.type != EUI_EVT_ENCODER_CW || e.data.enc_delta != 2) FAIL("first event wrong");
    if (mux->poll(&e, mux->user_data) <= 0) FAIL("second poll produced nothing");
    if (e.type != EUI_EVT_KEY_PRESS || e.data.key_id != 3) FAIL("second event wrong");
    if (mux->poll(&e, mux->user_data) != 0) FAIL("third poll should be empty");

    eui_input_mux_destroy(mux);
    PASS();
}

static void test_mux_skips_null_and_dead_subdrives(void) {
    TEST("mux: 跳过 NULL/无 poll 的子驱动，直到产出为止");
    reset_fakes();
    b.queue[0] = (eui_event_t){ .type = EUI_EVT_KEY_RELEASE, .data.key_id = 1 };
    b.count = 1;

    eui_input_drv_t *subs[3] = { NULL, &drv_a, &drv_b };   /* a 为空、b 有事件 */
    eui_input_mux_config_t cfg = { .drivers = subs, .count = 3 };
    eui_input_drv_t *mux = eui_input_mux_create(&cfg);
    if (!mux) FAIL("create returned NULL");

    eui_event_t e;
    if (mux->poll(&e, mux->user_data) <= 0) FAIL("expected event from b");
    if (e.type != EUI_EVT_KEY_RELEASE || e.data.key_id != 1) FAIL("event wrong");

    eui_input_mux_destroy(mux);
    PASS();
}

static void test_mux_init_deinit_calls_through(void) {
    TEST("mux: init/deinit 逐个下发，create 拒绝非法配置");
    reset_fakes();
    eui_input_drv_t *subs[2] = { &drv_a, &drv_b };
    eui_input_mux_config_t cfg = { .drivers = subs, .count = 2 };
    eui_input_drv_t *mux = eui_input_mux_create(&cfg);
    if (!mux) FAIL("create returned NULL");

    if (mux->init(mux->user_data) != 0) FAIL("init failed");
    if (a.init_calls != 1 || b.init_calls != 1) FAIL("init not dispatched to both");
    mux->deinit(mux->user_data);
    if (a.deinit_calls != 1 || b.deinit_calls != 1) FAIL("deinit not dispatched to both");

    eui_input_mux_destroy(mux);
    eui_input_mux_destroy(NULL);   /* NULL 安全 */

    if (eui_input_mux_create(NULL) != NULL) FAIL("NULL cfg must be rejected");
    eui_input_drv_t *none[1] = { NULL };
    eui_input_mux_config_t bad = { .drivers = none, .count = 0 };
    if (eui_input_mux_create(&bad) != NULL) FAIL("count=0 must be rejected");
    PASS();
}

int main(void) {
    eui_test_init();
    printf("=== Input Mux Tests ===\n");
    test_mux_polls_in_order();
    test_mux_skips_null_and_dead_subdrives();
    test_mux_init_deinit_calls_through();
    return eui_test_summary();
}
