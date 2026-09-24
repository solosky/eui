#include "eui/driver/eui_drv_st7789.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
    uint8_t           col_offset;
    uint8_t           row_offset;
    uint8_t           madctl;
    bool              invert;
    bool              little_endian;
} st7789_t;

#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_NORON   0x13
#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_RAMCTL  0xB0
#define ST7789_COLMOD  0x3A

static void st7789_write_cmd(st7789_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void st7789_write_data(st7789_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void st7789_set_addr_window(st7789_t *d, uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h) {
    uint16_t xs = x + d->col_offset, xe = xs + w - 1;
    uint16_t ys = y + d->row_offset, ye = ys + h - 1;
    uint8_t caset[4] = { (uint8_t)(xs >> 8), (uint8_t)(xs), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t raset[4] = { (uint8_t)(ys >> 8), (uint8_t)(ys), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7789_write_cmd(d, ST7789_CASET);
    st7789_write_data(d, caset, 4);
    st7789_write_cmd(d, ST7789_RASET);
    st7789_write_data(d, raset, 4);
}

static int st7789_init(void *ud) {
    st7789_t *d = (st7789_t*)ud;

    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    st7789_write_cmd(d, ST7789_SWRESET);
    d->spi.delay_ms(150, d->spi.user_data);
    st7789_write_cmd(d, ST7789_SLPOUT);
    d->spi.delay_ms(120, d->spi.user_data);

    st7789_write_cmd(d, ST7789_COLMOD);
    { uint8_t v = 0x55; st7789_write_data(d, &v, 1); }   /* 16bit/pixel RGB565 */

    st7789_write_cmd(d, ST7789_MADCTL);
    { uint8_t v = d->madctl; st7789_write_data(d, &v, 1); }

    /* RAMCTL(0xB0)：RGB565 字节序交给面板，CPU 侧零成本（不需要软件交换
     * 或 bounce buffer）。bit3 = 1 为 LSB 先。取值参考 IDF esp_lcd 的
     * ST7789 面板驱动（LCD_RGB_DATA_ENDIAN_BIG/LITTLE 语义）。 */
    st7789_write_cmd(d, ST7789_RAMCTL);
    { uint8_t v[2] = { 0x00, (uint8_t)(0xF0 | (d->little_endian ? (1u << 3) : 0u)) };
      st7789_write_data(d, v, 2); }

    { uint8_t v = d->invert ? ST7789_INVON : ST7789_INVOFF;
      st7789_write_cmd(d, v); }

    st7789_write_cmd(d, ST7789_NORON);
    d->spi.delay_ms(10, d->spi.user_data);
    st7789_write_cmd(d, ST7789_DISPON);
    d->spi.delay_ms(100, d->spi.user_data);

    st7789_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int st7789_deinit(void *ud) { (void)ud; return 0; }

static void st7789_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void st7789_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    st7789_t *d = (st7789_t*)ud;
    st7789_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y, rect->w, rect->h);
    st7789_write_cmd(d, ST7789_RAMWR);
    d->spi.set_dc(true, d->spi.user_data);
    uint32_t len = (uint32_t)rect->w * rect->h * 2;
    d->spi.write_data(buf, len, d->spi.user_data);
}

static void st7789_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void st7789_set_power(bool on, void *ud) { (void)on; (void)ud; }

static void st7789_set_invert(bool invert, void *ud) {
    st7789_t *d = (st7789_t*)ud;
    st7789_write_cmd(d, invert ? ST7789_INVON : ST7789_INVOFF);
}

static void st7789_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_drv_t* eui_drv_st7789_create(const eui_drv_st7789_config_t *cfg) {
    if (!cfg) return NULL;
    st7789_t *d = eui_malloc(sizeof(st7789_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->spi = cfg->spi;
    d->width = cfg->width;
    d->height = cfg->height;
    d->col_offset = cfg->col_offset;
    d->row_offset = cfg->row_offset;
    d->madctl = cfg->madctl;
    d->invert = cfg->invert;
    d->little_endian = cfg->little_endian;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 16;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = st7789_init;
    d->base.deinit = st7789_deinit;
    d->base.draw_pixel = st7789_draw_pixel;
    d->base.write_buffer = st7789_write_buffer;
    d->base.set_contrast = st7789_set_contrast;
    d->base.set_power = st7789_set_power;
    d->base.set_invert = st7789_set_invert;
    d->base.fill_rect = st7789_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_st7789_destroy(eui_display_drv_t *hal) {
    if (hal) eui_free(hal->user_data);
}
