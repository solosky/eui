#include "eui_port_esp_idf.h"
#include "eui/eui_allocator.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* 传输接口是 void 返回，错误只能记在侧通道：brick 可在关键路径后查询 */
static int g_last_error;

static void port_note_result(esp_err_t err, const char *what)
{
    g_last_error = (int)err;
    if (err != ESP_OK) {
        ESP_LOGE("eui_port", "%s failed: %s", what, esp_err_to_name(err));
    }
}

int eui_port_esp_idf_last_error(void) { return g_last_error; }

/* === I2C Implementation ===
 * 仍用 legacy 驱动（i2c_param_config + i2c_cmd_link）：IDF 5.1.5 还没有
 * i2c_master 新 API（i2c_new_master_bus/i2c_master_transmit 为 5.2 引入），
 * 而 VAMeter 的 arduino-esp32 组件硬性要求 5.1.x。工程升 IDF >= 5.2 时
 * 迁移到 i2c_master 并删除本段（legacy 驱动 6.x 移除）。 */

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
    bool                cs_owned_by_hw;  /* CS 由 SPI 外设按事务控制时，HAL 不再插手 */
    spi_host_device_t   host;
    bool                bus_initialized; /* 总线所有权在 transport，destroy 时归还 */
} spi_priv_t;

static void esp_spi_write_cmd(uint8_t cmd, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 0);

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    port_note_result(spi_device_polling_transmit(priv->handle, &trans), "spi write_cmd");
}

static void esp_spi_write_data(const uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 1);

    /* 整帧 115200 字节也走这里。S3 的单条 DMA 事务上限是
     * SPI_LL_DMA_MAX_BIT_LEN = 2^18 bit = 32KB（spi_master 的
     * check_trans_valid 直接拒绝更长的 length，与 max_transfer_sz 无关
     * ——计划期"IDF 自动分片"的结论在 S3 上不成立）。按 32KB 分块轮询
     * 发送：软件 CS 由驱动持有、跨块保持拉低（一次 RAMWR 连续写的
     * 要求），返回即发送完成。 */
    enum { SPI_CHUNK_MAX = 32768 };
    while (len > 0) {
        uint32_t chunk = len > SPI_CHUNK_MAX ? SPI_CHUNK_MAX : len;
        spi_transaction_t trans = {
            .length = chunk * 8,
            .tx_buffer = buf,
        };
        port_note_result(spi_device_polling_transmit(priv->handle, &trans), "spi write_data");
        buf += chunk;
        len -= chunk;
    }
}

static void esp_spi_read_data(uint8_t *buf, uint32_t len, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, 1);

    spi_transaction_t trans = {
        .length = len * 8,
        .rx_buffer = buf,
    };
    port_note_result(spi_device_polling_transmit(priv->handle, &trans), "spi read_data");
}

static void esp_spi_set_dc(bool data_mode, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    gpio_set_level(priv->dc_pin, data_mode ? 1 : 0);
}

