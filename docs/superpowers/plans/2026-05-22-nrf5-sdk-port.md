# nRF5 SDK 平台移植实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 EUI 框架新增 nRF5 SDK v17.x 平台移植层，实现 TWI (I2C)/SPI/GPIO 三个传输 HAL。

**Architecture:** 在 `port/nrf5/` 下新增独立平台适配层，通过 nRF5 SDK legacy 驱动 API (`nrf_drv_twi`, `nrf_drv_spi`, `nrf_gpio`) 填充 `eui_hal_i2c_t`/`eui_hal_spi_t`/`eui_hal_gpio_t` 回调。严格镜像 `port/esp-idf/` 的结构和代码风格。

**Tech Stack:** C99, nRF5 SDK v17.x, EUI framework, nrf_drv_twi, nrf_drv_spi, nrf_gpio, nrf_delay

---

## File Structure

```
port/nrf5/
├── CMakeLists.txt                    # 新增：双模 CMake（nRF5 SDK 集成 + 独立库）
├── eui_port_nrf5.h                   # 新增：端口公共头文件（配置结构体、API）
└── eui_port_nrf5.c                   # 新增：TWI/SPI/GPIO 传输回调实现
```

每一个文件单一职责，`eui_port_nrf5.c` 按 TWI / SPI / GPIO 三个独立段落组织，互不交叉。不依赖 FreeRTOS 或 SoftDevice。

---

### Task 1: 创建目录结构和端口头文件

**Files:**
- Create: `port/nrf5/eui_port_nrf5.h`

- [ ] **Step 1: 创建目录**

```bash
mkdir -p port/nrf5
```

- [ ] **Step 2: 创建头文件**

写入 `port/nrf5/eui_port_nrf5.h`：

```c
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
```

- [ ] **Step 3: 验证头文件自包含**

```bash
gcc -std=c99 -fsyntax-only -I include port/nrf5/eui_port_nrf5.h 2>&1
```

预期：编译通过，无错误。

- [ ] **Step 4: Commit**

```bash
git add port/nrf5/eui_port_nrf5.h
git commit -m "feat(port): add nrf5 port header with config structs"
```

---

### Task 2: 实现 TWI (I2C) 传输回调

**Files:**
- Create: `port/nrf5/eui_port_nrf5.c`

- [ ] **Step 1: 创建 .c 文件骨架并实现 TWI 回调**

写入 `port/nrf5/eui_port_nrf5.c`：

```c
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
```

- [ ] **Step 2: 验证语法**

```bash
gcc -std=c99 -Wall -Werror -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

预期：因缺少 nRF5 SDK 头而报错（正常）；确认无 C 语法错误。

```bash
gcc -std=c99 -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

预期：仅在 `#include "nrf_drv_twi.h"` 行报 `No such file`（非语法错误）。

- [ ] **Step 3: Commit**

```bash
git add port/nrf5/eui_port_nrf5.c
git commit -m "feat(port): implement nrf5 TWI (I2C) transport callbacks"
```

---

### Task 3: 实现 SPI 传输回调

**Files:**
- Modify: `port/nrf5/eui_port_nrf5.c`

- [ ] **Step 1: 在文件头部追加 SPI include**

在 `#include "nrf_delay.h"` 之后追加：

```c
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
```

- [ ] **Step 2: 在文件末尾（destroy 之后）追加 SPI 内部状态和回调**

追加以下代码：

```c
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
```

- [ ] **Step 2: 验证语法**

```bash
gcc -std=c99 -Wall -Werror -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

预期：仅在缺少 nRF5 SDK 头时 error（正常），无 C 语法错误。

```bash
gcc -std=c99 -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

- [ ] **Step 3: Commit**

```bash
git add port/nrf5/eui_port_nrf5.c
git commit -m "feat(port): implement nrf5 SPI transport callbacks"
```

---

### Task 4: 实现 GPIO 传输回调

**Files:**
- Modify: `port/nrf5/eui_port_nrf5.c`

- [ ] **Step 1: 在文件末尾追加 GPIO 内部状态和回调**

追加以下代码：

