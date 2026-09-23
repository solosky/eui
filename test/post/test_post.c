/* eui_post：帧尾投递（FIFO / 排空到空 / 执行点晚于渲染 / 零开销）。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <eui/eui.h>
#include <eui/eui_post.h>
#include <eui/eui_config.h>

static uint8_t pool[128 * 1024];
static uint32_t fake_ms;
static uint32_t get_tick(void) { return fake_ms; }

static int stub_init(void *ud) { (void)ud; return 0; }
static void stub_flush(const uint8_t *b, const eui_rect_t *r, void *ud) { (void)b; (void)r; (void)ud; }
static int in_init(void *ud) { (void)ud; return 0; }
static int in_poll(eui_event_t *e, void *ud) { (void)e; (void)ud; return 0; }

/* 顺序记录器：回调把 ud 转成的编号写进 order[] */
static int order[128];
static int order_n;
static void rec_cb(void *ud) { order[order_n++] = (int)(intptr_t)ud; }

/* 链式回调：每次执行后按 budget_left 继续投递自己 */
static int chain_ran;
static int chain_left;
static void chain_cb(void *ud);
static void chain_cb(void *ud)
{
    (void)ud;
    chain_ran++;
    if (--chain_left > 0)
        assert(eui_post(chain_cb, NULL, NULL));
}

/* 渲染计数视图：记录 DRAW 次数；post_on_draw 指定在第 N 次 DRAW 中投递一次 */
typedef struct {
    int draws;
    int post_on_draw;
    eui_view_t view;
} draw_view_t;

static draw_view_t *g_dv;
static int sametick_ran;      /* 渲染中投递 + 同帧执行的标记 */
static int late_ran;          /* 晚于渲染执行的回调 */
static int late_seen_draws;   /* 该回调观察到的 DRAW 次数 */

static void same_tick_cb(void *ud) { (void)ud; sametick_ran++; }
static void late_cb(void *ud) { (void)ud; late_ran++; late_seen_draws = g_dv->draws; }

/* X：执行时取消挂在 &dv 上的其余条目，然后自己记账（值 200）。
 * dv 在 main 内为局部量，此处经 g_dv 取同一地址。 */
static void cancel_others_cb(void *ud)
{
    (void)ud;
    eui_post_cancel((const void *)g_dv);
    order[order_n++] = 200;
}

static bool dv_handler(eui_view_event_t *event, void *context)
{
    draw_view_t *dv = (draw_view_t *)context;
    if (event->type == EUI_VIEW_EVT_DRAW) {
        dv->draws++;
        if (dv->post_on_draw != 0 && dv->draws == dv->post_on_draw) {
            dv->post_on_draw = 0;
            assert(eui_post(same_tick_cb, NULL, NULL));
        }
    }
    return false;
}

