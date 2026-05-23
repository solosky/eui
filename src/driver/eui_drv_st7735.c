#include "eui/driver/eui_drv_st7735.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
} st7735_t;

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

static void st7735_write_cmd(st7735_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void st7735_write_data(st7735_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void st7735_set_addr_window(st7735_t *d, uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;
    uint8_t caset[4] = { (uint8_t)(x >> 8), (uint8_t)(x), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t raset[4] = { (uint8_t)(y >> 8), (uint8_t)(y), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7735_write_cmd(d, ST7735_CASET);
    st7735_write_data(d, caset, 4);
    st7735_write_cmd(d, ST7735_RASET);
    st7735_write_data(d, raset, 4);
}

static int st7735_init(void *ud) {
    st7735_t *d = (st7735_t*)ud;

    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    st7735_write_cmd(d, ST7735_SWRESET);
    d->spi.delay_ms(150, d->spi.user_data);
    st7735_write_cmd(d, ST7735_SLPOUT);
    d->spi.delay_ms(500, d->spi.user_data);

    st7735_write_cmd(d, ST7735_COLMOD);
    { uint8_t v = 0x05; st7735_write_data(d, &v, 1); }

    st7735_write_cmd(d, ST7735_MADCTL);
    { uint8_t v = 0xC8; st7735_write_data(d, &v, 1); }

    st7735_write_cmd(d, ST7735_NORON);
    d->spi.delay_ms(10, d->spi.user_data);
    st7735_write_cmd(d, ST7735_DISPON);
    d->spi.delay_ms(100, d->spi.user_data);

    st7735_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int st7735_deinit(void *ud) { (void)ud; return 0; }

static void st7735_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void st7735_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    st7735_t *d = (st7735_t*)ud;
    st7735_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y, rect->w, rect->h);
    st7735_write_cmd(d, ST7735_RAMWR);
    d->spi.set_dc(true, d->spi.user_data);
    uint32_t len = (uint32_t)rect->w * rect->h * 2;
    d->spi.write_data(buf, len, d->spi.user_data);
}

static void st7735_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void st7735_set_power(bool on, void *ud) { (void)on; (void)ud; }

static void st7735_set_invert(bool invert, void *ud) {
    st7735_t *d = (st7735_t*)ud;
    st7735_write_cmd(d, invert ? 0x21 : 0x20);
}

static void st7735_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_drv_t* eui_drv_st7735_create(const eui_drv_st7735_config_t *cfg) {
    st7735_t *d = eui_malloc(sizeof(st7735_t));
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
    d->base.init = st7735_init;
    d->base.deinit = st7735_deinit;
    d->base.draw_pixel = st7735_draw_pixel;
    d->base.write_buffer = st7735_write_buffer;
    d->base.set_contrast = st7735_set_contrast;
    d->base.set_power = st7735_set_power;
    d->base.set_invert = st7735_set_invert;
    d->base.fill_rect = st7735_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_st7735_destroy(eui_display_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