static void esp_spi_set_cs(bool active, void *user_data)
{
    spi_priv_t *priv = (spi_priv_t *)user_data;
    /* 硬件 CS 模式下该引脚由 SPI 外设按事务拉低/释放，HAL 层不再插手
     * （ST7789 驱动只在 init 里调一次 set_cs(false)）。 */
    if (priv->cs_owned_by_hw) return;
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
        /* 整帧刷屏的关键：0 表示沿用安全默认 4092，整帧（240x240x16bpp
         * = 115200 字节）必须显式给出。 */
        .max_transfer_sz = cfg->max_transfer_sz > 0 ? cfg->max_transfer_sz : 4092,
    };

    /* DMA 必须开：SPI_DMA_DISABLED 下 IDF 把单次事务限制在 64 字节
     * （SPI_LL_CPU_MAX_BIT_LEN = 512bit），一帧被切成无数段且曾因返回码
     * 被忽略而表现为「帧静默丢失」。 */
    if (spi_bus_initialize(cfg->host, &bus_cfg, SPI_DMA_CH_AUTO) != ESP_OK) return NULL;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = cfg->freq,
        .mode = 0,
        /* hw_cs：外设持有 CS 并在整条事务期间保持拉低 */
        .spics_io_num = cfg->hw_cs ? cfg->cs : GPIO_NUM_NC,
        .queue_size = cfg->queue_size,
    };

    spi_device_handle_t handle;
    if (spi_bus_add_device(cfg->host, &dev_cfg, &handle) != ESP_OK) {
        spi_bus_free(cfg->host);
        return NULL;
    }

    if (cfg->dc != GPIO_NUM_NC) {
        gpio_set_direction(cfg->dc, GPIO_MODE_OUTPUT);
    }
    if (!cfg->hw_cs && cfg->cs != GPIO_NUM_NC) {
        /* 软件 CS 才由这里配置成输出；硬件 CS 的引脚归 SPI 外设 */
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
        spi_bus_free(cfg->host);
        return NULL;
    }
    memset(priv, 0, sizeof(*priv));
    priv->handle = handle;
    priv->dc_pin = cfg->dc;
    priv->cs_pin = cfg->cs;
    priv->rst_pin = cfg->rst;
    priv->cs_owned_by_hw = cfg->hw_cs;
    priv->host = cfg->host;
    priv->bus_initialized = true;

    eui_hal_spi_t *hal = eui_malloc(sizeof(eui_hal_spi_t));
    if (!hal) {
        spi_bus_remove_device(handle);
        spi_bus_free(cfg->host);
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
        if (priv->handle) spi_bus_remove_device(priv->handle);
        /* 不归还总线会让同一 host 再次 create 失败 */
        if (priv->bus_initialized) spi_bus_free(priv->host);
        eui_free(priv);
    }
    eui_free(hal);
}

/* === GPIO Implementation === */

typedef struct {
    uint32_t pin_mask;
    bool     active_low;   /* 低电平视为按下（上拉接地按键） */
} gpio_priv_t;

