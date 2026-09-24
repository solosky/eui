#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"
#include "eui/hal/eui_hal_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/driver/eui_drv_sh1106.h"
#include "eui/driver/eui_drv_st7735.h"
#include "eui/driver/eui_drv_st7789.h"
#include "eui/driver/eui_drv_ili9341.h"
#include "eui/driver/eui_drv_buttons.h"
#include "eui/driver/eui_drv_encoder.h"
#include "eui/driver/eui_drv_xpt2046.h"
#include <stdio.h>
#include <string.h>
#include "common/eui_test.h"

static int test_ssd1306_write_cmd_count;
static int test_ssd1306_write_data_count;
static int test_ssd1306_delay_count;

static void mock_i2c_write_cmd(uint8_t cmd, void *ud) {
    (void)cmd; (void)ud;
    test_ssd1306_write_cmd_count++;
}

static void mock_i2c_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)ud;
    test_ssd1306_write_data_count += (int)len;
}

static void mock_i2c_delay_ms(uint32_t ms, void *ud) {
    (void)ms; (void)ud;
    test_ssd1306_delay_count++;
}

static int test_spi_cmd_count;
static int test_spi_data_count;

static void mock_spi_write_cmd(uint8_t cmd, void *ud) {
    (void)cmd; (void)ud;
    test_spi_cmd_count++;
}
static void mock_spi_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)ud;
    test_spi_data_count += (int)len;
}
static void mock_spi_read_data(uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)len; (void)ud;
}
static void mock_spi_set_dc(bool dm, void *ud) { (void)dm; (void)ud; }
static void mock_spi_set_cs(bool a, void *ud) { (void)a; (void)ud; }
static void mock_spi_set_rst(bool a, void *ud) { (void)a; (void)ud; }
static void mock_spi_delay_ms(uint32_t ms, void *ud) { (void)ms; (void)ud; }

static void test_ssd1306_create_and_caps(void) {
    TEST("SSD1306 create sets correct caps");
    eui_drv_ssd1306_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_drv_t *hal = eui_drv_ssd1306_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 64) FAIL("height mismatch");
    if (hal->caps.color_depth != 1) FAIL("color depth mismatch");
    if (hal->caps.buffer_mode != EUI_BUFFER_PAGE) FAIL("buffer mode mismatch");
    eui_drv_ssd1306_destroy(hal);
    PASS();
}

static void test_ssd1306_init_sends_commands(void) {
    TEST("SSD1306 init sends command sequence");
    test_ssd1306_write_cmd_count = 0;
    eui_drv_ssd1306_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_drv_t *hal = eui_drv_ssd1306_create(&cfg);
    hal->init(hal->user_data);
    if (test_ssd1306_write_cmd_count < 10) FAIL("too few init commands sent");
    eui_drv_ssd1306_destroy(hal);
    PASS();
}

static void test_sh1106_create_and_caps(void) {
    TEST("SH1106 create sets correct caps");
    eui_drv_sh1106_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_drv_t *hal = eui_drv_sh1106_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 64) FAIL("height mismatch");
    if (hal->caps.color_depth != 1) FAIL("color depth mismatch");
    if (hal->caps.buffer_mode != EUI_BUFFER_PAGE) FAIL("buffer mode mismatch");
    eui_drv_sh1106_destroy(hal);
    PASS();
}

static void test_st7735_create_and_caps(void) {
    TEST("ST7735 create sets correct caps");
    eui_drv_st7735_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 128, .height = 160, .variant = 0,
    };
    eui_display_drv_t *hal = eui_drv_st7735_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 160) FAIL("height mismatch");
    if (hal->caps.color_depth != 16) FAIL("color depth mismatch");
    eui_drv_st7735_destroy(hal);
    PASS();
}

/* ST7789：捕获命令字节，验证初始化序列与 invert 语义 */
static uint8_t test_st7789_cmds[32];
static int test_st7789_cmd_count;
static uint8_t test_st7789_cur_cmd;   /* mock_write_data 按它归档载荷 */
static void mock_st7789_write_cmd(uint8_t cmd, void *ud) {
    (void)ud;
    test_st7789_cur_cmd = cmd;
    if (test_st7789_cmd_count < (int)sizeof(test_st7789_cmds))
        test_st7789_cmds[test_st7789_cmd_count++] = cmd;
}

/* RAMCTL 需要看数据字节：init 末尾的 CASET/RASET 各发 4 字节会覆盖
 * "最后一次 write_data"，所以按当前命令归档（只留 RAMCTL 的载荷） */
static uint8_t test_st7789_ramctl_data[2];
static int test_st7789_ramctl_len;
static void mock_st7789_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    (void)ud;
    if (test_st7789_cur_cmd == 0xB0) {
        test_st7789_ramctl_len =
            (int)(len < sizeof(test_st7789_ramctl_data) ? len : sizeof(test_st7789_ramctl_data));
        memcpy(test_st7789_ramctl_data, buf, (size_t)test_st7789_ramctl_len);
    }
}

