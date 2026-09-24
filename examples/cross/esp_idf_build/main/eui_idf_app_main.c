/* 交叉示例的 app_main：走库侧 bringup（eui_port_esp_idf_board_init）。
 * 过期的 port 目录 bootstrap 已删除——它传 .input = NULL、丢弃 eui_init
 * 返回码、硬编码 vTaskDelay(16)，三者都与当前 core 不兼容。
 *
 * 面板/引脚由 profile 的编译定义给出（main/CMakeLists.txt），这里只留
 * #ifndef 兜底默认值（与原 bootstrap 的默认一致：SSD1306 128x64 I2C；
 * EUI_DRV_ST7306 时走 SPI 面板）。编码器/按键默认 -1 = 不接。 */
#include "eui/eui.h"
#include "eui/eui_port_bootstrap.h"     /* eui_example_setup 原型 */
#include "eui_port_esp_idf.h"
#include "esp_log.h"
#include <string.h>

#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH
#define CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH  128
#endif
#ifndef CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT
#define CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT 64
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_PORT
#define CONFIG_EUI_EXAMPLE_I2C_PORT   0
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SDA
#define CONFIG_EUI_EXAMPLE_I2C_SDA    21
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_SCL
#define CONFIG_EUI_EXAMPLE_I2C_SCL    22
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_FREQ
#define CONFIG_EUI_EXAMPLE_I2C_FREQ   400000
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_ADDR
#define CONFIG_EUI_EXAMPLE_I2C_ADDR   0x3C
#endif
#ifndef CONFIG_EUI_EXAMPLE_I2C_TIMEOUT
#define CONFIG_EUI_EXAMPLE_I2C_TIMEOUT 100
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_HOST
#define CONFIG_EUI_EXAMPLE_SPI_HOST   2
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_MOSI
#define CONFIG_EUI_EXAMPLE_SPI_MOSI   23
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_SCLK
#define CONFIG_EUI_EXAMPLE_SPI_SCLK   18
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_CS
#define CONFIG_EUI_EXAMPLE_SPI_CS     5
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_DC
#define CONFIG_EUI_EXAMPLE_SPI_DC     16
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_RST
#define CONFIG_EUI_EXAMPLE_SPI_RST    17
#endif
#ifndef CONFIG_EUI_EXAMPLE_SPI_FREQ
#define CONFIG_EUI_EXAMPLE_SPI_FREQ   20000000
#endif
#ifndef CONFIG_EUI_EXAMPLE_ENC_A
#define CONFIG_EUI_EXAMPLE_ENC_A      (-1)   /* < 0 = 不接编码器 */
#endif
#ifndef CONFIG_EUI_EXAMPLE_ENC_B
#define CONFIG_EUI_EXAMPLE_ENC_B      (-1)
#endif
#ifndef CONFIG_EUI_EXAMPLE_BTN_OK
#define CONFIG_EUI_EXAMPLE_BTN_OK     (-1)   /* < 0 = 不接按键 */
#endif
#ifndef CONFIG_EUI_EXAMPLE_BTN_BACK
#define CONFIG_EUI_EXAMPLE_BTN_BACK   (-1)
#endif
#ifndef CONFIG_EUI_EXAMPLE_FPS
#define CONFIG_EUI_EXAMPLE_FPS        30
#endif

/* 池尺寸走 Kconfig 的 EUI_MEM_POOL_SIZE（16bpp 240x240 需 >= 115200）。
 * 静态 .bss：SPI DMA 只能读内部 RAM，池不能放 PSRAM。 */
static uint8_t s_pool[EUI_MEM_POOL_SIZE];

void app_main(void)
{
    eui_port_esp_idf_board_t board;
    memset(&board, 0, sizeof(board));

#ifdef EUI_DRV_ST7306
    board.panel.kind = EUI_PORT_DISP_ST7789;
    /* 注：ST7306 面板与 ST7789 命令集不同，此处借用 ST7789 路径仅保证
     * profile 可编；真实 ST7306 面板的 board 支持待需要时补。
     * 日常验证请用默认 SSD1306 profile 或 examples/esp-idf/st7789_240x240。 */
    board.panel.st7789.spi_host = CONFIG_EUI_EXAMPLE_SPI_HOST;
    board.panel.st7789.pin_mosi = CONFIG_EUI_EXAMPLE_SPI_MOSI;
    board.panel.st7789.pin_sclk = CONFIG_EUI_EXAMPLE_SPI_SCLK;
    board.panel.st7789.pin_cs   = CONFIG_EUI_EXAMPLE_SPI_CS;
    board.panel.st7789.pin_dc   = CONFIG_EUI_EXAMPLE_SPI_DC;
    board.panel.st7789.pin_rst  = CONFIG_EUI_EXAMPLE_SPI_RST;
    board.panel.st7789.width    = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH;
    board.panel.st7789.height   = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT;
    board.panel.st7789.invert   = false;
    board.panel.st7789.freq_hz  = CONFIG_EUI_EXAMPLE_SPI_FREQ;
#else
    board.panel.kind = EUI_PORT_DISP_SSD1306;
    board.panel.ssd1306.i2c.i2c_port = CONFIG_EUI_EXAMPLE_I2C_PORT;
    board.panel.ssd1306.i2c.pin_sda  = CONFIG_EUI_EXAMPLE_I2C_SDA;
    board.panel.ssd1306.i2c.pin_scl  = CONFIG_EUI_EXAMPLE_I2C_SCL;
    board.panel.ssd1306.i2c.freq     = CONFIG_EUI_EXAMPLE_I2C_FREQ;
    board.panel.ssd1306.i2c.timeout_ms = CONFIG_EUI_EXAMPLE_I2C_TIMEOUT;
    board.panel.ssd1306.width  = CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH;
    board.panel.ssd1306.height = CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT;
    board.panel.ssd1306.addr   = CONFIG_EUI_EXAMPLE_I2C_ADDR;
#endif

    /* 输入：编码器（可选）+ OK/BACK 按键（可选），board_init 内部经 mux 合成 */
    board.input.enc_pin_a = CONFIG_EUI_EXAMPLE_ENC_A;
    board.input.enc_pin_b = CONFIG_EUI_EXAMPLE_ENC_B;
    if (CONFIG_EUI_EXAMPLE_BTN_OK >= 0) {
        board.input.btn_pin[board.input.btn_count] = CONFIG_EUI_EXAMPLE_BTN_OK;
        board.input.btn_key[board.input.btn_count] = 0;   /* KEY_ID_OK */
        board.input.btn_count++;
    }
    if (CONFIG_EUI_EXAMPLE_BTN_BACK >= 0) {
        board.input.btn_pin[board.input.btn_count] = CONFIG_EUI_EXAMPLE_BTN_BACK;
        board.input.btn_key[board.input.btn_count] = 1;   /* KEY_ID_BACK */
        board.input.btn_count++;
    }
    board.input.active_low = true;

    board.fps = CONFIG_EUI_EXAMPLE_FPS;
    board.mem_pool = s_pool;
    board.mem_pool_size = sizeof(s_pool);

    if (eui_port_esp_idf_board_init(&board) != 0) {
        ESP_LOGE("example", "board init failed");
        return;
    }

    eui_example_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.display_width = (uint16_t)CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH;
    cfg.display_height = (uint16_t)CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT;
    eui_example_setup(&cfg);

    while (eui_is_running()) {
        eui_tick();
        eui_port_esp_idf_delay_frame();
    }
}
