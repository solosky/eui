#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"
#include "eui/hal/eui_hal_transport.h"
#include "eui/eui_allocator.h"
#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

int main(void) {
    uint8_t pool[4096];
    eui_config_t cfg = {
        .mem_pool_buffer = pool, .mem_pool_size = sizeof(pool),
        .display = NULL, .input = NULL, .fps_target = 30,
    };
    eui_init(&cfg);

    printf("=== Driver Tests ===\n\n");

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    eui_deinit();
    return tests_passed == tests_run ? 0 : 1;
}
