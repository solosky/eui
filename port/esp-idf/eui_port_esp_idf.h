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

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_ESP_IDF_H */
