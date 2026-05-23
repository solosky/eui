#include "eui/driver/eui_drv_ili9341.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
} ili9341_t;

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_SLPOUT  0x11
#define ILI9341_DISPON  0x29
#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

static void ili9341_write_cmd(ili9341_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void ili9341_write_data(ili9341_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void ili9341_set_addr_window(ili9341_t *d, uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;
    uint8_t caset[4] = { (uint8_t)(x >> 8), (uint8_t)(x), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t paset[4] = { (uint8_t)(y >> 8), (uint8_t)(y), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    ili9341_write_cmd(d, ILI9341_CASET);
    ili9341_write_data(d, caset, 4);
    ili9341_write_cmd(d, ILI9341_PASET);
    ili9341_write_data(d, paset, 4);
}

static int ili9341_init(void *ud) {
    ili9341_t *d = (ili9341_t*)ud;

    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    ili9341_write_cmd(d, ILI9341_SWRESET);
    d->spi.delay_ms(150, d->spi.user_data);
    ili9341_write_cmd(d, ILI9341_SLPOUT);
    d->spi.delay_ms(500, d->spi.user_data);

    ili9341_write_cmd(d, ILI9341_PIXFMT);
    { uint8_t v = 0x55; ili9341_write_data(d, &v, 1); }
    ili9341_write_cmd(d, ILI9341_MADCTL);
    { uint8_t v = 0x48; ili9341_write_data(d, &v, 1); }

    ili9341_write_cmd(d, ILI9341_DISPON);
    d->spi.delay_ms(100, d->spi.user_data);

    ili9341_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int ili9341_deinit(void *ud) { (void)ud; return 0; }
static void ili9341_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void ili9341_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    ili9341_t *d = (ili9341_t*)ud;
    ili9341_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y, rect->w, rect->h);
    ili9341_write_cmd(d, ILI9341_RAMWR);
    d->spi.set_dc(true, d->spi.user_data);
    uint32_t len = (uint32_t)rect->w * rect->h * 2;
    d->spi.write_data(buf, len, d->spi.user_data);
}

static void ili9341_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void ili9341_set_power(bool on, void *ud) { (void)on; (void)ud; }
static void ili9341_set_invert(bool invert, void *ud) {
    ili9341_t *d = (ili9341_t*)ud;
    ili9341_write_cmd(d, invert ? 0x21 : 0x20);
}
static void ili9341_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                              eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_drv_t* eui_drv_ili9341_create(const eui_drv_ili9341_config_t *cfg) {
    ili9341_t *d = eui_malloc(sizeof(ili9341_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->spi = cfg->spi;
    d->width = cfg->width;
    d->height = cfg->height;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 16;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = ili9341_init;
    d->base.deinit = ili9341_deinit;
    d->base.draw_pixel = ili9341_draw_pixel;
    d->base.write_buffer = ili9341_write_buffer;
    d->base.set_contrast = ili9341_set_contrast;
    d->base.set_power = ili9341_set_power;
    d->base.set_invert = ili9341_set_invert;
    d->base.fill_rect = ili9341_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_ili9341_destroy(eui_display_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