static void test_st7789_create_and_init_sequence(void) {
    TEST("ST7789 create + init sequence (invert=true, RAMCTL) and set_invert toggle");
    test_st7789_cmd_count = 0;
    eui_drv_st7789_config_t cfg = {
        .spi = { .write_cmd = mock_st7789_write_cmd, .write_data = mock_st7789_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 240, .height = 240, .col_offset = 0, .row_offset = 0,
        .madctl = 0x00, .invert = true, .little_endian = false,
    };
    eui_display_drv_t *hal = eui_drv_st7789_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 240) FAIL("width mismatch");
    if (hal->caps.height != 240) FAIL("height mismatch");
    if (hal->caps.color_depth != 16) FAIL("color depth mismatch");
    if (hal->caps.buffer_mode != EUI_BUFFER_FULL) FAIL("buffer mode mismatch");

    if (hal->init(hal->user_data) != 0) FAIL("init returned error");
    /* SWRESET SLPOUT COLMOD MADCTL RAMCTL INVON NORON DISPON CASET RASET */
    static const uint8_t want[] = { 0x01, 0x11, 0x3A, 0x36, 0xB0, 0x21, 0x13, 0x29, 0x2A, 0x2B };
    if (test_st7789_cmd_count != (int)(sizeof(want))) FAIL("init command count mismatch");
    for (unsigned i = 0; i < sizeof(want); i++) {
        if (test_st7789_cmds[i] != want[i]) FAIL("init command sequence mismatch");
    }
    /* RAMCTL 载荷：{0x00, 0xF0 | (little_endian << 3)}；init 的最后一次
     * write_data 就是它（RASET 在其后但走 set_addr_window 之外不会触发
     * write_data——init 末尾只有地址窗口设置，为稳妥在 set_invert 前断言） */
    if (test_st7789_ramctl_len != 2) FAIL("RAMCTL payload length != 2");
    if (test_st7789_ramctl_data[0] != 0x00) FAIL("RAMCTL val1 != 0x00");
    if (test_st7789_ramctl_data[1] != 0xF0) FAIL("RAMCTL big-endian val2 != 0xF0");

    hal->set_invert(false, hal->user_data);
    if (test_st7789_cmds[test_st7789_cmd_count - 1] != 0x20) FAIL("set_invert(false) != INVOFF");
    hal->set_invert(true, hal->user_data);
    if (test_st7789_cmds[test_st7789_cmd_count - 1] != 0x21) FAIL("set_invert(true) != INVON");

    eui_drv_st7789_destroy(hal);
    eui_drv_st7789_destroy(NULL);   /* NULL 安全 */
    PASS();
}

static void test_st7789_little_endian(void) {
    TEST("ST7789 little_endian=true 写 RAMCTL 0xF8");
    test_st7789_cmd_count = 0;
    eui_drv_st7789_config_t cfg = {
        .spi = { .write_cmd = mock_st7789_write_cmd, .write_data = mock_st7789_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 240, .height = 240, .col_offset = 0, .row_offset = 0,
        .madctl = 0x00, .invert = true, .little_endian = true,
    };
    eui_display_drv_t *hal = eui_drv_st7789_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->init(hal->user_data) != 0) FAIL("init returned error");
    if (test_st7789_ramctl_data[1] != 0xF8) FAIL("RAMCTL little-endian val2 != 0xF8 (0xF0|1<<3)");
    eui_drv_st7789_destroy(hal);
    PASS();
}

static void test_ili9341_create_and_caps(void) {
    TEST("ILI9341 create sets correct caps");
    eui_drv_ili9341_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 240, .height = 320,
    };
    eui_display_drv_t *hal = eui_drv_ili9341_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 240) FAIL("width mismatch");
    if (hal->caps.height != 320) FAIL("height mismatch");
    if (hal->caps.color_depth != 16) FAIL("color depth mismatch");
    eui_drv_ili9341_destroy(hal);
    PASS();
}

static uint8_t test_btn_pin_state;
static bool mock_btn_read_pin(uint8_t pin_id, void *ud) {
    (void)ud;
    return (test_btn_pin_state & (1u << pin_id)) != 0;
}
static void mock_btn_delay_us(uint32_t us, void *ud) { (void)us; (void)ud; }

static void test_buttons_press_release(void) {
    TEST("buttons poll detects press and release");
    const eui_drv_buttons_map_t map[] = {
        { .pin_id = 0, .key = 4 },   /* OK */
        { .pin_id = 1, .key = 5 },   /* BACK */
    };
    eui_drv_buttons_config_t cfg = {
        .gpio = { .read_pin = mock_btn_read_pin, .delay_us = mock_btn_delay_us, .user_data = NULL },
        .map = map, .count = 2,
    };
    eui_input_drv_t *hal = eui_drv_buttons_create(&cfg);
    if (!hal) FAIL("create returned NULL");

    hal->init(hal->user_data);

    test_btn_pin_state = 0x01;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on press");
    if (evt.type != EUI_EVT_KEY_PRESS || evt.data.key_id != 4) FAIL("expected OK press");

    ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event on unchanged state");

    test_btn_pin_state = 0x00;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on release");
    if (evt.type != EUI_EVT_KEY_RELEASE || evt.data.key_id != 4) FAIL("expected OK release");

    eui_drv_buttons_destroy(hal);
    PASS();
}

