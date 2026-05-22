#include "eui_port_nrf5.h"
#include "eui/eui_allocator.h"

#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"

#include <string.h>

/* ── TWI (I2C) internal state ─────────────────────────────── */

typedef struct {
    nrf_drv_twi_t twi;
    uint8_t       addr;
} i2c_priv_t;

static const nrf_drv_twi_t twi_instances[] = {
    NRF_DRV_TWI_INSTANCE(0),
    NRF_DRV_TWI_INSTANCE(1),
};

static void nrf5_i2c_write_cmd(uint8_t cmd, void *user_data)
{
    i2c_priv_t *p = (i2c_priv_t *)user_data;
    uint8_t buf[2] = { 0x00, cmd };
    (void)nrf_drv_twi_tx(&p->twi, p->addr, buf, 2, false);
}

static void nrf5_i2c_write_data(const uint8_t *buf, uint32_t len, void *user_data)
{
    i2c_priv_t *p = (i2c_priv_t *)user_data;
    uint8_t tmp[255];
    if (len > 254) return;
    tmp[0] = 0x40;
    memcpy(tmp + 1, buf, len);
    (void)nrf_drv_twi_tx(&p->twi, p->addr, tmp, len + 1, false);
}

static void nrf5_i2c_delay_ms(uint32_t ms, void *user_data)
{
    (void)user_data;
    nrf_delay_ms(ms);
}

eui_hal_i2c_t* eui_port_nrf5_i2c_create(const nrf5_i2c_config_t *cfg)
{
    if (cfg->instance_id >= 2) return NULL;

    nrf_drv_twi_config_t twi_cfg = {
        .scl                = cfg->scl,
        .sda                = cfg->sda,
        .frequency          = (cfg->freq >= 400000) ? NRF_DRV_TWI_FREQ_400K
                                                    : NRF_DRV_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOWEST,
        .clear_bus_init     = false,
        .hold_bus_uninit    = false,
    };

    i2c_priv_t *priv = eui_malloc(sizeof(i2c_priv_t));
    if (!priv) return NULL;

    priv->twi = twi_instances[cfg->instance_id];
    priv->addr = cfg->addr;

    ret_code_t err = nrf_drv_twi_init(&priv->twi, &twi_cfg, NULL, NULL);
    if (err != NRF_SUCCESS) {
        eui_free(priv);
        return NULL;
    }

    nrf_drv_twi_enable(&priv->twi);

    eui_hal_i2c_t *hal = eui_malloc(sizeof(eui_hal_i2c_t));
    if (!hal) {
        nrf_drv_twi_uninit(&priv->twi);
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->write_cmd = nrf5_i2c_write_cmd;
    hal->write_data = nrf5_i2c_write_data;
    hal->delay_ms = nrf5_i2c_delay_ms;
    hal->user_data = priv;

    return hal;
}

void eui_port_nrf5_i2c_destroy(eui_hal_i2c_t *hal)
{
    if (!hal) return;

    i2c_priv_t *priv = (i2c_priv_t *)hal->user_data;
    if (priv) {
        nrf_drv_twi_uninit(&priv->twi);
        eui_free(priv);
    }
    eui_free(hal);
}

/* ── SPI internal state ───────────────────────────────────── */

typedef struct {
    nrf_drv_spi_t spi;
    uint8_t       dc_pin;
    uint8_t       cs_pin;
    uint8_t       rst_pin;
} spi_priv_t;

static const nrf_drv_spi_t spi_instances[] = {
    NRF_DRV_SPI_INSTANCE(0),
    NRF_DRV_SPI_INSTANCE(1),
    NRF_DRV_SPI_INSTANCE(2),
};

static void nrf5_spi_write_cmd(uint8_t cmd, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_clear(p->cs_pin);
    }
    (void)nrf_drv_spi_transfer(&p->spi, &cmd, 1, NULL, 0);
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_set(p->cs_pin);
    }
}

static void nrf5_spi_write_data(const uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_clear(p->cs_pin);
    }
    (void)nrf_drv_spi_transfer(&p->spi, buf, (uint8_t)len, NULL, 0);
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_set(p->cs_pin);
    }
}

static void nrf5_spi_read_data(uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_clear(p->cs_pin);
    }
    (void)nrf_drv_spi_transfer(&p->spi, NULL, 0, buf, (uint8_t)len);
    if (p->cs_pin != NRF5_PIN_NOT_USED) {
        nrf_gpio_pin_set(p->cs_pin);
    }
}

