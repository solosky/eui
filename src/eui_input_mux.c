#include "eui/eui_input_mux.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_input_drv_t         base;
    eui_input_drv_t *const *drivers;
    uint8_t                 count;
} mux_t;

static int mux_init(void *ud) {
    mux_t *m = (mux_t *)ud;
    for (uint8_t i = 0; i < m->count; i++) {
        if (m->drivers[i] && m->drivers[i]->init) {
            if (m->drivers[i]->init(m->drivers[i]->user_data) != 0) return -1;
        }
    }
    return 0;
}

static int mux_deinit(void *ud) {
    mux_t *m = (mux_t *)ud;
    for (uint8_t i = 0; i < m->count; i++) {
        if (m->drivers[i] && m->drivers[i]->deinit) {
            m->drivers[i]->deinit(m->drivers[i]->user_data);
        }
    }
    return 0;
}

static int mux_poll(eui_event_t *evt, void *ud) {
    mux_t *m = (mux_t *)ud;
    for (uint8_t i = 0; i < m->count; i++) {
        eui_input_drv_t *d = m->drivers[i];
        if (!d || !d->poll) continue;
        if (d->poll(evt, d->user_data) > 0) return 1;   /* 契约：>0 = 产出事件 */
    }
    return 0;
}

static void mux_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb; (void)user_data;   /* 多源回调语义不明确，交由平台自行接管 */
}

eui_input_drv_t *eui_input_mux_create(const eui_input_mux_config_t *cfg) {
    if (!cfg || !cfg->drivers || cfg->count == 0) return NULL;

    mux_t *m = eui_malloc(sizeof(mux_t));
    if (!m) return NULL;
    memset(m, 0, sizeof(*m));
    m->drivers = cfg->drivers;
    m->count = cfg->count;
    m->base.init = mux_init;
    m->base.deinit = mux_deinit;
    m->base.poll = mux_poll;
    m->base.set_callback = mux_set_callback;
    m->base.user_data = m;
    return &m->base;
}

void eui_input_mux_destroy(eui_input_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
