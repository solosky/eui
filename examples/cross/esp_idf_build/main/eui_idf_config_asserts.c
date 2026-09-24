/* 交叉编译期的配置来源门禁。
 *
 * port/esp-idf/include/freertos/FreeRTOSConfig.h 是给宿主 CMake 构建用的假
 * 配置头（写死 configTICK_RATE_HZ 100）。它一旦出现在组件导出的 include
 * 路径上，就会遮蔽 IDF 的真配置：本工程 sdkconfig.defaults 把 tick 钉到
 * 1000，两者不再相等，于是下面这个断言成为可判别的证据。
 *
 * 用限定形式 "freertos/FreeRTOSConfig.h" 是本用例的关键——只有它会命中
 * 导出路径上的假头；无限定的 "FreeRTOSConfig.h" 走不到那里。 */
#include "sdkconfig.h"
#include "freertos/FreeRTOSConfig.h"

#include "eui/eui_config.h"

_Static_assert(configTICK_RATE_HZ == CONFIG_FREERTOS_HZ,
               "FreeRTOSConfig.h 被 port 目录里的假配置头遮蔽了（tick 与 sdkconfig 不符）");

/* eui 的容量项必须来自 Kconfig（映射头），不是旧硬编码 */
_Static_assert(EUI_POST_QUEUE_SIZE >= 1, "eui_config.h 未生效");
_Static_assert(EUI_POST_DRAIN_MAX >= 1, "eui_config.h 未生效");

/* 供链接期使用，避免空对象文件被优化掉的告警 */
int eui_idf_config_asserts_anchor(void);
int eui_idf_config_asserts_anchor(void) { return (int)configTICK_RATE_HZ; }