static bool esp_gpio_read_pin(uint8_t pin_id, void *user_data)
{
    gpio_priv_t *priv = (gpio_priv_t *)user_data;
    if (!((((uint32_t)1) << pin_id) & priv->pin_mask)) {
        /* 曾经静默返回 false——配错掩码时整条输入链路失效却毫无提示 */
        static bool warned;
        if (!warned) {
            warned = true;
            ESP_LOGW("eui_port", "read_pin(%u) not in pin_mask 0x%08x; input silently dead",
                     (unsigned)pin_id, (unsigned)priv->pin_mask);
        }
        return false;
    }
    bool level = gpio_get_level((gpio_num_t)pin_id) != 0;
    return priv->active_low ? !level : level;
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
    io_cfg.pull_up_en = (cfg->pull_up && !cfg->pull_down) ? GPIO_PULLUP_ENABLE
                                                          : GPIO_PULLUP_DISABLE;
    io_cfg.pull_down_en = cfg->pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type = GPIO_INTR_DISABLE;

    if (gpio_config(&io_cfg) != ESP_OK) return NULL;

    gpio_priv_t *priv = eui_malloc(sizeof(gpio_priv_t));
    if (!priv) return NULL;
    memset(priv, 0, sizeof(*priv));
    priv->pin_mask = cfg->pin_mask;
    priv->active_low = cfg->active_low;

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

/* === PCNT Encoder Implementation === */

typedef struct {
    pcnt_unit_handle_t unit;
} enc_priv_t;

static int32_t esp_enc_read_count(void *user_data) {
    enc_priv_t *priv = (enc_priv_t *)user_data;
    int count = 0;
    pcnt_unit_get_count(priv->unit, &count);
    return (int32_t)count;
}

eui_hal_encoder_t *eui_port_esp_idf_encoder_create(int pin_a, int pin_b, bool pull_up)
{
    if (pin_a < 0 || pin_b < 0) return NULL;

    pcnt_unit_config_t unit_cfg = {
        .high_limit = 30000,
        .low_limit = -30000,
    };
    pcnt_unit_handle_t unit = NULL;
    if (pcnt_new_unit(&unit_cfg, &unit) != ESP_OK) return NULL;

    pcnt_glitch_filter_config_t filter = { .max_glitch_ns = 1000 };
    pcnt_unit_set_glitch_filter(unit, &filter);

    /* half-quad：A 相做边沿计数、B 相做电平定向（2 计数/格），与老固件
     * ESP32Encoder::attachHalfQuad 的倍率一致 */
    pcnt_chan_config_t cha = { .edge_gpio_num = (gpio_num_t)pin_a, .level_gpio_num = (gpio_num_t)pin_b };
    pcnt_chan_config_t chb = { .edge_gpio_num = (gpio_num_t)pin_b, .level_gpio_num = (gpio_num_t)pin_a };
    pcnt_channel_handle_t chan_a = NULL, chan_b = NULL;
    if (pcnt_new_channel(unit, &cha, &chan_a) != ESP_OK) { pcnt_del_unit(unit); return NULL; }
    if (pcnt_new_channel(unit, &chb, &chan_b) != ESP_OK) {
        pcnt_del_channel(chan_a);
        pcnt_del_unit(unit);
        return NULL;
    }

    pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                         PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                          PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                         PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
                                          PCNT_CHANNEL_LEVEL_ACTION_KEEP);

    if (pull_up) {
        gpio_pullup_en((gpio_num_t)pin_a);
        gpio_pullup_en((gpio_num_t)pin_b);
    } else {
        gpio_pullup_dis((gpio_num_t)pin_a);
        gpio_pullup_dis((gpio_num_t)pin_b);
    }

    if (pcnt_unit_enable(unit) != ESP_OK || pcnt_unit_clear_count(unit) != ESP_OK ||
        pcnt_unit_start(unit) != ESP_OK) {
        pcnt_del_channel(chan_a);
        pcnt_del_channel(chan_b);
        pcnt_del_unit(unit);
        return NULL;
    }

    enc_priv_t *priv = eui_malloc(sizeof(enc_priv_t));
    eui_hal_encoder_t *hal = eui_malloc(sizeof(eui_hal_encoder_t));
    if (!priv || !hal) {
        eui_free(priv);
        eui_free(hal);
        pcnt_unit_stop(unit);
        pcnt_unit_disable(unit);
        pcnt_del_channel(chan_a);
        pcnt_del_channel(chan_b);
        pcnt_del_unit(unit);
        return NULL;
    }
    priv->unit = unit;
    hal->read_count = esp_enc_read_count;
    hal->user_data = priv;
    return hal;
}

void eui_port_esp_idf_encoder_destroy(eui_hal_encoder_t *hal)
{
    if (!hal) return;
    enc_priv_t *priv = (enc_priv_t *)hal->user_data;
    if (priv) {
        if (priv->unit) {
            pcnt_unit_stop(priv->unit);
            pcnt_unit_disable(priv->unit);
            pcnt_del_unit(priv->unit);
        }
        eui_free(priv);
    }
    eui_free(hal);
}

/* === 背光（LEDC PWM） === */

static bool s_bl_ready;
static uint8_t s_bl_level;
static ledc_timer_t s_bl_timer = LEDC_TIMER_1;
static ledc_channel_t s_bl_channel = LEDC_CHANNEL_1;

int eui_port_esp_idf_backlight_init(const esp_idf_backlight_config_t *cfg)
{
    if (!cfg || cfg->pin < 0) return -1;
    s_bl_timer = cfg->timer >= 0 ? (ledc_timer_t)cfg->timer : LEDC_TIMER_1;
    s_bl_channel = cfg->channel >= 0 ? (ledc_channel_t)cfg->channel : LEDC_CHANNEL_1;

    ledc_timer_config_t tcfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = s_bl_timer,
        .freq_hz = cfg->freq_hz ? (int)cfg->freq_hz : 500,   /* 老固件背光 500Hz */
        .clk_cfg = LEDC_AUTO_CLK,
    };
    if (ledc_timer_config(&tcfg) != ESP_OK) return -1;
    ledc_channel_config_t ccfg = {
        .gpio_num = (gpio_num_t)cfg->pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = s_bl_channel,
        .timer_sel = s_bl_timer,
        .duty = cfg->init_level,
        .hpoint = 0,
    };
    if (ledc_channel_config(&ccfg) != ESP_OK) return -1;
    s_bl_level = cfg->init_level;
    s_bl_ready = true;
    return 0;
}