static void test_buttons_press_back(void) {
    TEST("buttons poll detects BACK key");
    const eui_drv_buttons_map_t map[] = {
        { .pin_id = 0, .key = 0 },   /* UP */
        { .pin_id = 1, .key = 5 },   /* BACK */
    };
    eui_drv_buttons_config_t cfg = {
        .gpio = { .read_pin = mock_btn_read_pin, .delay_us = mock_btn_delay_us, .user_data = NULL },
        .map = map, .count = 2,
    };
    eui_input_drv_t *hal = eui_drv_buttons_create(&cfg);
    hal->init(hal->user_data);

    test_btn_pin_state = 0x02;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event");
    if (evt.data.key_id != 5) FAIL("expected BACK key");

    eui_drv_buttons_destroy(hal);
    PASS();
}

static uint8_t test_enc_pins;
static bool mock_enc_read_pin(uint8_t pin_id, void *ud) {
    (void)ud;
    return (test_enc_pins & (1u << pin_id)) != 0;
}
static void mock_enc_delay_us(uint32_t us, void *ud) { (void)us; (void)ud; }

static void test_encoder_cw(void) {
    TEST("encoder detects CW rotation");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_drv_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    test_enc_pins = 0x00; hal->poll(NULL, hal->user_data);
    test_enc_pins = 0x02;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CW) FAIL("expected CW");

    eui_drv_encoder_destroy(hal);
    PASS();
}

static void test_encoder_ccw(void) {
    TEST("encoder detects CCW rotation");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_drv_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    test_enc_pins = 0x00; hal->poll(NULL, hal->user_data);
    test_enc_pins = 0x01;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CCW) FAIL("expected CCW");

    eui_drv_encoder_destroy(hal);
    PASS();
}

static void test_encoder_click(void) {
    TEST("encoder detects click");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_drv_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    test_enc_pins = 0x04;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CLICK) FAIL("expected CLICK");

    test_enc_pins = 0x00;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event on release");

    eui_drv_encoder_destroy(hal);
    PASS();
}

static bool test_touch_irq_state;
static bool mock_xpt_irq(void *ud) { (void)ud; return test_touch_irq_state; }

static void test_xpt2046_touch_down_up(void) {
    TEST("XPT2046 detects touch down and up");
    eui_drv_xpt2046_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .irq = { .read_irq = mock_xpt_irq, .user_data = NULL },
        .width = 320, .height = 240,
    };
    eui_input_drv_t *hal = eui_drv_xpt2046_create(&cfg);
    hal->init(hal->user_data);

    /* no touch: IRQ high */
    test_touch_irq_state = true;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event when not touched");

    /* touch down: IRQ low */
    test_touch_irq_state = false;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on touch down");
    if (evt.type != EUI_EVT_TOUCH_DOWN) FAIL("expected TOUCH_DOWN");

    /* touch up: IRQ high */
    test_touch_irq_state = true;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on touch up");
    if (evt.type != EUI_EVT_TOUCH_UP) FAIL("expected TOUCH_UP");

    eui_drv_xpt2046_destroy(hal);
    PASS();
}

#define DRV_POOL_SIZE 32768
static uint8_t drv_pool[DRV_POOL_SIZE];

int main(void) {
    eui_config_t cfg = {
        .mem_pool_buffer = drv_pool, .mem_pool_size = DRV_POOL_SIZE,
        .display = NULL, .input = NULL, .fps_target = 30,
    };
    eui_allocator_init_tlsf(drv_pool, DRV_POOL_SIZE);
    eui_init(&cfg);

    printf("=== Driver Tests ===\n\n");

    printf("--- SSD1306 ---\n");
    test_ssd1306_create_and_caps();
    test_ssd1306_init_sends_commands();

    printf("--- SH1106 ---\n");
    test_sh1106_create_and_caps();

    printf("--- ST7735 ---\n");
    test_st7735_create_and_caps();

    printf("--- ST7789 ---\n");
    test_st7789_create_and_init_sequence();
    test_st7789_little_endian();

    printf("--- ILI9341 ---\n");
    test_ili9341_create_and_caps();

    printf("--- Buttons ---\n");
    test_buttons_press_release();
    test_buttons_press_back();

    printf("--- Encoder ---\n");
    test_encoder_cw();
    test_encoder_ccw();
    test_encoder_click();

    printf("--- XPT2046 ---\n");
    test_xpt2046_touch_down_up();

    eui_deinit();
    return eui_test_summary();
}
