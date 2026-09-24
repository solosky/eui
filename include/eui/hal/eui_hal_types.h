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