void eui_port_esp_idf_backlight_set(uint8_t level)
{
    if (!s_bl_ready) return;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, s_bl_channel, level);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, s_bl_channel);
    s_bl_level = level;
}

uint8_t eui_port_esp_idf_backlight_get(void)
{
    return s_bl_ready ? s_bl_level : 0;
}

void eui_port_esp_idf_backlight_deinit(void)
{
    if (!s_bl_ready) return;
    ledc_stop(LEDC_LOW_SPEED_MODE, s_bl_channel, 0);
    s_bl_ready = false;
    s_bl_level = 0;
}

/* === Board bring-up（brick 契约） === */

#include "esp_timer.h"
#include "eui/eui.h"
#include "eui/driver/eui_drv_st7789.h"
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/driver/eui_drv_buttons.h"
#include "eui/driver/eui_drv_encoder_hw.h"
#include "eui/eui_input_mux.h"

#define EUI_PORT_TAG "eui_port"

static struct {
    eui_display_drv_t  *display;
    eui_hal_spi_t      *spi;
    eui_hal_i2c_t      *i2c;
    eui_input_drv_t    *encoder;
    eui_hal_encoder_t  *encoder_hal;
    eui_input_drv_t    *buttons;
    eui_hal_gpio_t     *gpio;
    eui_input_drv_t    *mux;          /* 单输入时等于该子驱动指针（不重复释放） */
    bool                mux_owned;    /* mux 由本模块创建时才负责销毁 */
    uint32_t            frame_start_us;
} g_board;

static eui_drv_buttons_map_t s_btn_map[4];   /* buttons 驱动借用，须常驻 */

