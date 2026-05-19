#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"
#include "eui/hal/eui_hal_transport.h"
#include "eui/eui_allocator.h"
#include "eui/eui.h"
#include "eui/driver/eui_drv_ssd1306.h"
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

static void test_ssd1306_create_and_caps(void) {
    TEST("SSD1306 create sets correct caps");
    eui_drv_ssd1306_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_hal_t *hal = eui_drv_ssd1306_create(&cfg);
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
    eui_display_hal_t *hal = eui_drv_ssd1306_create(&cfg);
    hal->init(hal->user_data);
    if (test_ssd1306_write_cmd_count < 10) FAIL("too few init commands sent");
    eui_drv_ssd1306_destroy(hal);
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

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    eui_deinit();
    return tests_passed == tests_run ? 0 : 1;
}
