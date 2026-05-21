/* 16bpp config override.  Included via -include BEFORE any library source
 * so that EUI_CONFIG_H is defined before eui_config.h is ever read.
 * The generated config.h guard (#ifndef EUI_CONFIG_H) then skips it, and
 * all our values below remain in effect for the entire translation unit. */
#ifndef CONFIG_16BPP_OVERRIDE
#define CONFIG_16BPP_OVERRIDE

#define EUI_CONFIG_H            1       /* blocks generated config.h    */
#define EUI_COLOR_DEPTH        16
#define EUI_CANVAS_STATE_STACK  4
#define EUI_MEM_POOL_SIZE   65536
#define EUI_MAX_VIEWS          8
#define EUI_MAX_ANIMATIONS     8
#define EUI_MAX_WIDGETS       32
#define EUI_EVENT_QUEUE_SIZE   8
#define EUI_MAX_OVERLAYS       4
#define EUI_MAX_WIDGET_CHILDREN 8
#define EUI_FONT_ENABLE_U8G2   1
#define EUI_FONT_ENABLE_KERNING 1
#define EUI_FONT_ENABLE_MULTILINE 1

#endif