static uint32_t port_tick_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int eui_port_esp_idf_board_init(const eui_port_esp_idf_board_t *board)
{
    if (!board || !board->mem_pool || board->mem_pool_size == 0) {
        ESP_LOGE(EUI_PORT_TAG, "board_init: mem_pool required");
        return -1;
    }
    size_t canvas = 0;
    if (board->panel.kind == EUI_PORT_DISP_ST7789) {
        canvas = (size_t)board->panel.st7789.width * board->panel.st7789.height * 2u;
        if (canvas > board->mem_pool_size) {
            ESP_LOGE(EUI_PORT_TAG, "board_init: pool %u < canvas %u (w*h*2)",
                     (unsigned)board->mem_pool_size, (unsigned)canvas);
            return -1;
        }
        esp_idf_spi_config_t spi_cfg = {
            .host = (spi_host_device_t)board->panel.st7789.spi_host,
            .mosi = (gpio_num_t)board->panel.st7789.pin_mosi,
            .sclk = (gpio_num_t)board->panel.st7789.pin_sclk,
            .cs   = (gpio_num_t)board->panel.st7789.pin_cs,
            .dc   = (gpio_num_t)board->panel.st7789.pin_dc,
            .rst  = (gpio_num_t)board->panel.st7789.pin_rst,
            .freq = board->panel.st7789.freq_hz,
            .queue_size = 1,
            .max_transfer_sz = (int)canvas,      /* 整帧一次事务 */
            .hw_cs = board->panel.st7789.hw_cs,
        };
        g_board.spi = eui_port_esp_idf_spi_create(&spi_cfg);
        if (!g_board.spi) { ESP_LOGE(EUI_PORT_TAG, "spi create failed"); return -1; }

        eui_drv_st7789_config_t dcfg = {
            .spi = *g_board.spi,                 /* 驱动按值持有副本；transport 必须活得比驱动久 */
            .width = board->panel.st7789.width,
            .height = board->panel.st7789.height,
            .col_offset = board->panel.st7789.col_offset,
            .row_offset = board->panel.st7789.row_offset,
            .madctl = board->panel.st7789.madctl,
            .invert = board->panel.st7789.invert,
            .little_endian = board->panel.st7789.little_endian,
        };
        g_board.display = eui_drv_st7789_create(&dcfg);
    } else {
        const uint16_t w = board->panel.ssd1306.width;
        const uint16_t h = board->panel.ssd1306.height;
        canvas = (size_t)w * h / 8u;             /* 1bpp */
        if (canvas > board->mem_pool_size) {
            ESP_LOGE(EUI_PORT_TAG, "board_init: pool %u < canvas %u (w*h/8)",
                     (unsigned)board->mem_pool_size, (unsigned)canvas);
            return -1;
        }
        esp_idf_i2c_config_t i2c_cfg = {
            .port = (i2c_port_t)board->panel.ssd1306.i2c.i2c_port,
            .sda  = (gpio_num_t)board->panel.ssd1306.i2c.pin_sda,
            .scl  = (gpio_num_t)board->panel.ssd1306.i2c.pin_scl,
            .freq = board->panel.ssd1306.i2c.freq,
            .addr = board->panel.ssd1306.addr,
            .timeout_ms = board->panel.ssd1306.i2c.timeout_ms,
        };
        g_board.i2c = eui_port_esp_idf_i2c_create(&i2c_cfg);
        if (!g_board.i2c) { ESP_LOGE(EUI_PORT_TAG, "i2c create failed"); return -1; }

        eui_drv_ssd1306_config_t dcfg = {
            .i2c = *g_board.i2c,
            .width = w,
            .height = h,
            .i2c_addr = board->panel.ssd1306.addr,
        };
        g_board.display = eui_drv_ssd1306_create(&dcfg);
    }
    if (!g_board.display) { ESP_LOGE(EUI_PORT_TAG, "display create failed"); return -1; }

    /* 输入：编码器（可选）+ 按键（可选），多路经 mux 合成 */
    eui_input_drv_t *subs[2] = { NULL, NULL };
    int sub_count = 0;
    if (board->input.enc_pin_a >= 0 && board->input.enc_pin_b >= 0) {
        g_board.encoder_hal = eui_port_esp_idf_encoder_create(
            board->input.enc_pin_a, board->input.enc_pin_b, true);
        if (!g_board.encoder_hal) { ESP_LOGE(EUI_PORT_TAG, "pcnt create failed"); return -1; }
        eui_drv_encoder_hw_config_t ecfg = { .hw = *g_board.encoder_hal };
        g_board.encoder = eui_drv_encoder_hw_create(&ecfg);
        if (!g_board.encoder) { ESP_LOGE(EUI_PORT_TAG, "encoder drv failed"); return -1; }
        subs[sub_count++] = g_board.encoder;
    }
    if (board->input.btn_count > 0) {
        uint32_t mask = 0;
        for (uint8_t i = 0; i < board->input.btn_count && i < 4; i++) {
            s_btn_map[i].pin_id = (uint8_t)board->input.btn_pin[i];
            s_btn_map[i].key    = board->input.btn_key[i];
            mask |= 1u << board->input.btn_pin[i];
        }
        esp_idf_gpio_config_t gcfg = {
            .pin_mask = mask,
            .pull_up = true,
            .active_low = board->input.active_low,
        };
        g_board.gpio = eui_port_esp_idf_gpio_create(&gcfg);
        if (!g_board.gpio) { ESP_LOGE(EUI_PORT_TAG, "gpio create failed"); return -1; }
        eui_drv_buttons_config_t bcfg = {
            .gpio = *g_board.gpio, .map = s_btn_map, .count = board->input.btn_count,
        };
        g_board.buttons = eui_drv_buttons_create(&bcfg);
        if (!g_board.buttons) { ESP_LOGE(EUI_PORT_TAG, "buttons drv failed"); return -1; }
        subs[sub_count++] = g_board.buttons;
    }
    if (sub_count == 0) { ESP_LOGE(EUI_PORT_TAG, "no input configured"); return -1; }
    if (sub_count == 1) {
        g_board.mux = subs[0];
        g_board.mux_owned = false;
    } else {
        eui_input_mux_config_t mcfg = { .drivers = subs, .count = (uint8_t)sub_count };
        g_board.mux = eui_input_mux_create(&mcfg);
        g_board.mux_owned = (g_board.mux != NULL);
        if (!g_board.mux) { ESP_LOGE(EUI_PORT_TAG, "input mux failed"); return -1; }
    }

    eui_config_t ecfg = {
        .mem_pool_buffer = board->mem_pool,
        .mem_pool_size = board->mem_pool_size,
        .display = g_board.display,
        .input = g_board.mux,
        .fps_target = board->fps,
    };
    if (eui_init(&ecfg) != 0) {                  /* 返回码不能丢 */
        ESP_LOGE(EUI_PORT_TAG, "eui_init failed (pool %u bytes)", (unsigned)board->mem_pool_size);
        return -1;
    }
    eui_set_tick_callback(port_tick_ms);
    if (g_board.display->init(g_board.display->user_data) != 0) {
        ESP_LOGE(EUI_PORT_TAG, "display init failed");
        return -1;
    }

    /* 背光：面板配了 pin_bl 才起（失败只告警，不阻断 bring-up——屏幕已可用，
     * 只是暗着）。放在 display->init 之后：点亮时首帧内容已就绪，不闪白。 */
    if (board->panel.kind == EUI_PORT_DISP_ST7789 && board->panel.st7789.pin_bl >= 0) {
        esp_idf_backlight_config_t blcfg = {
            .pin = board->panel.st7789.pin_bl,
            .freq_hz = (uint32_t)board->panel.st7789.bl_freq_hz,
            .init_level = board->panel.st7789.bl_init_level,
            .timer = -1, .channel = -1,
        };
        if (eui_port_esp_idf_backlight_init(&blcfg) != 0)
            ESP_LOGW(EUI_PORT_TAG, "backlight init failed (pin %d)", board->panel.st7789.pin_bl);
    }

    eui_allocator_stats_t st = { 0 };
    eui_allocator_get_stats(&st);
    ESP_LOGI(EUI_PORT_TAG, "bringup ok: pool %u used %u peak %u; canvas needs %u",
             (unsigned)st.total, (unsigned)st.used, (unsigned)st.peak, (unsigned)canvas);
    g_board.frame_start_us = (uint32_t)esp_timer_get_time();
    return 0;
}

