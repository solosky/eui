#ifndef EUI_INPUT_MUX_H
#define EUI_INPUT_MUX_H

#include <stdint.h>
#include "eui/eui_input_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 多路输入合成配置。
 *
 * drivers 是借用的指针数组（mux 不接管子驱动的生命周期），必须比 mux 活得久；
 * 数组内的 NULL 条目在轮询时被跳过。
 */
typedef struct {
    eui_input_drv_t *const *drivers; /**< 子驱动指针数组（借用） */
    uint8_t                 count;   /**< 子驱动个数（>= 1） */
} eui_input_mux_config_t;

/**
 * @brief 把多个输入驱动合成一个 eui_input_drv_t。
 *
 * 真实板子通常是「编码器 + 按键」多颗输入设备，而 eui_init() 只接受一个
 * input driver；本 mux 按 drivers 顺序轮询，第一个产出事件的子驱动胜出。
 * init/deinit 逐个下发；set_callback 为 no-op（多源回调语义不明确，
 * 回调驱动式输入请由平台自行接管通知）。
 *
 * @param cfg  配置；drivers/count 非法时返回 NULL。
 * @return 合成驱动，失败返回 NULL（用 eui_input_mux_destroy 释放）。
 */
eui_input_drv_t *eui_input_mux_create(const eui_input_mux_config_t *cfg);

/**
 * @brief 释放 mux（不释放子驱动）。NULL 安全。
 */
void eui_input_mux_destroy(eui_input_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_INPUT_MUX_H */
