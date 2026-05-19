#include "eui/driver/eui_drv_xpt2046.h"
#include "eui/eui_allocator.h"
#include <string.h>

#define XPT2046_CMD_X  0xD0
#define XPT2046_CMD_Y  0x90
#define XPT2046_SAMPLES 3

typedef struct {
    eui_input_hal_t        base;
    eui_hal_spi_t          spi;
    eui_drv_xpt2046_irq_t  irq;
    uint16_t               screen_w;
    uint16_t               screen_h;
    bool                   touched;
} xpt2046_t;

static void _xpt2046_read_xy(xpt2046_t *t, uint16_t *x, uint16_t *y) {
    uint16_t xvals[XPT2046_SAMPLES];
    uint16_t yvals[XPT2046_SAMPLES];

    for (int i = 0; i < XPT2046_SAMPLES; i++) {
        uint8_t tx_x[3] = { XPT2046_CMD_X, 0x00, 0x00 };
        uint8_t rx_x[3] = { 0 };
        t->spi.set_cs(false, t->spi.user_data);
        t->spi.write_data(tx_x, 3, t->spi.user_data);
        t->spi.read_data(rx_x, 3, t->spi.user_data);
        t->spi.set_cs(true, t->spi.user_data);
        uint16_t raw_x = ((uint16_t)(rx_x[1] & 0xFF) << 4) | ((rx_x[2] >> 4) & 0x0F);

        uint8_t tx_y[3] = { XPT2046_CMD_Y, 0x00, 0x00 };
        uint8_t rx_y[3] = { 0 };
        t->spi.set_cs(false, t->spi.user_data);
        t->spi.write_data(tx_y, 3, t->spi.user_data);
        t->spi.read_data(rx_y, 3, t->spi.user_data);
        t->spi.set_cs(true, t->spi.user_data);
        uint16_t raw_y = ((uint16_t)(rx_y[1] & 0xFF) << 4) | ((rx_y[2] >> 4) & 0x0F);

        xvals[i] = raw_x;
        yvals[i] = raw_y;
    }

    /* insertion sort for median */
    for (int i = 1; i < XPT2046_SAMPLES; i++) {
        uint16_t tmp = xvals[i];
        int j = i - 1;
        while (j >= 0 && xvals[j] > tmp) { xvals[j + 1] = xvals[j]; j--; }
        xvals[j + 1] = tmp;
    }
    for (int i = 1; i < XPT2046_SAMPLES; i++) {
        uint16_t tmp = yvals[i];
        int j = i - 1;
        while (j >= 0 && yvals[j] > tmp) { yvals[j + 1] = yvals[j]; j--; }
        yvals[j + 1] = tmp;
    }

    *x = xvals[XPT2046_SAMPLES / 2];
    *y = yvals[XPT2046_SAMPLES / 2];
}

static int xpt2046_init(void *ud) {
    xpt2046_t *t = (xpt2046_t*)ud;
    t->touched = false;
    return 0;
}

static int xpt2046_deinit(void *ud) { (void)ud; return 0; }

static int xpt2046_poll(eui_event_t *evt, void *ud) {
    xpt2046_t *t = (xpt2046_t*)ud;
    bool pressed = !t->irq.read_irq(t->irq.user_data);

    if (pressed) {
        uint16_t rx, ry;
        _xpt2046_read_xy(t, &rx, &ry);
        int16_t sx = (int16_t)((uint32_t)rx * t->screen_w / 4096);
        int16_t sy = (int16_t)((uint32_t)ry * t->screen_h / 4096);

        evt->type = t->touched ? EUI_EVT_TOUCH_MOVE : EUI_EVT_TOUCH_DOWN;
        evt->data.touch.x = sx;
        evt->data.touch.y = sy;
        t->touched = true;
        return 1;
    }

    if (t->touched) {
        evt->type = EUI_EVT_TOUCH_UP;
        t->touched = false;
        return 1;
    }

    return 0;
}

static void xpt2046_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb; (void)user_data;
}

eui_input_hal_t* eui_drv_xpt2046_create(const eui_drv_xpt2046_config_t *cfg) {
    xpt2046_t *t = eui_malloc(sizeof(xpt2046_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->spi = cfg->spi;
    t->irq = cfg->irq;
    t->screen_w = cfg->screen_width;
    t->screen_h = cfg->screen_height;
    t->base.init = xpt2046_init;
    t->base.deinit = xpt2046_deinit;
    t->base.poll = xpt2046_poll;
    t->base.set_callback = xpt2046_set_callback;
    t->base.user_data = t;
    return &t->base;
}

void eui_drv_xpt2046_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
