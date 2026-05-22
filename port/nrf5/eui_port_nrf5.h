#ifndef EUI_PORT_NRF5_H
#define EUI_PORT_NRF5_H

#include <stdint.h>
#include <stdbool.h>

#include "eui/hal/eui_hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NRF5_PIN_NOT_USED 0xFF

/* ── I2C (TWI) ───────────────────────────────────────────── */

typedef struct {
    uint8_t  sda;
    uint8_t  scl;
    uint8_t  addr;         /* 7-bit device address */
    uint32_t freq;         /* 100000 or 400000 */
    uint8_t  instance_id;  /* 0 = TWI0, 1 = TWI1 */
} nrf5_i2c_config_t;

eui_hal_i2c_t* eui_port_nrf5_i2c_create(const nrf5_i2c_config_t *cfg);
void eui_port_nrf5_i2c_destroy(eui_hal_i2c_t *hal);

/* ── SPI ─────────────────────────────────────────────────── */

typedef struct {
    uint8_t  mosi;
    uint8_t  miso;         /* NRF5_PIN_NOT_USED if unused */
    uint8_t  sck;
    uint8_t  cs;           /* NRF5_PIN_NOT_USED if unused */
    uint8_t  dc;
    uint8_t  rst;          /* NRF5_PIN_NOT_USED if unused */
    uint32_t freq;         /* e.g. 8000000 */
    uint8_t  mode;         /* SPI mode 0-3 */
    uint8_t  instance_id;  /* 0 = SPI0, 1 = SPI1, 2 = SPI2 */
    uint8_t  orc;          /* over-run character, typically 0xFF */
} nrf5_spi_config_t;

eui_hal_spi_t* eui_port_nrf5_spi_create(const nrf5_spi_config_t *cfg);
void eui_port_nrf5_spi_destroy(eui_hal_spi_t *hal);

/* ── GPIO ────────────────────────────────────────────────── */

typedef struct {
    uint32_t pin_mask;     /* bitmask for quick lookup */
    uint8_t  buttons[8];   /* pin numbers for up to 8 buttons */
    uint8_t  num_buttons;
} nrf5_gpio_config_t;

eui_hal_gpio_t* eui_port_nrf5_gpio_create(const nrf5_gpio_config_t *cfg);
void eui_port_nrf5_gpio_destroy(eui_hal_gpio_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_PORT_NRF5_H */
