#ifndef EUI_DRV_ENCODER_HW_H
#define EUI_DRV_ENCODER_HW_H

#include "eui/eui_input_drv.h"
#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 绝对计数编码器驱动配置。
 *
 * 适用于平台能提供硬件正交计数（ESP32 PCNT 等）的场合：读绝对计数求
 * 增量，避免轮询状态表在快速旋转时丢步。编码器按键不在本驱动范围内，
 * 请用 eui_drv_buttons 并经 eui_input_mux 合成。
 */
typedef struct {
    eui_hal_encoder_t hw;   /**< 绝对计数 HAL（按值传入） */
} eui_drv_encoder_hw_config_t;

/**
 * @brief 创建绝对计数编码器驱动。
 * @return 输入驱动；cfg 非法或分配失败返回 NULL。
 */
eui_input_drv_t *eui_drv_encoder_hw_create(const eui_drv_encoder_hw_config_t *cfg);

/**
 * @brief 释放驱动（不释放 HAL 持有的硬件资源，那由平台的 HAL destroy
 *        负责）。NULL 安全。
 */
void eui_drv_encoder_hw_destroy(eui_input_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ENCODER_HW_H */
