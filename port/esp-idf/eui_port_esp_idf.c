#include "eui_port_esp_idf.h"
#include "eui/eui_allocator.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* === I2C Implementation === */

typedef struct {
    i2c_port_t  port;
    uint8_t     addr;
    uint16_t    timeout_ms;
} i2c_priv_t;

static void esp_i2c_write_cmd(uint8_t cmd, void *user_data)
{
    i2c_priv_t *priv = (i2c_priv_t *)user_data;
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    if (!link) return;

    i2c_master_start(link);
    i2c_master_write_byte(link, (priv->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x00, true);
    i2c_master_write_byte(link, cmd, true);
    i2c_master_stop(link);

    i2c_master_cmd_begin(priv->port, link, pdMS_TO_TICKS(priv->timeout_ms));
    i2c_cmd_link_delete(link);
}

static void esp_i2c_write_data(const uint8_t *buf, uint32_t len, void *user_data)
{
    i2c_priv_t *priv = (i2c_priv_t *)user_data;
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    if (!link) return;

    i2c_master_start(link);
    i2c_master_write_byte(link, (priv->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x40, true);
    i2c_master_write(link, buf, len, true);
    i2c_master_stop(link);

    i2c_master_cmd_begin(priv->port, link, pdMS_TO_TICKS(priv->timeout_ms));
    i2c_cmd_link_delete(link);
}

static void esp_i2c_delay_ms(uint32_t ms, void *user_data)
{
    (void)user_data;
    vTaskDelay(pdMS_TO_TICKS(ms));
}

eui_hal_i2c_t* eui_port_esp_idf_i2c_create(const esp_idf_i2c_config_t *cfg)
{
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = cfg->sda,
        .scl_io_num = cfg->scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = cfg->freq,
    };

    if (i2c_param_config(cfg->port, &i2c_cfg) != ESP_OK) return NULL;
    if (i2c_driver_install(cfg->port, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK) return NULL;

    i2c_priv_t *priv = eui_malloc(sizeof(i2c_priv_t));
    if (!priv) {
        i2c_driver_delete(cfg->port);
        return NULL;
    }
    memset(priv, 0, sizeof(*priv));
    priv->port = cfg->port;
    priv->addr = cfg->addr;
    priv->timeout_ms = cfg->timeout_ms;

    eui_hal_i2c_t *hal = eui_malloc(sizeof(eui_hal_i2c_t));
    if (!hal) {
        i2c_driver_delete(cfg->port);
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->write_cmd = esp_i2c_write_cmd;
    hal->write_data = esp_i2c_write_data;
    hal->delay_ms = esp_i2c_delay_ms;
    hal->user_data = priv;

    return hal;
}

void eui_port_esp_idf_i2c_destroy(eui_hal_i2c_t *hal)
{
    if (!hal) return;

    i2c_priv_t *priv = (i2c_priv_t *)hal->user_data;
    if (priv) {
        i2c_driver_delete(priv->port);
        eui_free(priv);
    }
    eui_free(hal);
}

/* === SPI Implementation === */

typedef struct {
    spi_device_handle_t handle;
    gpio_num_t          dc_pin;
    gpio_num_t          cs_pin;
    gpio_num_t          rst_pin;
} spi_priv_t;

static void esp_spi_write_cmd(uint8_t cmd, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 0);

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(priv->handle, &trans);
}

static void esp_spi_write_data(const uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 1);

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = buf,
    };
    spi_device_polling_transmit(priv->handle, &trans);
}

static void esp_spi_read_data(uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 1);

    spi_transaction_t trans = {
        .length = len * 8,
        .rx_buffer = buf,
    };
    spi_device_polling_transmit(priv->handle, &trans);
}

static void esp_spi_set_dc(bool data_mode, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, data_mode ? 1 : 0);
}

static void esp_spi_set_cs(bool active, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    if (priv->cs_pin != GPIO_NUM_NC) {
        gpio_set_level(priv->cs_pin, active ? 0 : 1);
    }
}

static void esp_spi_set_rst(bool active, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    if (priv->rst_pin != GPIO_NUM_NC) {
        gpio_set_level(priv->rst_pin, active ? 1 : 0);
    }
}

