#include "eui/eui_display_drv.h"
#include "eui/eui_input_drv.h"
#include "eui/hal/eui_hal_types.h"
#include "eui/eui_allocator.h"
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/driver/eui_drv_sh1106.h"
#include "eui/driver/eui_drv_st7735.h"
#include "eui/driver/eui_drv_ili9341.h"
#include "eui/driver/eui_drv_buttons.h"
#include "eui/driver/eui_drv_encoder.h"
#include "eui/driver/eui_drv_xpt2046.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

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
        { .pin_id = 0, .key = EUI_KEY_OK },
        { .pin_id = 1, .key = EUI_KEY_BACK },
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
    if (evt.type != EUI_EVT_KEY_PRESS || evt.data.key != EUI_KEY_OK) FAIL("expected OK press");

    ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event on unchanged state");

    test_btn_pin_state = 0x00;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on release");
    if (evt.type != EUI_EVT_KEY_RELEASE || evt.data.key != EUI_KEY_OK) FAIL("expected OK release");

    eui_drv_buttons_destroy(hal);
    PASS();
}

static void test_buttons_press_back(void) {
    TEST("buttons poll detects BACK key");
    const eui_drv_buttons_map_t map[] = {
        { .pin_id = 0, .key = EUI_KEY_UP },
        { .pin_id = 1, .key = EUI_KEY_BACK },
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
    if (evt.data.key != EUI_KEY_BACK) FAIL("expected BACK key");

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

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    eui_deinit();
    return tests_passed == tests_run ? 0 : 1;
}