int main(void)
{
    /* eui_init 之前投递：拒绝且不计数 */
    assert(!eui_post(rec_cb, NULL, NULL));
    assert(eui_post_count() == 0 && eui_post_dropped() == 0);

    eui_allocator_init_tlsf(pool, sizeof(pool));
    static eui_display_drv_t disp = {
        .caps = { .width = 64, .height = 64, .color_depth = 16, .buffer_mode = EUI_BUFFER_FULL },
        .init = stub_init, .write_buffer = stub_flush,
    };
    static eui_input_drv_t input = { .init = in_init, .poll = in_poll };
    eui_config_t cfg = { .display = &disp, .input = &input, .fps_target = 60,
                         .max_views = 8, .max_animations = 4, .max_widgets = 2 };
    assert(eui_init(&cfg) == 0);
    eui_set_tick_callback(get_tick);

    draw_view_t dv;
    memset(&dv, 0, sizeof(dv));
    eui_view_init(&dv.view, dv_handler, &dv);
    g_dv = &dv;
    assert(eui_view_dispatcher_add(eui_get_view_dispatcher(), 1, &dv.view) == 0);
    eui_view_dispatcher_switch_to(eui_get_view_dispatcher(), 1, EUI_ANIM_NONE);
    assert(dv.draws == 1);              /* switch_to 无动画时立即 DRAW 一次 */

    /* 1) FIFO：投递序即执行序 */
    assert(eui_post(rec_cb, (void *)(intptr_t)1, NULL));
    assert(eui_post(rec_cb, (void *)(intptr_t)2, NULL));
    assert(eui_post(rec_cb, (void *)(intptr_t)3, NULL));
    assert(eui_post_count() == 3);
    fake_ms += 16;
    eui_tick();
    assert(order_n == 3 && order[0] == 1 && order[1] == 2 && order[2] == 3);
    assert(eui_post_count() == 0);

    /* 2) 排空到空：回调内投递的条目同帧执行 */
    order_n = 0;
    chain_ran = 0;
    chain_left = 4;
    assert(eui_post(chain_cb, NULL, NULL));
    fake_ms += 16;
    eui_tick();
    assert(chain_ran == 4);             /* 4 条链式投递全部在本次 tick 内执行 */
    assert(eui_post_count() == 0);

    /* 3) 执行点晚于渲染：回调观察到的 DRAW 计数含本帧那一次 */
    int draws_before = dv.draws;
    assert(eui_post(late_cb, NULL, NULL));
    fake_ms += 16;
    eui_tick();
    assert(late_ran == 1);
    assert(dv.draws == draws_before + 1);
    assert(late_seen_draws == dv.draws);   /* 回调执行时本帧渲染已完成 */

    /* 4) 渲染中投递 → 同帧 Step 6 执行（app 时序等价性的前提） */
    sametick_ran = 0;
    dv.post_on_draw = dv.draws + 1;
    fake_ms += 16;
    eui_tick();
    assert(sametick_ran == 1);
    assert(eui_post_count() == 0);

    /* 5) 零开销：无投递时 tick 行为不变 */
    draws_before = dv.draws;
    fake_ms += 16;
    eui_tick();
    assert(dv.draws == draws_before + 1);
    assert(eui_post_count() == 0);
    assert(eui_post_dropped() == 0);

    /* 6) owner 取消：仅清匹配条目，其余保持相对顺序 */
    order_n = 0;
    assert(eui_post(rec_cb, (void *)(intptr_t)1, (const void *)&dv));
    assert(eui_post(rec_cb, (void *)(intptr_t)2, NULL));
    assert(eui_post(rec_cb, (void *)(intptr_t)3, (const void *)&dv));
    assert(eui_post_count() == 3);
    eui_post_cancel((const void *)&dv);
    assert(eui_post_count() == 1);
    fake_ms += 16;
    eui_tick();
    assert(order_n == 1 && order[0] == 2);

    /* 7) cancel(NULL) 是 no-op：匿名条目全部保留 */
    order_n = 0;
    assert(eui_post(rec_cb, (void *)(intptr_t)7, NULL));
    assert(eui_post(rec_cb, (void *)(intptr_t)8, NULL));
    eui_post_cancel(NULL);
    assert(eui_post_count() == 2);
    fake_ms += 16;
    eui_tick();
    assert(order_n == 2 && order[0] == 7 && order[1] == 8);

    /* 8) 队列满：投满后下一条失败并计数；已入队条目全部执行 */
    order_n = 0;
    int accepted = 0;
    while (eui_post(rec_cb, (void *)(intptr_t)0, NULL))
        accepted++;
    assert(accepted == EUI_POST_QUEUE_SIZE);
    assert(eui_post_count() == EUI_POST_QUEUE_SIZE);
    assert(eui_post_dropped() == 1);
    fake_ms += 16;
    eui_tick();
    assert(order_n == EUI_POST_QUEUE_SIZE);
    assert(eui_post_count() == 0);

    /* 9) 每帧预算：超预算的链式投递顺延到下一帧，不丢弃
     *    （注意：链式回调每次只续投 1 条，队列占用恒为 1 —— 「余量」体现在
     *    chain_left 的剩余执行次数上，而不是队列长度） */
    chain_ran = 0;
    chain_left = EUI_POST_DRAIN_MAX + 5;
    assert(eui_post(chain_cb, NULL, NULL));
    fake_ms += 16;
    eui_tick();
    assert(chain_ran == EUI_POST_DRAIN_MAX);   /* 本帧恰好执行预算数 */
    assert(eui_post_count() == 1);             /* 余下的执行次数以 1 条待执行链的形式顺延 */
    fake_ms += 16;
    eui_tick();
    assert(chain_ran == EUI_POST_DRAIN_MAX + 5);   /* 顺延部分未丢失，全部执行完 */
    assert(eui_post_count() == 0);

    /* 10) 回调内取消其他条目（重入稳定性）：X 执行时清掉同 owner 的 Y */
    order_n = 0;
    assert(eui_post(cancel_others_cb, NULL, (const void *)&dv));   /* X：owner 无关 */
    assert(eui_post(rec_cb, (void *)(intptr_t)100, (const void *)&dv));  /* Y：待取消 */
    fake_ms += 16;
    eui_tick();
    assert(order_n == 1 && order[0] == 200);   /* 见 cancel_others_cb 实现 */

    /* 11) deinit 清空且不执行；再次 init 后残留不得执行 */
    order_n = 0;
    assert(eui_post(rec_cb, (void *)(intptr_t)9, NULL));
    assert(eui_post_count() == 1);
    eui_deinit();
    assert(eui_post_count() == 0);
    assert(!eui_post(rec_cb, (void *)(intptr_t)10, NULL));   /* 未初始化：拒绝 */
    assert(eui_init(&cfg) == 0);
    eui_tick();
    assert(order_n == 0);

    printf("test_post passed\n");
    return 0;
}
