#ifndef EUI_HAL_TYPES_H
#define EUI_HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_i2c_t;

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*read_data)(uint8_t *buf, uint32_t len, void *user_data);
    void (*set_dc)(bool data_mode, void *user_data);
    /** SPI 片选线电平（低有效）：false = 拉低选中，true = 拉高释放。
     *  **每个事务都要括起**（拉低→发命令/数据→拉高）：ST7789 这类面板要求 CS
     *  在每个事务之间释放，否则 4 线串口不会被 CS 的去选中沿同步——真机表现为
     *  SCLK/MOSI/DC 波形全部正确、面板却始终未被配置（背光亮而屏全黑）。
     *  显示驱动按"一条命令 + 其数据"为一组括起；触摸等共享设备同样逐事务括起。 */
    void (*set_cs)(bool active, void *user_data);
    void (*set_rst)(bool active, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_spi_t;

typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_hal_gpio_t;

/** 绝对计数编码器 HAL：硬件计数器（PCNT/正交解码器）提供绝对计数，
 *  驱动负责求增量。相比 eui_hal_gpio_t 的轮询状态表，它不会因为采样
 *  周期长而丢掉快速旋转的刻度。 */
typedef struct {
    int32_t (*read_count)(void *user_data);
    void *user_data;
} eui_hal_encoder_t;

#ifdef __cplusplus
}
#endif

#endif /* EUI_HAL_TYPES_H */
