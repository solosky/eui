#include "eui/eui.h"
#include <string.h>

static struct {
    bool initialized;
    eui_config_t config;
} g_eui;

int eui_init(const eui_config_t *config) {
    if (!config || !config->display || !config->input) return -1;
    if (config->mem_pool_buffer) {
        eui_allocator_init_tlsf(config->mem_pool_buffer, config->mem_pool_size);
    }
    g_eui.config = *config;
    g_eui.initialized = true;
    return 0;
}

void eui_deinit(void) {
    g_eui.initialized = false;
}

void eui_tick(void) {
    if (!g_eui.initialized) return;
}

bool eui_is_running(void) {
    return g_eui.initialized;
}

void eui_set_fps(uint16_t fps) {
    g_eui.config.fps_target = fps;
}

uint16_t eui_get_fps(void) {
    return g_eui.config.fps_target;
}
