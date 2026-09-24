#include "eui/driver/eui_drv_encoder_hw.h"
#include "eui/eui_allocator.h"
#include <limits.h>
#include <string.h>

typedef struct {
    eui_input_drv_t   base;
    eui_hal_encoder_t hw;
    int32_t           last_count;
    int32_t           carry;      /* 上一次因夹取未发出的余量 */
} encoder_hw_t;

static int enc_init(void *ud) {
    encoder_hw_t *e = (encoder_hw_t *)ud;
    e->last_count = e->hw.read_count(e->hw.user_data);
    e->carry = 0;
    return 0;
}

static int enc_deinit(void *ud) { (void)ud; return 0; }

static int enc_poll(eui_event_t *evt, void *ud) {
    encoder_hw_t *e = (encoder_hw_t *)ud;

    /* 先把上次的余量发掉，保证快速旋转的刻度不丢 */
    int32_t delta = e->carry;
    e->carry = 0;

    if (delta == 0) {
        int32_t now = e->hw.read_count(e->hw.user_data);
        int32_t diff = now - e->last_count;   /* int32 差值对回绕安全 */
        if (diff == 0) return 0;
        e->last_count = now;
        delta = diff;
    }

    /* 夹取到 int16（事件字段宽度），余量留到下一次 poll */
    if (delta > INT16_MAX) {
        e->carry = delta - INT16_MAX;
        delta = INT16_MAX;
    } else if (delta < -INT16_MAX) {
        e->carry = delta + INT16_MAX;
        delta = -INT16_MAX;
    }

    evt->type = (delta > 0) ? EUI_EVT_ENCODER_CW : EUI_EVT_ENCODER_CCW;
    evt->data.enc_delta = (int16_t)delta;
    return 1;
}

static void enc_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb; (void)user_data;
}

eui_input_drv_t *eui_drv_encoder_hw_create(const eui_drv_encoder_hw_config_t *cfg) {
    if (!cfg || !cfg->hw.read_count) return NULL;
    encoder_hw_t *e = eui_malloc(sizeof(encoder_hw_t));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->hw = cfg->hw;
    e->last_count = cfg->hw.read_count(cfg->hw.user_data);
    e->base.init = enc_init;
    e->base.deinit = enc_deinit;
    e->base.poll = enc_poll;
    e->base.set_callback = enc_set_callback;
    e->base.user_data = e;
    return &e->base;
}

void eui_drv_encoder_hw_destroy(eui_input_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
