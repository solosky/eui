#include "eui/driver/eui_drv_st7306.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_drv_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
    uint8_t          *framebuffer;  /* 150 * 200 = 30000 bytes */
} st7306_t;

/* --- ST7306 commands --- */
#define ST7306_SWRESET  0x01
#define ST7306_SLPIN    0x10
#define ST7306_SLPOUT   0x11
#define ST7306_INVOFF   0x20
#define ST7306_INVON    0x21
#define ST7306_DISPOFF  0x28
#define ST7306_DISPON   0x29
#define ST7306_CASET    0x2A
#define ST7306_RASET    0x2B
#define ST7306_RAMWR    0x2C
#define ST7306_MADCTL   0x36
#define ST7306_COLMOD   0x3A

/* Buffer: 150 bytes per row × 200 rows for 300×400 logical pixels */
#define ST7306_BUF_W    150
#define ST7306_BUF_H    200
#define ST7306_BUF_SIZE (ST7306_BUF_W * ST7306_BUF_H)

/* --- Private helpers --- */
static void st7306_write_cmd(st7306_t *d, uint8_t cmd)
{
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void st7306_write_data(st7306_t *d, const uint8_t *data, uint32_t len)
{
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void st7306_write_cmd_multi(st7306_t *d, uint8_t cmd,
                                   const uint8_t *data, uint32_t len)
{
    st7306_write_cmd(d, cmd);
    if (len > 0)
        st7306_write_data(d, data, len);
}

/* --- Init --- */
static int st7306_init(void *ud)
{
    st7306_t *d = (st7306_t *)ud;

    /* Hardware reset */
    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(10, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(10, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(10, d->spi.user_data);

    /* NVM Load Control */
    {
        uint8_t dat[] = { 0x17, 0x02 };
        st7306_write_cmd_multi(d, 0xD6, dat, 2);
    }
    /* Booster Enable */
    {
        uint8_t dat[] = { 0x01 };
        st7306_write_cmd_multi(d, 0xD1, dat, 1);
    }
    /* Gate Voltage (VGH=17V, VGL=-10V) */
    {
        uint8_t dat[] = { 0x12, 0x0A };
        st7306_write_cmd_multi(d, 0xC0, dat, 2);
    }
    /* VSHP (4.8V) */
    {
        uint8_t dat[] = { 115, 0x3E, 0x3C, 0x3C };
        st7306_write_cmd_multi(d, 0xC1, dat, 4);
    }
    /* VSLP (0.98V) */
    {
        uint8_t dat[] = { 0, 0x21, 0x23, 0x23 };
        st7306_write_cmd_multi(d, 0xC2, dat, 4);
    }
    /* VSHN (-3.6V) */
    {
        uint8_t dat[] = { 50, 0x5C, 0x5A, 0x5A };
        st7306_write_cmd_multi(d, 0xC4, dat, 4);
    }
    /* VSLN (0.22V) */
    {
        uint8_t dat[] = { 50, 0x35, 0x37, 0x37 };
        st7306_write_cmd_multi(d, 0xC5, dat, 4);
    }
    /* OSC Setting (HPM=32hz) */
    {
        uint8_t dat[] = { 0xA6, 0xE9 };
        st7306_write_cmd_multi(d, 0xD8, dat, 2);
    }
    /* Frame Rate (HPM=32hz, LPM=1hz) */
    {
        uint8_t dat[] = { 0x12 };
        st7306_write_cmd_multi(d, 0xB2, dat, 1);
    }
    /* Gate EQ HPM */
    {
        uint8_t dat[] = { 0xE5, 0xF6, 0x17, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x71 };
        st7306_write_cmd_multi(d, 0xB3, dat, 10);
    }
    /* Gate EQ LPM */
    {
        uint8_t dat[] = { 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45 };
        st7306_write_cmd_multi(d, 0xB4, dat, 8);
    }
    /* Gate Timing */
    {
        uint8_t dat[] = { 0x32, 0x03, 0x1F };
        st7306_write_cmd_multi(d, 0x62, dat, 3);
    }
    /* Source EQ Enable */
    {
        uint8_t dat[] = { 0x13 };
        st7306_write_cmd_multi(d, 0xB7, dat, 1);
    }
    /* Gate Line Setting (400 lines) */
    {
        uint8_t dat[] = { 0x64 };
        st7306_write_cmd_multi(d, 0xB0, dat, 1);
    }

    st7306_write_cmd(d, ST7306_SLPOUT);
    d->spi.delay_ms(120, d->spi.user_data);

    /* Source Voltage Select */
    {
        uint8_t dat[] = { 0x00 };
        st7306_write_cmd_multi(d, 0xC9, dat, 1);
    }
    /* MADCTL: MX=1, DO=1 (portrait orientation) */
    {
        uint8_t dat[] = { 0x48 };
        st7306_write_cmd_multi(d, ST7306_MADCTL, dat, 1);
    }
    /* COLMOD: 3-write for 24bit (2bpp × 12 pixels per address) */
    {
        uint8_t dat[] = { 0x11 };
        st7306_write_cmd_multi(d, ST7306_COLMOD, dat, 1);
    }
    /* Gamma Mode: Mono */
    {
        uint8_t dat[] = { 0x20 };
        st7306_write_cmd_multi(d, 0xB9, dat, 1);
    }
    /* Panel Setting (1-Dot inversion, One Line Interlace) */
    {
        uint8_t dat[] = { 0x29 };
        st7306_write_cmd_multi(d, 0xB8, dat, 1);
    }
    /* CASET: 5-54 (50 addresses × 6 pixels = 300) */
    {
        uint8_t dat[] = { 0x05, 0x36 };
        st7306_write_cmd_multi(d, ST7306_CASET, dat, 2);
    }
    /* RASET: 0-199 (200 addresses × 2 rows = 400) */
    {
        uint8_t dat[] = { 0x00, 0xC7 };
        st7306_write_cmd_multi(d, ST7306_RASET, dat, 2);
    }
    /* TE off */
    {
        uint8_t dat[] = { 0x00 };
        st7306_write_cmd_multi(d, 0x35, dat, 1);
    }
    /* Auto power down ON */
    {
        uint8_t dat[] = { 0xFF };
        st7306_write_cmd_multi(d, 0xD0, dat, 1);
    }
    /* HPM: High Power Mode ON */
    st7306_write_cmd(d, 0x38);

    /* Display ON */
    st7306_write_cmd(d, ST7306_DISPON);
    st7306_write_cmd(d, ST7306_INVOFF);

    /* Clear internal RAM */
    {
        uint8_t dat[] = { 0x4F };
        st7306_write_cmd_multi(d, 0xBB, dat, 1);
    }

    /* Clear host framebuffer */
    memset(d->framebuffer, 0, ST7306_BUF_SIZE);

    return 0;
}

/* --- Deinit --- */
static int st7306_deinit(void *ud)
{
    st7306_t *d = (st7306_t *)ud;
    st7306_write_cmd(d, ST7306_DISPOFF);
    st7306_write_cmd(d, ST7306_SLPIN);
    return 0;
}

/* --- Draw pixel --- */
static void st7306_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud)
{
    st7306_t *d = (st7306_t *)ud;

    if (x < 0 || x >= (int16_t)d->width || y < 0 || y >= (int16_t)d->height)
        return;

    /* Logical → buffer coords (2×2 pixel blocks) */
    int bx = x / 2;
    int by = y / 2;
    int idx = by * ST7306_BUF_W + bx;

    /* Sub-pixel position in 2×2 block */
    int sx = x % 2;  /* 0=left, 1=right */
    int sy = y % 2;  /* 0=top, 1=bottom */

    int bit1_pos = 7 - (sx * 4 + sy);
    int bit0_pos = 7 - (sx * 4 + 2 + sy);

    uint8_t byte_val = d->framebuffer[idx];

    /* Clear old bits */
    byte_val &= ~((1u << bit1_pos) | (1u << bit0_pos));

    /* Set new 2-bit gray value (0-3) */
    uint8_t gray = (uint8_t)color & 3;
    byte_val |= ((gray & 2u) << (bit1_pos - 1)) | ((gray & 1u) << bit0_pos);

    d->framebuffer[idx] = byte_val;
}

/* --- Write buffer (canvas → native → SPI flush) --- */
static void st7306_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud)
{
    st7306_t *d = (st7306_t *)ud;

    if (!buf || rect->w == 0 || rect->h == 0)
        return;

    /* Convert canvas pixels into native framebuffer */
    for (uint16_t row = 0; row < rect->h; row++) {
        for (uint16_t col = 0; col < rect->w; col++) {
            int16_t lx = rect->x + col;
            int16_t ly = rect->y + row;

            if (lx < 0 || lx >= (int16_t)d->width ||
                ly < 0 || ly >= (int16_t)d->height)
                continue;

            /* Canvas 2bpp: 4 pixels per byte, packed as: [p0|p1|p2|p3] */
            uint32_t byte_idx = (uint32_t)row * rect->w + col;
            uint32_t pix_idx = byte_idx >> 2;        /* byte index in buf */
            uint32_t shift = (byte_idx & 3u) << 1;   /* 0, 2, 4, or 6 */

            uint8_t gray = (buf[pix_idx] >> (6 - shift)) & 3;

            /* Write into ST7306 native framebuffer */
            int bx = lx / 2;
            int by = ly / 2;
            int idx = by * ST7306_BUF_W + bx;

            int sx = lx % 2;
            int sy = ly % 2;
            int bit1_pos = 7 - (sx * 4 + sy);
            int bit0_pos = 7 - (sx * 4 + 2 + sy);

            uint8_t byte_val = d->framebuffer[idx];
            byte_val &= ~((1u << bit1_pos) | (1u << bit0_pos));
            byte_val |= ((gray & 2u) << (bit1_pos - 1)) | ((gray & 1u) << bit0_pos);
            d->framebuffer[idx] = byte_val;
        }
    }

    /* Translate logical rect to ST7306 address window */
    int gram_col_start = rect->x / 6;
    int gram_col_end   = (rect->x + rect->w - 1) / 6;
    int gram_row_start = rect->y / 2;
    int gram_row_end   = (rect->y + rect->h - 1) / 2;

    uint8_t caset[2] = { (uint8_t)(5 + gram_col_start), (uint8_t)(5 + gram_col_end) };
    uint8_t raset[2] = { (uint8_t)gram_row_start, (uint8_t)gram_row_end };

    st7306_write_cmd(d, ST7306_CASET);
    st7306_write_data(d, caset, 2);
    st7306_write_cmd(d, ST7306_RASET);
    st7306_write_data(d, raset, 2);
    st7306_write_cmd(d, ST7306_RAMWR);

    int gram_width = gram_col_end - gram_col_start + 1;
    int gram_height = gram_row_end - gram_row_start + 1;
    int bytes_per_row = gram_width * 3;

    /* Send data row by row */
    for (int row = 0; row < gram_height; row++) {
        int row_off = (gram_row_start + row) * ST7306_BUF_W + gram_col_start * 3;
        st7306_write_data(d, &d->framebuffer[row_off], (uint32_t)bytes_per_row);
    }
}

/* --- Contrast (no-op, reflective display) --- */
static void st7306_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }

/* --- Power --- */
static void st7306_set_power(bool on, void *ud)
{
    st7306_t *d = (st7306_t *)ud;
    st7306_write_cmd(d, on ? ST7306_DISPON : ST7306_DISPOFF);
}

/* --- Invert --- */
static void st7306_set_invert(bool invert, void *ud)
{
    st7306_t *d = (st7306_t *)ud;
    st7306_write_cmd(d, invert ? ST7306_INVON : ST7306_INVOFF);
}

/* --- Hardware fill (no-op, delegated to canvas) --- */
static void st7306_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud)
{
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

/* --- Create --- */
eui_display_drv_t* eui_drv_st7306_create(const eui_drv_st7306_config_t *cfg)
{
    if (!cfg) return NULL;

    st7306_t *d = eui_malloc(sizeof(st7306_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));

    d->spi = cfg->spi;
    d->width = cfg->width;
    d->height = cfg->height;

    d->framebuffer = eui_malloc(ST7306_BUF_SIZE);
    if (!d->framebuffer) {
        eui_free(d);
        return NULL;
    }
    memset(d->framebuffer, 0, ST7306_BUF_SIZE);

    d->base.caps.width       = cfg->width;
    d->base.caps.height      = cfg->height;
    d->base.caps.color_depth = 2;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram    = true;
    d->base.caps.hw_scroll   = false;

    d->base.init         = st7306_init;
    d->base.deinit       = st7306_deinit;
    d->base.draw_pixel   = st7306_draw_pixel;
    d->base.write_buffer = st7306_write_buffer;
    d->base.set_contrast = st7306_set_contrast;
    d->base.set_power    = st7306_set_power;
    d->base.set_invert   = st7306_set_invert;
    d->base.fill_rect    = st7306_fill_rect;
    d->base.user_data    = d;

    return &d->base;
}

/* --- Destroy --- */
void eui_drv_st7306_destroy(eui_display_drv_t *hal)
{
    if (!hal) return;
    st7306_t *d = (st7306_t *)hal->user_data;
    if (d) {
        eui_free(d->framebuffer);
        eui_free(d);
    }
}
