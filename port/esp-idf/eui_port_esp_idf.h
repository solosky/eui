#ifndef EUI_PORT_ESP_IDF_H
#define EUI_PORT_ESP_IDF_H

#include <stdint.h>
#include <stdbool.h>

#include "eui/hal/eui_hal_types.h"

#include "hal/i2c_types.h"
#include "hal/spi_types.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_port_t  port;
    gpio_num_t  sda;
    gpio_num_t  scl;
    uint32_t    freq;
    uint8_t     addr;
    uint16_t    timeout_ms;
} esp_idf_i2c_config_t;

typedef struct {
    spi_host_device_t host;
    gpio_num_t        mosi;
    gpio_num_t        sclk;
    gpio_num_t        cs;
    gpio_num_t        dc;
    gpio_num_t        rst;
    int               freq;
    int               queue_size;
    int               max_transfer_sz; /**< 单次传输上限（字节）；<=0 = IDF 默认 4092。
                                            整帧刷屏需 >= width*height*2 */
    bool              hw_cs;           /**< true = CS 交给 SPI 外设按事务拉低（整条
                                            事务保持选中，一次 RAMWR 连续写的要求）；
                                            false = 由 HAL 的 set_cs 手工控制 */
} esp_idf_spi_config_t;

typedef struct {
    uint32_t pin_mask;
    bool     pull_up;
    bool     pull_down;    /**< 与 pull_up 同时为 true 时下拉优先（掩码内全部生效） */
    bool     active_low;   /**< true = 低电平视为按下（上拉接地按键，如 VAMeter） */
} esp_idf_gpio_config_t;

eui_hal_i2c_t* eui_port_esp_idf_i2c_create(const esp_idf_i2c_config_t *cfg);
void eui_port_esp_idf_i2c_destroy(eui_hal_i2c_t *hal);

eui_hal_spi_t* eui_port_esp_idf_spi_create(const esp_idf_spi_config_t *cfg);
void eui_port_esp_idf_spi_destroy(eui_hal_spi_t *hal);

eui_hal_gpio_t* eui_port_esp_idf_gpio_create(const esp_idf_gpio_config_t *cfg);
void eui_port_esp_idf_gpio_destroy(eui_hal_gpio_t *hal);

/** 最近一次 SPI/I2C 传输的错误码（0 = 无错）。传输接口是 void 返回，
 *  brick 可在关键路径（如首帧提交）后查询本值做自诊断。 */
int eui_port_esp_idf_last_error(void);

/** PCNT 正交计数编码器：half-quad 模式（A 相双沿计数，2 计数/格），
 *  与老固件 ESP32Encoder::attachHalfQuad 的计数倍率一致。
 *  pull_up 通常为 true（多数编码器模块开漏输出）。 */
eui_hal_encoder_t *eui_port_esp_idf_encoder_create(int pin_a, int pin_b, bool pull_up);
void eui_port_esp_idf_encoder_destroy(eui_hal_encoder_t *hal);

/* ---- 板级装配（brick 契约） ----
 * 顺序是硬约束：池 → transport → driver → eui_init → tick 回调 →
 * display->init。core 不会调用 display->init()，由 board_init 调。 */

typedef struct {
    int      spi_host;      /* SPI2_HOST / SPI3_HOST */
    int      pin_mosi, pin_sclk, pin_cs, pin_dc, pin_rst, pin_bl; /* bl 仅记录，由 brick 自配 PWM */
    uint16_t width, height;
    uint8_t  col_offset, row_offset, madctl;
    bool     invert;
    bool     little_endian; /* ESP32 原生 uint16 布局用 true（RAMCTL bit3） */
    int      freq_hz;       /* 80MHz 高刷 / 40MHz 常规 */
    bool     hw_cs;         /* true = CS 交给 SPI 外设按事务拉低 */
} eui_port_esp_idf_display_t;

typedef struct {
    int      i2c_port;
    int      pin_sda, pin_scl;
    uint32_t freq;          /* 通常 400000 */
    uint8_t  addr;          /* SSD1306 常见 0x3C */
    uint16_t timeout_ms;    /* 单次传输超时，通常 100 */
} eui_port_esp_idf_i2c_t;

typedef enum {
    EUI_PORT_DISP_ST7789 = 0,   /* SPI，16bpp FULL（RGB565） */
    EUI_PORT_DISP_SSD1306,      /* I2C，1bpp（页缓冲由驱动处理） */
} eui_port_disp_kind_t;

typedef struct {
    eui_port_disp_kind_t kind;
    union {
        eui_port_esp_idf_display_t st7789;
        struct {
            eui_port_esp_idf_i2c_t i2c;
            uint16_t width, height;
            uint8_t  addr;
        } ssd1306;
    };
} eui_port_esp_idf_panel_t;

typedef struct {
    int      enc_pin_a, enc_pin_b;   /* < 0 = 不接编码器 */
    int      btn_pin[4];
    uint8_t  btn_key[4];             /* 语义键号（如 0=OK 1=BACK），由项目定义 */
    uint8_t  btn_count;
    bool     active_low;             /* 上拉接地按键用 true */
} eui_port_esp_idf_input_t;

typedef struct {
    eui_port_esp_idf_panel_t   panel;    /* 面板类型 + 引脚/时序 */
    eui_port_esp_idf_input_t   input;
    uint16_t fps;                    /* 0 = delay_frame 不做节拍 */
    uint8_t *mem_pool;               /* 必须 DMA 可达的内部 RAM（SPI DMA 读不了 PSRAM） */
    size_t   mem_pool_size;
} eui_port_esp_idf_board_t;

/** 按契约完成装配。失败返回负值（已打印原因），成功返回 0；
 *  调用后即可 build UI 并进入 eui_tick() 循环。 */
int  eui_port_esp_idf_board_init(const eui_port_esp_idf_board_t *board);

/** 拆解装配（eui_deinit → driver → transport，反序释放）。 */
void eui_port_esp_idf_board_deinit(void);

/** 帧节拍：按 eui_get_fps() 与上一帧实际耗时补偿，取代硬编码 vTaskDelay(16)；
 *  fps == 0 时不做节拍。 */
void eui_port_esp_idf_delay_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_ESP_IDF_H */