static void esp_spi_delay_ms(uint32_t ms, void *user_data)
{
    (void)user_data;
    vTaskDelay(pdMS_TO_TICKS(ms));
}

eui_hal_spi_t* eui_port_esp_idf_spi_create(const esp_idf_spi_config_t *cfg)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = cfg->mosi,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = cfg->sclk,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4092,
    };

    if (spi_bus_initialize(cfg->host, &bus_cfg, SPI_DMA_DISABLED) != ESP_OK) return NULL;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = cfg->freq,
        .mode = 0,
        .spics_io_num = GPIO_NUM_NC,
        .queue_size = cfg->queue_size,
    };

    spi_device_handle_t handle;
    if (spi_bus_add_device(cfg->host, &dev_cfg, &handle) != ESP_OK) return NULL;

    if (cfg->dc != GPIO_NUM_NC) {
        gpio_set_direction(cfg->dc, GPIO_MODE_OUTPUT);
    }
    if (cfg->cs != GPIO_NUM_NC) {
        gpio_set_direction(cfg->cs, GPIO_MODE_OUTPUT);
        gpio_set_level(cfg->cs, 1);
    }
    if (cfg->rst != GPIO_NUM_NC) {
        gpio_set_direction(cfg->rst, GPIO_MODE_OUTPUT);
        gpio_set_level(cfg->rst, 0);
    }

    spi_priv_t *priv = eui_malloc(sizeof(spi_priv_t));
    if (!priv) {
        spi_bus_remove_device(handle);
        return NULL;
    }
    memset(priv, 0, sizeof(*priv));
    priv->handle = handle;
    priv->dc_pin = cfg->dc;
    priv->cs_pin = cfg->cs;
    priv->rst_pin = cfg->rst;

    eui_hal_spi_t *hal = eui_malloc(sizeof(eui_hal_spi_t));
    if (!hal) {
        spi_bus_remove_device(handle);
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->write_cmd = esp_spi_write_cmd;
    hal->write_data = esp_spi_write_data;
    hal->read_data = esp_spi_read_data;
    hal->set_dc = esp_spi_set_dc;
    hal->set_cs = esp_spi_set_cs;
    hal->set_rst = esp_spi_set_rst;
    hal->delay_ms = esp_spi_delay_ms;
    hal->user_data = priv;

    return hal;
}

void eui_port_esp_idf_spi_destroy(eui_hal_spi_t *hal)
{
    if (!hal) return;

    spi_priv_t *priv = (spi_priv_t *)hal->user_data;
    if (priv) {
        spi_bus_remove_device(priv->handle);
        eui_free(priv);
    }
    eui_free(hal);
}

/* === GPIO Implementation === */

typedef struct {
    uint32_t pin_mask;
} gpio_priv_t;

static bool esp_gpio_read_pin(uint8_t pin_id, void *user_data)
{
    gpio_priv_t *priv = (gpio_priv_t *)user_data;
    if (!((((uint32_t)1) << pin_id) & priv->pin_mask)) return false;
    return gpio_get_level((gpio_num_t)pin_id) != 0;
}

static void esp_gpio_delay_us(uint32_t us, void *user_data)
{
    (void)user_data;
    esp_rom_delay_us(us);
}

eui_hal_gpio_t* eui_port_esp_idf_gpio_create(const esp_idf_gpio_config_t *cfg)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = cfg->pin_mask,
        .mode = GPIO_MODE_INPUT,
    };
    if (cfg->pull_up) {
        io_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else {
        io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }
    io_cfg.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&io_cfg) != ESP_OK) return NULL;

    gpio_priv_t *priv = eui_malloc(sizeof(gpio_priv_t));
    if (!priv) return NULL;
    memset(priv, 0, sizeof(*priv));
    priv->pin_mask = cfg->pin_mask;

    eui_hal_gpio_t *hal = eui_malloc(sizeof(eui_hal_gpio_t));
    if (!hal) {
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->read_pin = esp_gpio_read_pin;
    hal->delay_us = esp_gpio_delay_us;
    hal->user_data = priv;

    return hal;
}

void eui_port_esp_idf_gpio_destroy(eui_hal_gpio_t *hal)
{
    if (!hal) return;

    gpio_priv_t *priv = (gpio_priv_t *)hal->user_data;
    if (priv) eui_free(priv);
    eui_free(hal);
}
