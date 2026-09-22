#ifndef EUI_DRV_ST7789_H
#define EUI_DRV_ST7789_H

#include "eui/eui_display_drv.h"
#include "eui/hal/eui_hal_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ST7789V/ST7789VW SPI display driver (RGB565, full-frame buffer).
 *
 * 初始化序列对齐 LovyanGFX Panel_ST7789 的默认行为（参考 VAMeter
 * 240x240 真机配置：4-wire SPI、invert=true）：SWRESET → SLPOUT →
 * COLMOD 16bit → MADCTL → INVON/INVOFF → NORON → DISPON。
 */
typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;      /**< 面板宽（像素） */
    uint16_t      height;     /**< 面板高（像素） */
    uint8_t       col_offset; /**< GRAM 列偏移（240x240 为 0；135x240 类为 40） */
    uint8_t       row_offset; /**< GRAM 行偏移（240x240 为 0；135x240 类为 53） */
    uint8_t       madctl;     /**< MADCTL 值（方向/RGB 顺序），0x00 = 竖屏 RGB */
    bool          invert;     /**< 多数 ST7789 面板需要 INVON（ips 屏） */
} eui_drv_st7789_config_t;

eui_display_drv_t* eui_drv_st7789_create(const eui_drv_st7789_config_t *cfg);
void eui_drv_st7789_destroy(eui_display_drv_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ST7789_H */
