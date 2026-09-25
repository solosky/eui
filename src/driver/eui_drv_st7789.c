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

/* ---- CS 括起契约（真机定位的根因） ----
 *
 * 本面板**要求每个事务之间释放 CS**：CS 必须"拉低 → 发命令(+数据) → 拉高"，
 * 即按"一条命令 + 它的数据"为单位括起（与 LovyanGFX 的 begin/endTransaction 同义）。
 * 早先的实现是在 init 开头 set_cs(false) 后**永久保持拉低**：波形层面 SCLK/MOSI/DC
 * 全部正确（真机用 PCNT 逐线量过），但 4 线串口从未见到 CS 的去选中沿，命令被误解析、
 * 面板始终未被配置、GRAM 未正确写入 —— 表现为背光亮而屏全黑，且所有寄存器级检查都"正确"。
 * 真机 A/B：同一份命令字节、同一传输口，仅 CS 括起者显示（青），CS 常低者全黑（品红）。
 *
 * 因此：set_cs(false)=拉低选中、set_cs(true)=拉高释放；事务边界即括起边界。 */
static void st7789_cs(st7789_t *d, bool active) {
    if (d->spi.set_cs) d->spi.set_cs(active, d->spi.user_data);
}

static void st7789_write_cmd(st7789_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void st7789_write_data(st7789_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

/* 一个完整事务：命令（+可选数据）作为一组，前后括起 CS。 */
static void st7789_cmd(st7789_t *d, uint8_t cmd, const uint8_t *data, uint32_t len) {
    st7789_cs(d, false);
    st7789_write_cmd(d, cmd);
    if (len) st7789_write_data(d, data, len);
    st7789_cs(d, true);
}

static void st7789_set_addr_window(st7789_t *d, uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h) {
    uint16_t xs = x + d->col_offset, xe = xs + w - 1;
    uint16_t ys = y + d->row_offset, ye = ys + h - 1;
    uint8_t caset[4] = { (uint8_t)(xs >> 8), (uint8_t)(xs), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t raset[4] = { (uint8_t)(ys >> 8), (uint8_t)(ys), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7789_cmd(d, ST7789_CASET, caset, 4);
    st7789_cmd(d, ST7789_RASET, raset, 4);
}

static int st7789_init(void *ud) {
    st7789_t *d = (st7789_t*)ud;

    st7789_cs(d, true);   /* 复位期间 CS 释放（未选中）*/
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    st7789_cmd(d, ST7789_SWRESET, NULL, 0);
    d->spi.delay_ms(150, d->spi.user_data);
    st7789_cmd(d, ST7789_SLPOUT, NULL, 0);
    d->spi.delay_ms(120, d->spi.user_data);

    /* 电源/时序/伽马命令组：与 LovyanGFX Panel_ST7789 的 list0 逐条对齐
     * （该序列在 VAMeter 面板实测可显示）。此前只发精简序列 + RAMCTL
     * {0x00,0xF8}，真机黑屏；LGFX 用 RAMCTRL {0x00,0xC0}，故弃用
     * esp_lcd 风格的 GBPF/endian 位拼装。 */
    { uint8_t v[5] = { 0x0c, 0x0c, 0x00, 0x33, 0x33 }; st7789_cmd(d, 0xB2, v, 5); }              /* PORCTRL */
    { uint8_t v = 0x35; st7789_cmd(d, 0xB7, &v, 1); }              /* GCTRL */
    { uint8_t v = 0x28; st7789_cmd(d, 0xBB, &v, 1); }              /* VCOMS */
    { uint8_t v = 0x0C; st7789_cmd(d, 0xC0, &v, 1); }              /* LCMCTRL */
    { uint8_t v[2] = { 0x01, 0xFF }; st7789_cmd(d, 0xC2, v, 2); }              /* VDVVRHEN */
    { uint8_t v = 0x10; st7789_cmd(d, 0xC3, &v, 1); }              /* VRHS */
    { uint8_t v = 0x20; st7789_cmd(d, 0xC4, &v, 1); }              /* VDVSET */
    { uint8_t v = 0x0f; st7789_cmd(d, 0xC6, &v, 1); }              /* FRCTR2 (60Hz) */
    { uint8_t v[2] = { 0xa4, 0xa1 }; st7789_cmd(d, 0xD0, v, 2); }              /* PWCTRL1 */
    { uint8_t v[2] = { 0x00, 0xC0 }; st7789_cmd(d, 0xB0, v, 2); }              /* RAMCTRL（LGFX 同款 {0x00,0xC0}） */
    { uint8_t v[14] = { 0xd0,0x00,0x02,0x07,0x0a,0x28,0x32,0x44,
                        0x42,0x06,0x0e,0x12,0x14,0x17 };
      st7789_cmd(d, 0xE0, v, 14); }          /* PVGAMCTRL */
    { uint8_t v[14] = { 0xd0,0x00,0x02,0x07,0x0a,0x28,0x31,0x54,
                        0x47,0x0e,0x1c,0x17,0x1b,0x1e };
      st7789_cmd(d, 0xE1, v, 14); }          /* NVGAMCTRL */

    { uint8_t v = 0x55; st7789_cmd(d, ST7789_COLMOD, &v, 1); }   /* 16bit/pixel RGB565 */

    { uint8_t v = d->madctl; st7789_cmd(d, ST7789_MADCTL, &v, 1); }

    st7789_cmd(d, d->invert ? ST7789_INVON : ST7789_INVOFF, NULL, 0);

    st7789_cmd(d, ST7789_NORON, NULL, 0);
    d->spi.delay_ms(10, d->spi.user_data);
    st7789_cmd(d, 0x38, NULL, 0);           /* IDMOFF（LGFX list0 同款） */
    st7789_cmd(d, ST7789_DISPON, NULL, 0);
    d->spi.delay_ms(100, d->spi.user_data);

    st7789_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int st7789_deinit(void *ud) { (void)ud; return 0; }

static void st7789_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

/* 画布线序 = 面板线序 = swap565（见 app-eui/ui/ui_draw.c 的约定说明），
 * 因此整帧像素**原样直发**，不做任何字节转换。
 * little_endian=false 时调用方若用了别的线序，需自行保证一致。 */

static void st7789_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    st7789_t *d = (st7789_t*)ud;
    st7789_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y, rect->w, rect->h);
    /* RAMWR 与其后整帧像素必须同属一个 CS 括起事务（与 LGFX 的 pushSprite 一致） */
    uint32_t len = (uint32_t)rect->w * rect->h * 2;
    st7789_cs(d, false);
    st7789_write_cmd(d, ST7789_RAMWR);
    st7789_write_data(d, buf, len);   /* 线序已一致，零转换 */
    st7789_cs(d, true);
}

static void st7789_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void st7789_set_power(bool on, void *ud) { (void)on; (void)ud; }

static void st7789_set_invert(bool invert, void *ud) {
    st7789_t *d = (st7789_t*)ud;
    st7789_cmd(d, invert ? ST7789_INVON : ST7789_INVOFF, NULL, 0);
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