static void nrf5_spi_set_dc(bool data_mode, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (data_mode) {
        nrf_gpio_pin_set(p->dc_pin);
    } else {
        nrf_gpio_pin_clear(p->dc_pin);
    }
}

static void nrf5_spi_set_cs(bool active, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (p->cs_pin == NRF5_PIN_NOT_USED) return;
    if (active) {
        nrf_gpio_pin_clear(p->cs_pin);
    } else {
        nrf_gpio_pin_set(p->cs_pin);
    }
}

static void nrf5_spi_set_rst(bool active, void *user_data)
{
    spi_priv_t *p = (spi_priv_t *)user_data;
    if (p->rst_pin == NRF5_PIN_NOT_USED) return;
    if (active) {
        nrf_gpio_pin_set(p->rst_pin);
    } else {
        nrf_gpio_pin_clear(p->rst_pin);
    }
}

static void nrf5_spi_delay_ms(uint32_t ms, void *user_data)
{
    (void)user_data;
    nrf_delay_ms(ms);
}

eui_hal_spi_t* eui_port_nrf5_spi_create(const nrf5_spi_config_t *cfg)
{
    if (cfg->instance_id >= 3) return NULL;

    nrf_drv_spi_config_t spi_cfg = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_cfg.sck_pin  = cfg->sck;
    spi_cfg.mosi_pin = cfg->mosi;
    spi_cfg.miso_pin = (cfg->miso == NRF5_PIN_NOT_USED)
                        ? NRF_DRV_SPI_PIN_NOT_USED : cfg->miso;
    spi_cfg.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED;
    spi_cfg.orc      = cfg->orc;
    spi_cfg.frequency = (cfg->freq >= 8000000) ? NRF_DRV_SPI_FREQ_8M
                       : (cfg->freq >= 4000000) ? NRF_DRV_SPI_FREQ_4M
                       : (cfg->freq >= 2000000) ? NRF_DRV_SPI_FREQ_2M
                       : (cfg->freq >= 1000000) ? NRF_DRV_SPI_FREQ_1M
                       : NRF_DRV_SPI_FREQ_500K;
    switch (cfg->mode) {
        case 0: spi_cfg.mode = NRF_DRV_SPI_MODE_0; break;
        case 1: spi_cfg.mode = NRF_DRV_SPI_MODE_1; break;
        case 2: spi_cfg.mode = NRF_DRV_SPI_MODE_2; break;
        case 3: spi_cfg.mode = NRF_DRV_SPI_MODE_3; break;
        default: spi_cfg.mode = NRF_DRV_SPI_MODE_0; break;
    }

    spi_priv_t *priv = eui_malloc(sizeof(spi_priv_t));
    if (!priv) return NULL;

    priv->spi = spi_instances[cfg->instance_id];
    priv->dc_pin = cfg->dc;
    priv->cs_pin = cfg->cs;
    priv->rst_pin = cfg->rst;

    ret_code_t err = nrf_drv_spi_init(&priv->spi, &spi_cfg, NULL, NULL);
    if (err != NRF_SUCCESS) {
        eui_free(priv);
        return NULL;
    }

    if (cfg->dc != NRF5_PIN_NOT_USED) {
        nrf_gpio_cfg_output(cfg->dc);
    }
    if (cfg->cs != NRF5_PIN_NOT_USED) {
        nrf_gpio_cfg_output(cfg->cs);
        nrf_gpio_pin_set(cfg->cs);
    }
    if (cfg->rst != NRF5_PIN_NOT_USED) {
        nrf_gpio_cfg_output(cfg->rst);
        nrf_gpio_pin_clear(cfg->rst);
    }

    eui_hal_spi_t *hal = eui_malloc(sizeof(eui_hal_spi_t));
    if (!hal) {
        nrf_drv_spi_uninit(&priv->spi);
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->write_cmd = nrf5_spi_write_cmd;
    hal->write_data = nrf5_spi_write_data;
    hal->read_data = nrf5_spi_read_data;
    hal->set_dc = nrf5_spi_set_dc;
    hal->set_cs = nrf5_spi_set_cs;
    hal->set_rst = nrf5_spi_set_rst;
    hal->delay_ms = nrf5_spi_delay_ms;
    hal->user_data = priv;

    return hal;
}

void eui_port_nrf5_spi_destroy(eui_hal_spi_t *hal)
{
    if (!hal) return;

    spi_priv_t *priv = (spi_priv_t *)hal->user_data;
    if (priv) {
        nrf_drv_spi_uninit(&priv->spi);
        eui_free(priv);
    }
    eui_free(hal);
}
