#include "eui_port_nrf5.h"
#include "eui/eui_allocator.h"

#include "nrf_drv_twi.h"
#include "nrf_delay.h"

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
