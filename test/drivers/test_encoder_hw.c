/* 绝对计数编码器驱动：增量运算、方向映射、夹取与余量携带。
 * 装配器侧的 delta 语义已由 test/input/test_input_edge.c 钉住
 * （CW 正 delta 保留、CCW 负 delta 保留、CCW 非负被丢弃），此处只管驱动产出。 */
#include "eui/driver/eui_drv_encoder_hw.h"
#include <stdio.h>
#include <string.h>
#include "common/eui_test.h"

static int32_t fake_count;
static int32_t fake_read_count(void *ud) { (void)ud; return fake_count; }

static eui_input_drv_t *make_drv(void) {
    eui_drv_encoder_hw_config_t cfg = {
        .hw = { .read_count = fake_read_count, .user_data = NULL },
    };
    return eui_drv_encoder_hw_create(&cfg);
}

static void test_hw_encoder_delta_and_direction(void) {
    TEST("hw encoder: 增量正负映射 CW/CCW，delta 保留数量");
    fake_count = 0;
    eui_input_drv_t *d = make_drv();
    if (!d) FAIL("create returned NULL");

    eui_event_t e;
    if (d->poll(&e, d->user_data) != 0) FAIL("no change should produce no event");

    fake_count = 3;                      /* 一次 polling 周期内转 3 格 */
    if (d->poll(&e, d->user_data) <= 0) FAIL("expected an event");
    if (e.type != EUI_EVT_ENCODER_CW) FAIL("positive delta must be CW");
    if (e.data.enc_delta != 3) FAIL("delta not preserved");
    if (d->poll(&e, d->user_data) != 0) FAIL("same count must not repeat");

    fake_count = 1;                      /* 反向 2 格（绝对计数 3 → 1 = 增量 -2） */
    if (d->poll(&e, d->user_data) <= 0) FAIL("expected an event");
    if (e.type != EUI_EVT_ENCODER_CCW) FAIL("negative delta must be CCW");
    if (e.data.enc_delta != -2) FAIL("delta not preserved");

    eui_drv_encoder_hw_destroy(d);
    eui_drv_encoder_hw_destroy(NULL);
    PASS();
}

static void test_hw_encoder_clamps_and_carries(void) {
    TEST("hw encoder: 超出 int16 范围时夹取并携带余量");
    fake_count = 0;
    eui_input_drv_t *d = make_drv();
    if (!d) FAIL("create returned NULL");

    eui_event_t e;
    fake_count = 40000;                  /* > INT16_MAX */
    if (d->poll(&e, d->user_data) <= 0) FAIL("expected an event");
    if (e.data.enc_delta != INT16_MAX) FAIL("delta not clamped to INT16_MAX");

    /* 余量 40000 - 32767 = 7233：下一次 poll 即使计数不变也要吐出余量 */
    if (d->poll(&e, d->user_data) <= 0) FAIL("carry not delivered");
    if (e.data.enc_delta != 7233) FAIL("carry value wrong");
    if (d->poll(&e, d->user_data) != 0) FAIL("carry must drain to zero");

    eui_drv_encoder_hw_destroy(d);
    PASS();
}

static void test_hw_encoder_init_rejects_bad_cfg(void) {
    TEST("hw encoder: create 拒绝 NULL/缺 read_count 的配置");
    if (eui_drv_encoder_hw_create(NULL) != NULL) FAIL("NULL cfg must be rejected");
    eui_drv_encoder_hw_config_t bad = { .hw = { .read_count = NULL, .user_data = NULL } };
    if (eui_drv_encoder_hw_create(&bad) != NULL) FAIL("missing read_count must be rejected");
    PASS();
}

int main(void) {
    eui_test_init();
    printf("=== HW Encoder Driver Tests ===\n");
    test_hw_encoder_delta_and_direction();
    test_hw_encoder_clamps_and_carries();
    test_hw_encoder_init_rejects_bad_cfg();
    return eui_test_summary();
}
