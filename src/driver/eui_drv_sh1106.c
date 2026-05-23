#include "eui/driver/eui_drv_sh1106.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t base;
    eui_hal_i2c_t     i2c;
    uint16_t          width;
    uint16_t          height;
    uint8_t           i2c_addr;
    uint8_t           col_offset;
} sh1106_t;

static int sh1106_init(void *ud) {
    sh1106_t *d = (sh1106_t*)ud;

    d->i2c.write_cmd(0xAE, d->i2c.user_data);
    d->i2c.write_cmd(0x40, d->i2c.user_data);
    d->i2c.write_cmd(0xA1, d->i2c.user_data);
    d->i2c.write_cmd(0xC8, d->i2c.user_data);
    d->i2c.write_cmd(0xA6, d->i2c.user_data);
    d->i2c.write_cmd(0xA8, d->i2c.user_data);
    d->i2c.write_cmd((uint8_t)(d->height - 1), d->i2c.user_data);
    d->i2c.write_cmd(0xD3, d->i2c.user_data);
    d->i2c.write_cmd(0x00, d->i2c.user_data);
    d->i2c.write_cmd(0xD5, d->i2c.user_data);
    d->i2c.write_cmd(0x80, d->i2c.user_data);
    d->i2c.write_cmd(0xD9, d->i2c.user_data);
    d->i2c.write_cmd(0x22, d->i2c.user_data);
    d->i2c.write_cmd(0xDB, d->i2c.user_data);
    d->i2c.write_cmd(0x35, d->i2c.user_data);
    d->i2c.write_cmd(0xDA, d->i2c.user_data);
    d->i2c.write_cmd(0x12, d->i2c.user_data);
    d->i2c.write_cmd(0x81, d->i2c.user_data);
    d->i2c.write_cmd(0xFF, d->i2c.user_data);
    d->i2c.write_cmd(0xA4, d->i2c.user_data);
    d->i2c.write_cmd(0x2E, d->i2c.user_data);
    d->i2c.delay_ms(100, d->i2c.user_data);
    d->i2c.write_cmd(0xAF, d->i2c.user_data);
    return 0;
}

static int sh1106_deinit(void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(0xAE, d->i2c.user_data);
    return 0;
}

static void sh1106_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void sh1106_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    uint8_t page = (uint8_t)(rect->y / 8);
    uint8_t col  = (uint8_t)(rect->x + d->col_offset);

    for (int16_t row = rect->y; row < rect->y + rect->h; row += 8) {
        d->i2c.write_cmd(0xB0 | page, d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)((col >> 4) | 0x10), d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(col & 0x0F), d->i2c.user_data);
        d->i2c.write_data(buf + (page - (uint8_t)(rect->y / 8)) * rect->w,
                          rect->w, d->i2c.user_data);
        page++;
    }
}

static void sh1106_set_contrast(uint8_t level, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(0x81, d->i2c.user_data);
    d->i2c.write_cmd(level, d->i2c.user_data);
}

static void sh1106_set_power(bool on, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(on ? 0xAF : 0xAE, d->i2c.user_data);
}

static void sh1106_set_invert(bool invert, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(invert ? 0xA7 : 0xA6, d->i2c.user_data);
}

static void sh1106_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_drv_t* eui_drv_sh1106_create(const eui_drv_sh1106_config_t *cfg) {
    sh1106_t *d = eui_malloc(sizeof(sh1106_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->i2c = cfg->i2c;
    d->width = cfg->width;
    d->height = cfg->height;
    d->i2c_addr = cfg->i2c_addr;
    d->col_offset = 2;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 1;
    d->base.caps.buffer_mode = EUI_BUFFER_PAGE;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = sh1106_init;
    d->base.deinit = sh1106_deinit;
    d->base.draw_pixel = sh1106_draw_pixel;
    d->base.write_buffer = sh1106_write_buffer;
    d->base.set_contrast = sh1106_set_contrast;
    d->base.set_power = sh1106_set_power;
    d->base.set_invert = sh1106_set_invert;
    d->base.fill_rect = sh1106_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_sh1106_destroy(eui_display_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
