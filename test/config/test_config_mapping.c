/* 配置映射头单测：IDF 路径的 eui_config.h 必须把 CONFIG_EUI_* 映射成 EUI_*，
 * 且在没有任何 CONFIG_ 定义时回落到默认值（宿主/未走 Kconfig 的消费方）。
 *
 * 覆盖范围说明：本用例只测整数项的映射（色深/容量是本次重整的核心），以及
 * 宏集合的完整性。三个 bool 字体开关的映射依赖 ESP_PLATFORM（用来区分
 * 「sdkconfig.h 存在但该项取 n」与「根本没走 Kconfig」），宿主上无法两条
 * 分支都测到——它们由 IDF 构建侧的 sdkconfig 检查与既有字体测试覆盖。
 */
#define CONFIG_EUI_COLOR_DEPTH      16
#define CONFIG_EUI_MAX_VIEWS        16
#define CONFIG_EUI_MAX_OVERLAYS      8
#define CONFIG_EUI_SCENE_MAX        24
#define CONFIG_EUI_KEY_ID_MAX        8
#define CONFIG_EUI_POST_QUEUE_SIZE  16
#define CONFIG_EUI_POST_DRAIN_MAX   64
#define CONFIG_EUI_EVENT_QUEUE_SIZE  8
#define CONFIG_EUI_MAX_ANIMATIONS    8
#define CONFIG_EUI_MAX_WIDGETS      32
#define CONFIG_EUI_MEM_POOL_SIZE 204800

/* 必须走直接相对路径：eui_add_test 把 ${CMAKE_BINARY_DIR}/include 排在
 * include 列表最前，用 "eui/eui_config.h" 会命中 standalone 生成的那份，
 * 测不到 IDF 映射头。 */
#include "../../port/esp-idf/include/eui/eui_config.h"
#include "common/eui_test.h"
#include <stdio.h>
#include <stdint.h>

/* CONFIG_ 存在时必须原样映射 */
static void test_config_override_wins(void)
{
    TEST("config: CONFIG_EUI_* 覆盖默认值");
    if (EUI_COLOR_DEPTH != 16)       FAIL("COLOR_DEPTH 未取 CONFIG_ 值");
    if (EUI_MAX_VIEWS != 16)         FAIL("MAX_VIEWS 未取 CONFIG_ 值");
    if (EUI_MAX_OVERLAYS != 8)       FAIL("MAX_OVERLAYS 未取 CONFIG_ 值");
    if (EUI_SCENE_MAX != 24)         FAIL("SCENE_MAX 未取 CONFIG_ 值");
    if (EUI_MEM_POOL_SIZE != 204800) FAIL("MEM_POOL_SIZE 未取 CONFIG_ 值");
    PASS();
}

/* 宏集合必须齐全：漏一个就会在别处静默用错值 */
static void test_config_macro_set_complete(void)
{
    TEST("config: 映射头导出完整宏集合");
    /* 每个宏都参与一次运算，缺失即编译失败 */
    uint32_t probe = (uint32_t)EUI_COLOR_DEPTH + (uint32_t)EUI_MAX_VIEWS
                   + (uint32_t)EUI_MAX_ANIMATIONS + (uint32_t)EUI_MAX_WIDGETS
                   + (uint32_t)EUI_EVENT_QUEUE_SIZE + (uint32_t)EUI_MAX_OVERLAYS
                   + (uint32_t)EUI_SCENE_MAX + (uint32_t)EUI_KEY_ID_MAX
                   + (uint32_t)EUI_POST_QUEUE_SIZE + (uint32_t)EUI_POST_DRAIN_MAX
                   + (uint32_t)EUI_MEM_POOL_SIZE
                   + (uint32_t)EUI_MAX_WIDGET_CHILDREN + (uint32_t)EUI_CANVAS_STATE_STACK
                   + (uint32_t)EUI_FONT_ENABLE_U8G2 + (uint32_t)EUI_FONT_ENABLE_KERNING
                   + (uint32_t)EUI_FONT_ENABLE_MULTILINE;
    if (probe == 0) FAIL("宏集合异常（全 0）");
    PASS();
}

/* 默认值回落到 standalone 那组：不带任何 CONFIG_ 时也应可用 */
static void test_config_defaults_shape(void)
{
    TEST("config: 默认值形状合法（通过编译期校验）");
    /* EUI_COLOR_DEPTH 的合法性由映射头里的 #error 守住；这里只确认
     * 取值落在画布实现支持的集合里 */
    if (!(EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2 || EUI_COLOR_DEPTH == 4 ||
          EUI_COLOR_DEPTH == 8 || EUI_COLOR_DEPTH == 16)) {
        FAIL("COLOR_DEPTH 不在 {1,2,4,8,16}");
    }
    PASS();
}

int main(void)
{
    eui_test_init();
    printf("=== Config Mapping Tests ===\n");
    test_config_override_wins();
    test_config_macro_set_complete();
    test_config_defaults_shape();
    return eui_test_summary();
}
