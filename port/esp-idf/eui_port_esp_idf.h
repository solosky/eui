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
} esp_idf_spi_config_t;

typedef struct {
    uint32_t pin_mask;
    bool     pull_up;
} esp_idf_gpio_config_t;

eui_hal_i2c_t* eui_port_esp_idf_i2c_create(const esp_idf_i2c_config_t *cfg);
void eui_port_esp_idf_i2c_destroy(eui_hal_i2c_t *hal);

eui_hal_spi_t* eui_port_esp_idf_spi_create(const esp_idf_spi_config_t *cfg);
void eui_port_esp_idf_spi_destroy(eui_hal_spi_t *hal);

eui_hal_gpio_t* eui_port_esp_idf_gpio_create(const esp_idf_gpio_config_t *cfg);
void eui_port_esp_idf_gpio_destroy(eui_hal_gpio_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_ESP_IDF_H */
