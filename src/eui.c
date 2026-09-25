#include "eui/eui.h"
#include "eui/eui_canvas.h"
#include "eui/eui_input.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_anim.h"
#include "eui/eui_post.h"
#include <string.h>

static struct {
    bool initialized;
    eui_config_t config;
    eui_canvas_t *canvas;
    eui_input_manager_t input_mgr;
    eui_view_dispatcher_t vd;
    eui_view_dispatcher_t *active_vd;   /* NULL = 用内部 vd */
    eui_view_dispatcher_t *chrome_vd;   /* 预留：chrome 层 dispatcher，当前恒为 NULL */
    uint32_t last_tick_ms;
    uint32_t (*tick_fn)(void);
} g_eui;

int eui_init(const eui_config_t *config) {
    if (!config || !config->display || !config->input) return -1;

    /* 池只建一次：transport/driver 先于 eui_init 装配的 brick（池初始化在
     * 更早处完成）已分配过块，此处重建会把它们的元数据清掉——真机上表现
     * 为 HAL 函数指针被覆写成野值（InstrFetchProhibited）。 */
    if (config->mem_pool_buffer && !eui_allocator_is_initialized()) {
        eui_allocator_init_tlsf(config->mem_pool_buffer, config->mem_pool_size);
    }

    g_eui.config = *config;
    g_eui.canvas = eui_canvas_create(config->display);
    if (!g_eui.canvas) return -1;

    eui_input_init(&g_eui.input_mgr, config->input);
    eui_view_dispatcher_init(&g_eui.vd, g_eui.canvas, eui_get_tick_ms);
    eui_anim_init();
    eui_post_reset();
    g_eui.last_tick_ms = 0;
    g_eui.active_vd = NULL;
    g_eui.chrome_vd = NULL;
    g_eui.initialized = true;
    return 0;
}

void eui_deinit(void) {
    if (g_eui.canvas) eui_canvas_destroy(g_eui.canvas);
    g_eui.canvas = NULL;
    eui_post_reset();   /* 清空未执行条目（不执行回调） */
    g_eui.active_vd = NULL;
    g_eui.chrome_vd = NULL;
    g_eui.initialized = false;
}

void eui_tick(void) {
    if (!g_eui.initialized) return;

    uint32_t now = eui_get_tick_ms();
    uint32_t delta = now - g_eui.last_tick_ms;
    g_eui.last_tick_ms = now;
    if (delta == 0) delta = 1;

    /* Step 1: Update animations */
    eui_anim_update(delta);

    /* Step 2: Poll input */
    eui_input_update(&g_eui.input_mgr, now);

    /* 活动 dispatcher：未注册时回退内部 vd */
    eui_view_dispatcher_t *vd = g_eui.active_vd ? g_eui.active_vd : &g_eui.vd;

    /* Step 3: raw → 手势装配 → 路由 */
    eui_event_t raw;
    while (eui_input_get_event(&g_eui.input_mgr, &raw))
        eui_input_edge_on_event(&vd->edge, &raw);
    eui_input_edge_tick(&vd->edge, now);
    eui_event_t gesture;
    while (eui_input_edge_pop_event(&vd->edge, &gesture))
        eui_view_dispatcher_send_input(vd, &gesture);

    /* Step 4: Render —— 主视图画入画布（不提交）。
     * Step 5: chrome 层（系统 status bar）在同一画布上叠画（不清屏、
     *         最后画、浮于主视图与过渡动画之上；chrome 的 input edge
     *         永不泵（不可点））。
     * Step 5.5: 整帧一次提交。此前主视图与 chrome 各自 commit（各推一次
     *         整帧），面板 GRAM 一半时间没有状态条 → 状态条以帧率一半
     *         的频率闪烁（真机实测）。 */
    eui_view_dispatcher_render(vd);

    if (g_eui.chrome_vd) {
        eui_view_t *cv = eui_view_dispatcher_get_active(g_eui.chrome_vd);
        if (cv)
            eui_view_send_draw(cv, g_eui.chrome_vd->canvas);
    }

    eui_canvas_commit(g_eui.canvas);

    /* Step 6: post 队列排空（帧尾延迟执行）——渲染与 chrome 均已 commit，
     * 本步回调若绘制则只写 canvas、不 commit（下一帧 Step 4 提交），与
     * 「宿主在 eui_tick() 返回后派发」的提交边界一致。 */
    eui_post_drain();
}

bool eui_is_running(void) { return g_eui.initialized; }

eui_view_dispatcher_t* eui_get_view_dispatcher(void) {
    return &g_eui.vd;
}

eui_input_edge_t* eui_get_input_edge(void) {
    return &(g_eui.active_vd ? g_eui.active_vd : &g_eui.vd)->edge;
}

void eui_set_active_dispatcher(eui_view_dispatcher_t *vd) {
    g_eui.active_vd = vd;
}

eui_view_dispatcher_t* eui_get_active_dispatcher(void) {
    return g_eui.active_vd ? g_eui.active_vd : &g_eui.vd;
}

eui_canvas_t* eui_get_canvas(void) {
    return g_eui.canvas;
}

void eui_set_chrome_dispatcher(eui_view_dispatcher_t *vd) {
    g_eui.chrome_vd = vd;   /* NULL = 隐藏 chrome 层 */
}

eui_display_drv_t* eui_get_display(void) {
    return g_eui.config.display;
}

void eui_set_fps(uint16_t fps) { g_eui.config.fps_target = fps; }

uint16_t eui_get_fps(void) { return g_eui.config.fps_target; }

void eui_set_tick_callback(uint32_t (*tick_fn)(void)) {
    g_eui.tick_fn = tick_fn;
}

uint32_t eui_get_tick_ms(void) {
    return g_eui.tick_fn ? g_eui.tick_fn() : 0;
}

uint8_t eui_input_pending(void) {
    if (!g_eui.initialized) return 0;
    return eui_event_queue_count(&g_eui.input_mgr.queue);
}
