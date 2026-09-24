/* IDF 构建的 eui 配置：把 Kconfig 的 CONFIG_EUI_* 映射成 EUI_*。
 *
 * 两条构建路径共用同一套宏名（改一处要改两处）：
 *   standalone: CMakeLists.txt 的 EUI_* cache 变量 → configure_file → eui_config.h
 *   IDF:        Kconfig 的 CONFIG_EUI_* → 本文件
 *
 * 逃生口：预先定义 EUI_CONFIG_H 并自带一套 EUI_* 定义即可完全绕过本文件
 * （本文件的守卫会跳过），用于给单个编译单元换一套容量参数。
 */
#ifndef EUI_CONFIG_H
#define EUI_CONFIG_H

/* sdkconfig.h 只在 IDF 构建里存在；它的存在性同时是「Kconfig 已被处理过」
 * 的证据——bool 项取 n 时符号在 sdkconfig.h 里彻底消失，没有这个标记就
 * 无法区分「用户显式关掉了」与「这个工程根本没走 Kconfig」。 */
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#define EUI_PORT_HAVE_SDKCONFIG 1
#else
#define EUI_PORT_HAVE_SDKCONFIG 0
#endif

/* ---- 整数项：CONFIG_ 优先，否则回落 standalone 的默认值 ---- */
#ifndef CONFIG_EUI_COLOR_DEPTH
#define CONFIG_EUI_COLOR_DEPTH 1
#endif
#ifndef CONFIG_EUI_MEM_POOL_SIZE
#define CONFIG_EUI_MEM_POOL_SIZE 8192
#endif
#ifndef CONFIG_EUI_MAX_VIEWS
#define CONFIG_EUI_MAX_VIEWS 8
#endif
#ifndef CONFIG_EUI_MAX_ANIMATIONS
#define CONFIG_EUI_MAX_ANIMATIONS 8
#endif
#ifndef CONFIG_EUI_MAX_WIDGETS
#define CONFIG_EUI_MAX_WIDGETS 32
#endif
#ifndef CONFIG_EUI_EVENT_QUEUE_SIZE
#define CONFIG_EUI_EVENT_QUEUE_SIZE 8
#endif
#ifndef CONFIG_EUI_MAX_OVERLAYS
#define CONFIG_EUI_MAX_OVERLAYS 4
#endif
#ifndef CONFIG_EUI_SCENE_MAX
#define CONFIG_EUI_SCENE_MAX 16
#endif
#ifndef CONFIG_EUI_KEY_ID_MAX
#define CONFIG_EUI_KEY_ID_MAX 8
#endif
#ifndef CONFIG_EUI_POST_QUEUE_SIZE
#define CONFIG_EUI_POST_QUEUE_SIZE 16
#endif
#ifndef CONFIG_EUI_POST_DRAIN_MAX
#define CONFIG_EUI_POST_DRAIN_MAX 64
#endif

#define EUI_COLOR_DEPTH      CONFIG_EUI_COLOR_DEPTH
#define EUI_MEM_POOL_SIZE    CONFIG_EUI_MEM_POOL_SIZE
#define EUI_MAX_VIEWS        CONFIG_EUI_MAX_VIEWS
#define EUI_MAX_ANIMATIONS   CONFIG_EUI_MAX_ANIMATIONS
#define EUI_MAX_WIDGETS      CONFIG_EUI_MAX_WIDGETS
#define EUI_EVENT_QUEUE_SIZE CONFIG_EUI_EVENT_QUEUE_SIZE
#define EUI_MAX_OVERLAYS     CONFIG_EUI_MAX_OVERLAYS
#define EUI_SCENE_MAX        CONFIG_EUI_SCENE_MAX
#define EUI_KEY_ID_MAX       CONFIG_EUI_KEY_ID_MAX
#define EUI_POST_QUEUE_SIZE  CONFIG_EUI_POST_QUEUE_SIZE
#define EUI_POST_DRAIN_MAX   CONFIG_EUI_POST_DRAIN_MAX

/* ---- 字体开关：折成 0/1，与 standalone 模板的 #cmakedefine01 语义一致 ---- */
#if EUI_PORT_HAVE_SDKCONFIG
#  ifdef CONFIG_EUI_FONT_ENABLE_U8G2
#    define EUI_FONT_ENABLE_U8G2 1
#  else
#    define EUI_FONT_ENABLE_U8G2 0
#  endif
#  ifdef CONFIG_EUI_FONT_ENABLE_KERNING
#    define EUI_FONT_ENABLE_KERNING 1
#  else
#    define EUI_FONT_ENABLE_KERNING 0
#  endif
#  ifdef CONFIG_EUI_FONT_ENABLE_MULTILINE
#    define EUI_FONT_ENABLE_MULTILINE 1
#  else
#    define EUI_FONT_ENABLE_MULTILINE 0
#  endif
#else
/* 非 IDF 构建：Kconfig 的默认值都是 y，这里跟随 */
#  define EUI_FONT_ENABLE_U8G2      1
#  define EUI_FONT_ENABLE_KERNING   1
#  define EUI_FONT_ENABLE_MULTILINE 1
#endif

/* ---- 两个不在 Kconfig 里的容量项（与 standalone 模板同为常量） ---- */
#define EUI_MAX_WIDGET_CHILDREN 8
#define EUI_CANVAS_STATE_STACK  4

/* ---- 取值校验：配错在编译期炸，不要运行期静默错 ---- */
#if !(EUI_COLOR_DEPTH == 1 || EUI_COLOR_DEPTH == 2 || EUI_COLOR_DEPTH == 4 || \
      EUI_COLOR_DEPTH == 8 || EUI_COLOR_DEPTH == 16)
#error "EUI_COLOR_DEPTH must be 1, 2, 4, 8 or 16"
#endif
#if EUI_KEY_ID_MAX < 1 || EUI_KEY_ID_MAX > 8
#error "EUI_KEY_ID_MAX must be 1..8 (key_id is uint8_t; gesture slots are bounded)"
#endif
#if EUI_POST_QUEUE_SIZE < 1 || EUI_POST_QUEUE_SIZE > 255
#error "EUI_POST_QUEUE_SIZE must be 1..255"
#endif
#if EUI_POST_DRAIN_MAX < 1
#error "EUI_POST_DRAIN_MAX must be >= 1"
#endif

#endif /* EUI_CONFIG_H */
