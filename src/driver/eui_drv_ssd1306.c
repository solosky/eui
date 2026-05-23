#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t    base;
    eui_hal_i2c_t        i2c;
    uint16_t             width;
    uint16_t             height;
    uint8_t              i2c_addr;
} ssd1306_t;

static int ssd1306_init(void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;

    static const uint8_t init_cmds[] = {
        0xAE,
        0xD5, 0x80,
        0xA8,
        0xD3, 0x00,
        0x40,
        0x8D,
        0x20, 0x00,
        0xA1,
        0xC8,
        0xDA,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0x2E,
        0xAF,
    };
    uint8_t cmds[sizeof(init_cmds)];
    memcpy(cmds, init_cmds, sizeof(init_cmds));

    if (d->height <= 32) {
        cmds[4]  = 0x1F;
        cmds[10] = 0x10;
        cmds[18] = 0x02;
    } else {
        cmds[4]  = 0x3F;
        cmds[10] = 0x14;
        cmds[18] = 0x12;
    }

    for (size_t i = 0; i < sizeof(cmds); i++) {
        d->i2c.write_cmd(cmds[i], d->i2c.user_data);
    }
    return 0;
}

static int ssd1306_deinit(void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(0xAE, d->i2c.user_data);
    return 0;
}

static void ssd1306_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void ssd1306_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    uint8_t page_start = (uint8_t)(rect->y / 8);
    uint8_t page_end   = (uint8_t)((rect->y + rect->h - 1) / 8);

    for (uint8_t p = page_start; p <= page_end; p++) {
        d->i2c.write_cmd(0xB0 | p, d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(rect->x & 0x0F), d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(0x10 | ((rect->x >> 4) & 0x0F)), d->i2c.user_data);
        d->i2c.write_data(buf + (p - page_start) * rect->w, rect->w, d->i2c.user_data);
    }
}

static void ssd1306_set_contrast(uint8_t level, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(0x81, d->i2c.user_data);
    d->i2c.write_cmd(level, d->i2c.user_data);
}

static void ssd1306_set_power(bool on, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(on ? 0xAF : 0xAE, d->i2c.user_data);
}

static void ssd1306_set_invert(bool invert, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(invert ? 0xA7 : 0xA6, d->i2c.user_data);
}

static void ssd1306_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                              eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_drv_t* eui_drv_ssd1306_create(const eui_drv_ssd1306_config_t *cfg) {
    ssd1306_t *d = eui_malloc(sizeof(ssd1306_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->i2c = cfg->i2c;
    d->width = cfg->width;
    d->height = cfg->height;
    d->i2c_addr = cfg->i2c_addr;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 1;
    d->base.caps.buffer_mode = EUI_BUFFER_PAGE;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = ssd1306_init;
    d->base.deinit = ssd1306_deinit;
    d->base.draw_pixel = ssd1306_draw_pixel;
    d->base.write_buffer = ssd1306_write_buffer;
    d->base.set_contrast = ssd1306_set_contrast;
    d->base.set_power = ssd1306_set_power;
    d->base.set_invert = ssd1306_set_invert;
    d->base.fill_rect = ssd1306_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_ssd1306_destroy(eui_display_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