void eui_port_esp_idf_delay_frame(void)
{
    uint16_t fps = eui_get_fps();
    if (fps == 0) return;
    uint32_t budget_us = 1000000u / fps;
    uint32_t elapsed = (uint32_t)esp_timer_get_time() - g_board.frame_start_us;
    if (elapsed < budget_us) {
        /* 补不上就下一帧多休，不做追帧（追帧会让动画时间轴抖动） */
        uint32_t remain = budget_us - elapsed;
        vTaskDelay(pdMS_TO_TICKS(remain / 1000) + 1);
    }
    g_board.frame_start_us = (uint32_t)esp_timer_get_time();
}

void eui_port_esp_idf_board_deinit(void)
{
    eui_port_esp_idf_backlight_deinit();
    eui_deinit();
    if (g_board.mux_owned && g_board.mux) eui_input_mux_destroy(g_board.mux);
    if (g_board.buttons) eui_drv_buttons_destroy(g_board.buttons);
    if (g_board.encoder) eui_drv_encoder_hw_destroy(g_board.encoder);
    if (g_board.encoder_hal) eui_port_esp_idf_encoder_destroy(g_board.encoder_hal);
    if (g_board.gpio) eui_port_esp_idf_gpio_destroy(g_board.gpio);
    if (g_board.display) {
        if (g_board.spi) eui_drv_st7789_destroy(g_board.display);
        else             eui_drv_ssd1306_destroy(g_board.display);
    }
    if (g_board.spi) eui_port_esp_idf_spi_destroy(g_board.spi);
    if (g_board.i2c) eui_port_esp_idf_i2c_destroy(g_board.i2c);
    memset(&g_board, 0, sizeof(g_board));
}
