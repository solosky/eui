#include "eui/eui.h"
#include "eui/eui_canvas.h"
#include "eui/eui_input.h"
#include "eui/eui_view_dispatcher.h"
#include "eui/eui_anim.h"
#include <string.h>

static struct {
    bool initialized;
    eui_config_t config;
    eui_canvas_t *canvas;
    eui_input_manager_t input_mgr;
    eui_view_dispatcher_t vd;
    uint32_t last_tick_ms;
    uint32_t (*tick_fn)(void);
} g_eui;

int eui_init(const eui_config_t *config) {
    if (!config || !config->display || !config->input) return -1;

    if (config->mem_pool_buffer) {
        eui_allocator_init_tlsf(config->mem_pool_buffer, config->mem_pool_size);
    }

    g_eui.config = *config;
    g_eui.canvas = eui_canvas_create(config->display);
    if (!g_eui.canvas) return -1;

    eui_input_init(&g_eui.input_mgr, config->input);
    eui_view_dispatcher_init(&g_eui.vd, g_eui.canvas);
    eui_anim_init();
    g_eui.last_tick_ms = 0;
    g_eui.initialized = true;
    return 0;
}

void eui_deinit(void) {
    if (g_eui.canvas) eui_canvas_destroy(g_eui.canvas);
    g_eui.canvas = NULL;
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

    /* Step 3: Process input events */
    eui_event_t evt;
    while (eui_input_get_event(&g_eui.input_mgr, &evt)) {
        eui_view_dispatcher_send_input(&g_eui.vd, &evt);
    }

    /* Step 4: Render */
    eui_view_dispatcher_tick(&g_eui.vd);
}

bool eui_is_running(void) { return g_eui.initialized; }

void eui_set_fps(uint16_t fps) { g_eui.config.fps_target = fps; }

uint16_t eui_get_fps(void) { return g_eui.config.fps_target; }

void eui_set_tick_callback(uint32_t (*tick_fn)(void)) {
    g_eui.tick_fn = tick_fn;
}

uint32_t eui_get_tick_ms(void) {
    return g_eui.tick_fn ? g_eui.tick_fn() : 0;
}