```c
/* ── GPIO internal state ──────────────────────────────────── */

typedef struct {
    uint32_t pin_mask;
} gpio_priv_t;

static bool nrf5_gpio_read_pin(uint8_t pin_id, void *user_data)
{
    gpio_priv_t *p = (gpio_priv_t *)user_data;
    if (!(p->pin_mask & (1UL << pin_id))) return false;
    return nrf_gpio_pin_read(pin_id) != 0;
}

static void nrf5_gpio_delay_us(uint32_t us, void *user_data)
{
    (void)user_data;
    nrf_delay_us(us);
}

eui_hal_gpio_t* eui_port_nrf5_gpio_create(const nrf5_gpio_config_t *cfg)
{
    for (uint8_t i = 0; i < cfg->num_buttons; i++) {
        nrf_gpio_cfg_input(cfg->buttons[i], NRF_GPIO_PIN_PULLUP);
    }

    gpio_priv_t *priv = eui_malloc(sizeof(gpio_priv_t));
    if (!priv) return NULL;
    priv->pin_mask = cfg->pin_mask;

    eui_hal_gpio_t *hal = eui_malloc(sizeof(eui_hal_gpio_t));
    if (!hal) {
        eui_free(priv);
        return NULL;
    }
    memset(hal, 0, sizeof(*hal));
    hal->read_pin = nrf5_gpio_read_pin;
    hal->delay_us = nrf5_gpio_delay_us;
    hal->user_data = priv;

    return hal;
}

void eui_port_nrf5_gpio_destroy(eui_hal_gpio_t *hal)
{
    if (!hal) return;

    gpio_priv_t *priv = (gpio_priv_t *)hal->user_data;
    if (priv) eui_free(priv);
    eui_free(hal);
}
```

- [ ] **Step 2: 验证语法**

```bash
gcc -std=c99 -Wall -Werror -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

预期：仅在缺少 nRF5 SDK 头时 error（正常），无 C 语法错误。

- [ ] **Step 3: Commit**

```bash
git add port/nrf5/eui_port_nrf5.c
git commit -m "feat(port): implement nrf5 GPIO transport callbacks"
```

---

### Task 5: 创建 CMakeLists.txt

**Files:**
- Create: `port/nrf5/CMakeLists.txt`

- [ ] **Step 1: 写入双模 CMake 文件**

```cmake
if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT $ENV{NRF5_SDK_ROOT})

    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)

    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${NRF5_SDK_ROOT}/components
        ${NRF5_SDK_ROOT}/components/drivers_nrf/twi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/spi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/delay
        ${NRF5_SDK_ROOT}/components/libraries/util
        ${NRF5_SDK_ROOT}/integration/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
        ${NRF5_SDK_ROOT}/modules/nrfx/hal
        ${NRF5_SDK_ROOT}/components/toolchain
    )

    target_compile_definitions(eui_port_nrf5 PRIVATE
        NRF52
        NRF52832_XXAA
    )

    target_link_libraries(eui_port_nrf5 PUBLIC eui)
else()
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)
    target_include_directories(eui_port_nrf5 PUBLIC .)
    target_link_libraries(eui_port_nrf5 PUBLIC eui)
endif()
```

- [ ] **Step 2: Commit**

```bash
git add port/nrf5/CMakeLists.txt
git commit -m "feat(port): add dual-mode CMakeLists for nrf5 port"
```

---

### Task 6: 最终验证

- [ ] **Step 1: 检查所有新增文件**

```bash
git status
```

预期输出确认文件列表：

```
new file:   port/nrf5/CMakeLists.txt
new file:   port/nrf5/eui_port_nrf5.h
new file:   port/nrf5/eui_port_nrf5.c
```

- [ ] **Step 2: 本地语法检查**

```bash
gcc -std=c99 -Wall -Werror -fsyntax-only -I include port/nrf5/eui_port_nrf5.c 2>&1
```

预期：因缺少 nRF5 SDK 头文件报 `fatal error: nrf_drv_twi.h: No such file or directory`。确认后续行均为此 cascading error 引起，而非 C 语法错误。

确认方式：注释掉 `#include "nrf_drv_twi.h"` 等 nRF5 SDK 头，临时用 `typedef int ret_code_t; #define NRF_SUCCESS 0` 占位，再次运行 `gcc -std=c99 -Wall -Werror -fsyntax-only`，应显示 `#include "eui/hal/eui_hal_types.h"` 等 EUI 头可正常找到，仅 `NRF_DRV_TWI_INSTANCE(0)` 等宏未定义（正常的，说明除 SDK 宏外无语法错误）。

- [ ] **Step 3: 与 ESP-IDF 端口对比验证结构一致性**

```bash
echo "=== ESP-IDF port ===" && ls -la port/esp-idf/
echo "=== nRF5 port ===" && ls -la port/nrf5/
```

确认两个端口目录都包含 `CMakeLists.txt`、`.h`、`.c` 三个文件，结构对等。

- [ ] **Step 4: Commit（如有遗漏）**

```bash
git status
```

预期：working tree clean，所有文件已提交。
