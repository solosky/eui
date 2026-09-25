/* eui 在 ESP32-S3 + ST7789 240x240 上的最小可跑工程。
 *
 * 这份 main.c 就是 brick 契约的可执行说明书：填板级描述符 → board_init →
 * 建 UI → eui_tick + delay_frame。换板子只改顶部宏。 */
#include "eui/eui.h"
#include "eui/eui_input_mux.h"
#include "eui_port_esp_idf.h"
#include "esp_log.h"
#include <string.h>

/* ---- 板级参数（换板子只改这里；当前值 = VAMeter 面板） ---- */
#define PIN_MOSI 5
#define PIN_SCLK 4
#define PIN_CS   3
#define PIN_DC   2
#define PIN_RST  6
#define PIN_BL   13      /* 背光由 board_init 起 LEDC PWM（pin_bl/bl_init_level） */
#define ENC_A    18
#define ENC_B    17
#define BTN_ENC  21      /* 编码器按键 */
#define BTN_SIDE 0       /* 侧键 */

/* 16bpp 240x240 画布 = 115200 字节；池要装下它 + 视图/场景/动画。
 * EUI_MEM_POOL_SIZE 来自 sdkconfig.defaults（本工程 196608）。 */
static uint8_t s_pool[EUI_MEM_POOL_SIZE];

_Static_assert(EUI_COLOR_DEPTH == 16, "本示例要求 16bpp；检查 sdkconfig.defaults");
_Static_assert(EUI_MEM_POOL_SIZE >= 240 * 240 * 2, "池装不下 240x240@16bpp 的一帧");

/* ---- 一个最简视图：证明渲染与输入链路通了 ---- */

static bool root_handler(eui_view_event_t *event, void *context)
{
    if (event->type != EUI_VIEW_EVT_DRAW) return false;
    eui_canvas_t *c = event->event.draw.canvas;

    eui_canvas_set_bg_color(c, EUI_COLOR_BLACK);
    eui_canvas_clear(c);
    eui_canvas_set_color(c, EUI_COLOR_WHITE);
    eui_canvas_set_font(c, NULL);   /* NULL = 内置 8x8 */
    eui_canvas_draw_str(c, 8, 16, "eui on ESP32-S3");
    eui_canvas_draw_str(c, 8, 32, "ST7789 240x240 16bpp");
    eui_canvas_draw_str(c, 8, 56, "rotate encoder / press keys");
    (void)context;
    return true;
}

static eui_view_t s_root;

void app_main(void)
{
    eui_port_esp_idf_board_t board;
    memset(&board, 0, sizeof(board));

    board.panel.kind = EUI_PORT_DISP_ST7789;
    board.panel.st7789.spi_host = 2;            /* SPI2_HOST */
    board.panel.st7789.pin_mosi = PIN_MOSI;
    board.panel.st7789.pin_sclk = PIN_SCLK;
    board.panel.st7789.pin_cs   = PIN_CS;
    board.panel.st7789.pin_dc   = PIN_DC;
    board.panel.st7789.pin_rst  = PIN_RST;
    board.panel.st7789.pin_bl   = PIN_BL;
    board.panel.st7789.bl_freq_hz    = 500;   /* 0 = 默认 500Hz（老固件背光同款） */
    board.panel.st7789.bl_init_level = 255;   /* 演示全程全亮；运行时可改
                                                 eui_port_esp_idf_backlight_set() */
    board.panel.st7789.width    = 240;
    board.panel.st7789.height   = 240;
    board.panel.st7789.madctl   = 0x00;
    board.panel.st7789.invert   = true;
    board.panel.st7789.little_endian = true;    /* ESP32 原生 uint16 布局（RAMCTL bit3）；
                                                 * 真机颜色反了就改 false 重试 */
    board.panel.st7789.freq_hz  = 80000000;     /* 一帧 115200B ≈ 11.5ms */
    board.panel.st7789.hw_cs    = false;        /* 该板 CS 走 GPIO */

    board.input.enc_pin_a = ENC_A;
    board.input.enc_pin_b = ENC_B;
    board.input.btn_count = 2;
    board.input.btn_pin[0] = BTN_ENC;  board.input.btn_key[0] = 0;   /* OK */
    board.input.btn_pin[1] = BTN_SIDE; board.input.btn_key[1] = 1;   /* BACK */
    board.input.active_low = true;              /* 上拉接地按键 */

    board.fps = 30;
    board.mem_pool = s_pool;
    board.mem_pool_size = sizeof(s_pool);

    if (eui_port_esp_idf_board_init(&board) != 0) {
        ESP_LOGE("example", "bringup failed");
        return;
    }

    eui_view_init(&s_root, root_handler, NULL);
    eui_view_dispatcher_t *vd = eui_get_view_dispatcher();
    eui_view_dispatcher_add(vd, 1, &s_root);
    eui_view_dispatcher_switch_to(vd, 1, EUI_ANIM_NONE);

    ESP_LOGI("example", "entering eui loop @ %u fps", (unsigned)board.fps);
    while (eui_is_running()) {
        eui_tick();
        eui_port_esp_idf_delay_frame();
    }
    eui_port_esp_idf_board_deinit();
}
