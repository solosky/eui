/* 活动 dispatcher 切换：输入路由与渲染跟随 eui_set_active_dispatcher。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <eui/eui.h>
#include <eui/eui_input_edge.h>

static uint8_t pool[128 * 1024];
static uint32_t fake_ms;
static uint32_t get_tick(void) { return fake_ms; }

static int stub_init(void *ud) { (void)ud; return 0; }
static void stub_flush(const uint8_t *b, const eui_rect_t *r, void *ud) { (void)b; (void)r; (void)ud; }
static int in_init(void *ud) { (void)ud; return 0; }
static int in_poll(eui_event_t *e, void *ud) { (void)e; (void)ud; return 0; }

/* 两个计数视图：ENTER 次数记账，断言事件路由到了"活动"的那个 */
typedef struct {
    int enters;
    eui_view_t view;
} cnt_view_t;

static bool cnt_handler(eui_view_event_t *event, void *context) {
    cnt_view_t *cv = (cnt_view_t *)context;
    if (event->type == EUI_VIEW_EVT_ENTER) cv->enters++;
    return false;
}

static void cnt_init(cnt_view_t *cv) {
    memset(cv, 0, sizeof(*cv));
    eui_view_init(&cv->view, cnt_handler, cv);
}

int main(void)
{
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

    cnt_view_t a, b;
    cnt_init(&a); cnt_init(&b);

    /* canvas getter：归 eui 所有，非 NULL */
    assert(eui_get_canvas() != NULL);

    /* 默认：无活动注册，行为 = 内部 vd（把 a 注册进内部 vd） */
    assert(eui_get_active_dispatcher() == eui_get_view_dispatcher());
    eui_set_active_dispatcher(NULL);
    assert(eui_view_dispatcher_add(eui_get_view_dispatcher(), 1, &a.view) == 0);
    eui_view_dispatcher_switch_to(eui_get_view_dispatcher(), 1, EUI_ANIM_NONE);
    assert(eui_get_input_edge() == &eui_get_view_dispatcher()->edge);
    assert(a.enters == 1 && b.enters == 0);

    /* 自建 vd_b（绑全局 canvas），注册为活动：edge 跟随、输入路由跟随 */
    eui_view_dispatcher_t vd_b;
    eui_view_dispatcher_init(&vd_b, eui_get_canvas(), eui_get_tick_ms);
    assert(eui_view_dispatcher_add(&vd_b, 2, &b.view) == 0);
    eui_set_active_dispatcher(&vd_b);
    assert(eui_get_input_edge() == &vd_b.edge);
    assert(eui_get_active_dispatcher() == &vd_b);

    eui_view_dispatcher_switch_to(&vd_b, 2, EUI_ANIM_NONE);
    assert(b.enters == 1 && a.enters == 1);   /* enter 在 switch_to 时派发，未重复 */
    fake_ms += 16;
    eui_tick();                                /* 泵活动 vd_b，不触碰内部 vd 上的 a */
    assert(b.enters == 1 && a.enters == 1);

    /* 回退 NULL：恢复内部 vd */
    eui_set_active_dispatcher(NULL);
    assert(eui_get_input_edge() == &eui_get_view_dispatcher()->edge);
    assert(eui_get_active_dispatcher() == eui_get_view_dispatcher());

    printf("test_dispatcher_active passed\n");
    return 0;
}
