/* chrome 层泵点：eui_tick 末尾绘制 chrome vd 的活动视图，浮于主视图之上。 */
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

/* 主视图：全屏白 */
static bool main_draw(eui_view_event_t *event, void *context) {
    (void)context;
    if (event->type == EUI_VIEW_EVT_DRAW) {
        eui_canvas_t *c = event->event.draw.canvas;
        eui_canvas_set_color(c, 0xFFFF);
        eui_canvas_fill_rect(c, 0, 0, 64, 64);
    }
    return false;
}

/* chrome 视图：顶部 8 行黑条 */
static bool chrome_draw(eui_view_event_t *event, void *context) {
    (void)context;
    if (event->type == EUI_VIEW_EVT_DRAW) {
        eui_canvas_t *c = event->event.draw.canvas;
        eui_canvas_set_color(c, 0x0000);
        eui_canvas_fill_rect(c, 0, 0, 64, 8);
    }
    return false;
}

#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000

static uint16_t px(eui_canvas_t *canvas, int x, int y) {
    return ((uint16_t *)canvas->buffer)[y * 64 + x];
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

    eui_view_t main_view, chrome_view;
    eui_view_init(&main_view, main_draw, NULL);
    eui_view_init(&chrome_view, chrome_draw, NULL);

    /* 主 vd = 内部 vd；chrome vd 自建，绑全局 canvas */
    assert(eui_view_dispatcher_add(eui_get_view_dispatcher(), 1, &main_view) == 0);
    eui_view_dispatcher_switch_to(eui_get_view_dispatcher(), 1, EUI_ANIM_NONE);

    eui_view_dispatcher_t chrome_vd;
    eui_view_dispatcher_init(&chrome_vd, eui_get_canvas(), eui_get_tick_ms);
    assert(eui_view_dispatcher_add(&chrome_vd, 1, &chrome_view) == 0);
    eui_view_dispatcher_switch_to(&chrome_vd, 1, EUI_ANIM_NONE);

    /* 未注册 chrome：整屏白 */
    fake_ms += 16;
    eui_tick();
    eui_canvas_t *canvas = eui_get_canvas();
    assert(px(canvas, 32, 0) == COLOR_WHITE);
    assert(px(canvas, 32, 32) == COLOR_WHITE);

    /* 注册 chrome vd：顶部黑条、其余白 */
    eui_set_chrome_dispatcher(&chrome_vd);
    fake_ms += 16;
    eui_tick();
    assert(px(canvas, 0, 0) == COLOR_BLACK);
    assert(px(canvas, 63, 7) == COLOR_BLACK);
    assert(px(canvas, 0, 8) == COLOR_WHITE);
    assert(px(canvas, 32, 32) == COLOR_WHITE);

    /* 注销 chrome：顶部恢复白 */
    eui_set_chrome_dispatcher(NULL);
    fake_ms += 16;
    eui_tick();
    assert(px(canvas, 0, 0) == COLOR_WHITE);
    assert(px(canvas, 32, 32) == COLOR_WHITE);

    printf("test_chrome_layer passed\n");
    return 0;
}
