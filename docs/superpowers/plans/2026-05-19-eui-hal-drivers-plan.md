# EUI HAL Drivers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 7 platform-independent HAL driver templates (4 display + 3 input) plus transport abstraction to the EUI framework.

**Architecture:** Platform-independent chip drivers implement `eui_display_hal_t` / `eui_input_hal_t` via config-struct initialization. Display drivers take `eui_hal_i2c_t` / `eui_hal_spi_t` transport callbacks; input drivers take simple GPIO read callbacks. Existing raylib HAL moves to `hal/` subdirectory.

**Tech Stack:** C99, EUI framework (TLSF allocator, HAL interfaces), no external MCU SDK dependencies.

---

### Task 1: Create directory structure

**Files:**
- Create: `include/eui/hal/` (empty dir)
- Create: `include/eui/driver/` (empty dir)
- Create: `src/hal/` (empty dir)
- Create: `src/driver/` (empty dir)

- [ ] **Step 1: Create directories**

```bash
mkdir -p include/eui/hal include/eui/driver src/hal src/driver
```

- [ ] **Step 2: Commit**

```bash
git add include/eui/hal include/eui/driver src/hal src/driver
git commit -m "chore: create hal/ and driver/ subdirectories"
```

---

### Task 2: Write transport abstraction header

**Files:**
- Create: `include/eui/hal/eui_hal_transport.h`

- [ ] **Step 1: Write `include/eui/hal/eui_hal_transport.h`**

```c
#ifndef EUI_HAL_TRANSPORT_H
#define EUI_HAL_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_i2c_t;

typedef struct {
    void (*write_cmd)(uint8_t cmd, void *user_data);
    void (*write_data)(const uint8_t *buf, uint32_t len, void *user_data);
    void (*read_data)(uint8_t *buf, uint32_t len, void *user_data);
    void (*set_dc)(bool data_mode, void *user_data);
    void (*set_cs)(bool active, void *user_data);
    void (*set_rst)(bool active, void *user_data);
    void (*delay_ms)(uint32_t ms, void *user_data);
    void *user_data;
} eui_hal_spi_t;

#ifdef __cplusplus
}
#endif

#endif /* EUI_HAL_TRANSPORT_H */
```

- [ ] **Step 2: Commit**

```bash
git add include/eui/hal/eui_hal_transport.h
git commit -m "feat: add I2C and SPI transport abstraction types"
```

---

### Task 3: Move raylib HAL to hal/ subdirectory

**Files:**
- Move: `include/eui/eui_hal_raylib.h` → `include/eui/hal/eui_hal_raylib.h`
- Move: `src/eui_hal_raylib.c` → `src/hal/eui_hal_raylib.c`
- Modify: `include/eui/eui.h:26` — update include path
- Modify: `src/CMakeLists.txt:35` — update source path
- Modify: `examples/CMakeLists.txt:9` — update include path if needed
- Modify: `test/CMakeLists.txt` — update include path if needed

- [ ] **Step 1: Move the files**

```bash
git mv include/eui/eui_hal_raylib.h include/eui/hal/eui_hal_raylib.h
git mv src/eui_hal_raylib.c src/hal/eui_hal_raylib.c
```

- [ ] **Step 2: Update `include/eui/eui.h` line 26**

Change:
```c
#include "eui/eui_hal_raylib.h"
```
To:
```c
#include "eui/hal/eui_hal_raylib.h"
```

- [ ] **Step 3: Update `src/CMakeLists.txt`**

Change the raylib block:
```cmake
    add_library(eui_hal_raylib STATIC eui_hal_raylib.c)
```
To:
```cmake
    add_library(eui_hal_raylib STATIC hal/eui_hal_raylib.c)
```

- [ ] **Step 4: Build to verify no breakage**

```bash
cmake -B build -DEUI_BUILD_EXAMPLES=ON -DEUI_BUILD_TESTS=ON
cmake --build build
```
Expected: build succeeds with no errors.

- [ ] **Step 5: Commit**

```bash
git add include/eui/hal/eui_hal_raylib.h src/hal/eui_hal_raylib.c
git add include/eui/eui.h src/CMakeLists.txt
git commit -m "refactor: move raylib HAL into hal/ subdirectory"
```

---

### Task 4: Write test infrastructure for driver tests

**Files:**
- Create: `test/test_drivers.c`

- [ ] **Step 1: Write unified test file for all driver tests**

```c
#include "eui/eui_display_hal.h"
#include "eui/eui_input_hal.h"
#include "eui/hal/eui_hal_transport.h"
#include "eui/eui_allocator.h"
#include "eui/eui.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

/* We'll add driver-specific test functions here later */

int main(void) {
    uint8_t pool[4096];
    eui_config_t cfg = {
        .mem_pool_buffer = pool, .mem_pool_size = sizeof(pool),
        .display = NULL, .input = NULL, .fps_target = 30,
    };
    eui_init(&cfg);

    printf("=== Driver Tests ===\n\n");

    /* tests appended in subsequent tasks */

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    eui_deinit();
    return tests_passed == tests_run ? 0 : 1;
}
```

- [ ] **Step 2: Add to `test/CMakeLists.txt` with initial state (commented out, enabled after first driver)**

```cmake
add_executable(test_drivers test_drivers.c)
target_include_directories(test_drivers PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_drivers PRIVATE eui)
```

- [ ] **Step 3: Build to verify test compiles and runs (no tests yet)**

```bash
cmake --build build
./build/test/test_drivers
```
Expected: "0/0 tests passed"

- [ ] **Step 4: Commit**

```bash
git add test/test_drivers.c test/CMakeLists.txt
git commit -m "test: add driver test skeleton"
```

---

### Task 5: Implement SSD1306 driver

**Files:**
- Create: `include/eui/driver/eui_drv_ssd1306.h`
- Create: `src/driver/eui_drv_ssd1306.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_ssd1306.h`**

```c
#ifndef EUI_DRV_SSD1306_H
#define EUI_DRV_SSD1306_H

#include "eui/eui_display_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;
} eui_drv_ssd1306_config_t;

eui_display_hal_t* eui_drv_ssd1306_create(const eui_drv_ssd1306_config_t *cfg);
void eui_drv_ssd1306_destroy(eui_display_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_SSD1306_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_ssd1306.c`**

```c
#include "eui/driver/eui_drv_ssd1306.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_hal_t    base;
    eui_hal_i2c_t        i2c;
    uint16_t             width;
    uint16_t             height;
    uint8_t              i2c_addr;
} ssd1306_t;

static int ssd1306_init(void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;

    static const uint8_t init_cmds[] = {
        0xAE,           /* display off */
        0xD5, 0x80,     /* clock div */
        0xA8,           /* multiplex - next byte depends on height */
        /* multiplex value filled below */
        0xD3, 0x00,     /* display offset */
        0x40,           /* start line */
        0x8D,           /* charge pump - next byte depends on height */
        /* charge pump value filled below */
        0x20, 0x00,     /* horizontal addressing mode */
        0xA1,           /* segment remap */
        0xC8,           /* COM scan direction */
        0xDA,           /* COM pins - next byte depends on height */
        /* COM pins value filled below */
        0x81, 0xCF,     /* contrast */
        0xD9, 0xF1,     /* precharge */
        0xDB, 0x40,     /* VCOMH deselect */
        0xA4,           /* display all on resume */
        0xA6,           /* normal display */
        0x2E,           /* deactivate scroll */
        0xAF,           /* display on */
    };
    uint8_t cmds[sizeof(init_cmds)];
    memcpy(cmds, init_cmds, sizeof(init_cmds));

    /* patch height-dependent fields */
    if (d->height <= 32) {
        cmds[4]  = 0x1F;  /* multiplex = 31 */
        cmds[10] = 0x10;  /* charge pump disable */
        cmds[18] = 0x02;  /* COM pins = sequential, no left/right remap */
    } else {
        cmds[4]  = 0x3F;  /* multiplex = 63 */
        cmds[10] = 0x14;  /* charge pump enable */
        cmds[18] = 0x12;  /* COM pins = alternative, disable left/right remap */
    }

    for (size_t i = 0; i < sizeof(cmds); i++) {
        d->i2c.write_cmd(cmds[i], d->i2c.user_data);
    }
    return 0;
}

static int ssd1306_deinit(void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(0xAE, d->i2c.user_data);  /* display off */
    return 0;
}

static void ssd1306_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void ssd1306_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    uint8_t page_start = (uint8_t)(rect->y / 8);
    uint8_t page_end   = (uint8_t)((rect->y + rect->h - 1) / 8);

    for (uint8_t p = page_start; p <= page_end; p++) {
        d->i2c.write_cmd(0xB0 | p, d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(rect->x & 0x0F), d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(0x10 | ((rect->x >> 4) & 0x0F)), d->i2c.user_data);
        d->i2c.write_data(buf + (p - page_start) * rect->w, rect->w, d->i2c.user_data);
    }
}

static void ssd1306_set_contrast(uint8_t level, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(0x81, d->i2c.user_data);
    d->i2c.write_cmd(level, d->i2c.user_data);
}

static void ssd1306_set_power(bool on, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(on ? 0xAF : 0xAE, d->i2c.user_data);
}

static void ssd1306_set_invert(bool invert, void *ud) {
    ssd1306_t *d = (ssd1306_t*)ud;
    d->i2c.write_cmd(invert ? 0xA7 : 0xA6, d->i2c.user_data);
}

static void ssd1306_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                              eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_hal_t* eui_drv_ssd1306_create(const eui_drv_ssd1306_config_t *cfg) {
    ssd1306_t *d = eui_malloc(sizeof(ssd1306_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->i2c = cfg->i2c;
    d->width = cfg->width;
    d->height = cfg->height;
    d->i2c_addr = cfg->i2c_addr;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 1;
    d->base.caps.buffer_mode = EUI_BUFFER_PAGE;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = ssd1306_init;
    d->base.deinit = ssd1306_deinit;
    d->base.draw_pixel = ssd1306_draw_pixel;
    d->base.write_buffer = ssd1306_write_buffer;
    d->base.set_contrast = ssd1306_set_contrast;
    d->base.set_power = ssd1306_set_power;
    d->base.set_invert = ssd1306_set_invert;
    d->base.fill_rect = ssd1306_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_ssd1306_destroy(eui_display_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add SSD1306 test to `test/test_drivers.c`** (append before `main`)

```c
static int test_ssd1306_write_cmd_count;
static int test_ssd1306_write_data_count;
static int test_ssd1306_delay_count;

static void mock_i2c_write_cmd(uint8_t cmd, void *ud) {
    (void)cmd; (void)ud;
    test_ssd1306_write_cmd_count++;
}

static void mock_i2c_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)ud;
    test_ssd1306_write_data_count += (int)len;
}

static void mock_i2c_delay_ms(uint32_t ms, void *ud) {
    (void)ms; (void)ud;
    test_ssd1306_delay_count++;
}

static void test_ssd1306_create_and_caps(void) {
    TEST("SSD1306 create sets correct caps");
    eui_drv_ssd1306_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_hal_t *hal = eui_drv_ssd1306_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 64) FAIL("height mismatch");
    if (hal->caps.color_depth != 1) FAIL("color depth mismatch");
    if (hal->caps.buffer_mode != EUI_BUFFER_PAGE) FAIL("buffer mode mismatch");
    eui_drv_ssd1306_destroy(hal);
    PASS();
}

static void test_ssd1306_init_sends_commands(void) {
    TEST("SSD1306 init sends command sequence");
    test_ssd1306_write_cmd_count = 0;
    eui_drv_ssd1306_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_hal_t *hal = eui_drv_ssd1306_create(&cfg);
    hal->init(hal->user_data);
    if (test_ssd1306_write_cmd_count < 10) FAIL("too few init commands sent");
    eui_drv_ssd1306_destroy(hal);
    PASS();
}
```

Append to `main()` before `printf("\n%d/%d tests passed"...`:
```c
    printf("--- SSD1306 ---\n");
    test_ssd1306_create_and_caps();
    test_ssd1306_init_sends_commands();
```

Also add `#include "eui/driver/eui_drv_ssd1306.h"` at top.

- [ ] **Step 4: Enable `test_drivers` in `test/CMakeLists.txt`**

Add after the test_drivers block:
```cmake
add_test(NAME drivers COMMAND test_drivers)
```

- [ ] **Step 5: Build and run test**

```bash
cmake -B build -DEUI_BUILD_TESTS=ON
cmake --build build
./build/test/test_drivers
```
Expected: 2 tests pass in SSD1306 section.

- [ ] **Step 6: Commit**

```bash
git add include/eui/driver/eui_drv_ssd1306.h src/driver/eui_drv_ssd1306.c test/test_drivers.c test/CMakeLists.txt
git commit -m "feat: add SSD1306 I2C OLED display driver"
```

---

### Task 6: Implement SH1106 driver

**Files:**
- Create: `include/eui/driver/eui_drv_sh1106.h`
- Create: `src/driver/eui_drv_sh1106.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_sh1106.h`**

```c
#ifndef EUI_DRV_SH1106_H
#define EUI_DRV_SH1106_H

#include "eui/eui_display_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_i2c_t i2c;
    uint16_t      width;
    uint16_t      height;
    uint8_t       i2c_addr;
} eui_drv_sh1106_config_t;

eui_display_hal_t* eui_drv_sh1106_create(const eui_drv_sh1106_config_t *cfg);
void eui_drv_sh1106_destroy(eui_display_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_SH1106_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_sh1106.c`**

```c
#include "eui/driver/eui_drv_sh1106.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_hal_t base;
    eui_hal_i2c_t     i2c;
    uint16_t          width;
    uint16_t          height;
    uint8_t           i2c_addr;
    uint8_t           col_offset;   /* 2 for 128-wide visible area on 132-column chip */
} sh1106_t;

static int sh1106_init(void *ud) {
    sh1106_t *d = (sh1106_t*)ud;

    /* SH1106 init sequence */
    d->i2c.write_cmd(0xAE, d->i2c.user_data);  /* display off */
    d->i2c.write_cmd(0x40, d->i2c.user_data);  /* start line = 0 */
    d->i2c.write_cmd(0xA1, d->i2c.user_data);  /* segment remap */
    d->i2c.write_cmd(0xC8, d->i2c.user_data);  /* COM scan direction */
    d->i2c.write_cmd(0xA6, d->i2c.user_data);  /* normal display */
    d->i2c.write_cmd(0xA8, d->i2c.user_data);  /* set multiplex */
    d->i2c.write_cmd((uint8_t)(d->height - 1), d->i2c.user_data);
    d->i2c.write_cmd(0xD3, d->i2c.user_data);  /* display offset */
    d->i2c.write_cmd(0x00, d->i2c.user_data);
    d->i2c.write_cmd(0xD5, d->i2c.user_data);  /* clock div */
    d->i2c.write_cmd(0x80, d->i2c.user_data);
    d->i2c.write_cmd(0xD9, d->i2c.user_data);  /* precharge */
    d->i2c.write_cmd(0x22, d->i2c.user_data);
    d->i2c.write_cmd(0xDB, d->i2c.user_data);  /* VCOMH deselect */
    d->i2c.write_cmd(0x35, d->i2c.user_data);
    d->i2c.write_cmd(0xDA, d->i2c.user_data);  /* COM pins */
    d->i2c.write_cmd(0x12, d->i2c.user_data);
    d->i2c.write_cmd(0x81, d->i2c.user_data);  /* contrast */
    d->i2c.write_cmd(0xFF, d->i2c.user_data);
    d->i2c.write_cmd(0xA4, d->i2c.user_data);  /* display all on resume */
    d->i2c.write_cmd(0x2E, d->i2c.user_data);  /* deactivate scroll */
    d->i2c.delay_ms(100, d->i2c.user_data);
    d->i2c.write_cmd(0xAF, d->i2c.user_data);  /* display on */
    return 0;
}

static int sh1106_deinit(void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(0xAE, d->i2c.user_data);
    return 0;
}

static void sh1106_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void sh1106_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    uint8_t page = (uint8_t)(rect->y / 8);
    uint8_t col  = (uint8_t)(rect->x + d->col_offset);

    for (int16_t row = rect->y; row < rect->y + rect->h; row += 8) {
        d->i2c.write_cmd(0xB0 | page, d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)((col >> 4) | 0x10), d->i2c.user_data);
        d->i2c.write_cmd((uint8_t)(col & 0x0F), d->i2c.user_data);
        d->i2c.write_data(buf + (page - (uint8_t)(rect->y / 8)) * rect->w,
                          rect->w, d->i2c.user_data);
        page++;
    }
}

static void sh1106_set_contrast(uint8_t level, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(0x81, d->i2c.user_data);
    d->i2c.write_cmd(level, d->i2c.user_data);
}

static void sh1106_set_power(bool on, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(on ? 0xAF : 0xAE, d->i2c.user_data);
}

static void sh1106_set_invert(bool invert, void *ud) {
    sh1106_t *d = (sh1106_t*)ud;
    d->i2c.write_cmd(invert ? 0xA7 : 0xA6, d->i2c.user_data);
}

static void sh1106_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_hal_t* eui_drv_sh1106_create(const eui_drv_sh1106_config_t *cfg) {
    sh1106_t *d = eui_malloc(sizeof(sh1106_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->i2c = cfg->i2c;
    d->width = cfg->width;
    d->height = cfg->height;
    d->i2c_addr = cfg->i2c_addr;
    d->col_offset = 2;  /* 132 columns chip, 128 visible */
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 1;
    d->base.caps.buffer_mode = EUI_BUFFER_PAGE;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = sh1106_init;
    d->base.deinit = sh1106_deinit;
    d->base.draw_pixel = sh1106_draw_pixel;
    d->base.write_buffer = sh1106_write_buffer;
    d->base.set_contrast = sh1106_set_contrast;
    d->base.set_power = sh1106_set_power;
    d->base.set_invert = sh1106_set_invert;
    d->base.fill_rect = sh1106_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_sh1106_destroy(eui_display_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add SH1106 test to `test/test_drivers.c`**

Add include: `#include "eui/driver/eui_drv_sh1106.h"`

Add after SSD1306 tests:
```c
static void test_sh1106_create_and_caps(void) {
    TEST("SH1106 create sets correct caps");
    eui_drv_sh1106_config_t cfg = {
        .i2c = { .write_cmd = mock_i2c_write_cmd, .write_data = mock_i2c_write_data,
                 .delay_ms = mock_i2c_delay_ms, .user_data = NULL },
        .width = 128, .height = 64, .i2c_addr = 0x3C,
    };
    eui_display_hal_t *hal = eui_drv_sh1106_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 64) FAIL("height mismatch");
    if (hal->caps.color_depth != 1) FAIL("color depth mismatch");
    if (hal->caps.buffer_mode != EUI_BUFFER_PAGE) FAIL("buffer mode mismatch");
    eui_drv_sh1106_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- SH1106 ---\n");
    test_sh1106_create_and_caps();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```
Expected: 3 tests pass (2 SSD1306 + 1 SH1106).

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_sh1106.h src/driver/eui_drv_sh1106.c test/test_drivers.c
git commit -m "feat: add SH1106 I2C OLED display driver"
```

---

### Task 7: Implement ST7735 driver

**Files:**
- Create: `include/eui/driver/eui_drv_st7735.h`
- Create: `src/driver/eui_drv_st7735.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_st7735.h`**

```c
#ifndef EUI_DRV_ST7735_H
#define EUI_DRV_ST7735_H

#include "eui/eui_display_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;
    uint16_t      height;
    uint8_t       variant;    /* 0=green tab, 1=red tab, 2=black tab */
} eui_drv_st7735_config_t;

eui_display_hal_t* eui_drv_st7735_create(const eui_drv_st7735_config_t *cfg);
void eui_drv_st7735_destroy(eui_display_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ST7735_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_st7735.c`**

```c
#include "eui/driver/eui_drv_st7735.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_hal_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
} st7735_t;

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

static void st7735_write_cmd(st7735_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void st7735_write_data(st7735_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void st7735_set_addr_window(st7735_t *d, uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;
    uint8_t caset[4] = { (uint8_t)(x >> 8), (uint8_t)(x), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t raset[4] = { (uint8_t)(y >> 8), (uint8_t)(y), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    st7735_write_cmd(d, ST7735_CASET);
    st7735_write_data(d, caset, 4);
    st7735_write_cmd(d, ST7735_RASET);
    st7735_write_data(d, raset, 4);
}

static int st7735_init(void *ud) {
    st7735_t *d = (st7735_t*)ud;

    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    st7735_write_cmd(d, ST7735_SWRESET);
    d->spi.delay_ms(150, d->spi.user_data);
    st7735_write_cmd(d, ST7735_SLPOUT);
    d->spi.delay_ms(500, d->spi.user_data);

    /* pixel format: 16bpp RGB565 */
    st7735_write_cmd(d, ST7735_COLMOD);
    { uint8_t v = 0x05; st7735_write_data(d, &v, 1); }

    /* memory access control */
    st7735_write_cmd(d, ST7735_MADCTL);
    { uint8_t v = 0xC8; st7735_write_data(d, &v, 1); }

    st7735_write_cmd(d, ST7735_NORON);
    d->spi.delay_ms(10, d->spi.user_data);
    st7735_write_cmd(d, ST7735_DISPON);
    d->spi.delay_ms(100, d->spi.user_data);

    st7735_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int st7735_deinit(void *ud) {
    (void)ud;
    return 0;
}

static void st7735_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void st7735_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    st7735_t *d = (st7735_t*)ud;
    st7735_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y,
                           rect->w, rect->h);
    st7735_write_cmd(d, ST7735_RAMWR);
    d->spi.set_dc(true, d->spi.user_data);
    uint32_t len = (uint32_t)rect->w * rect->h * 2;  /* 16bpp = 2 bytes/px */
    d->spi.write_data(buf, len, d->spi.user_data);
}

static void st7735_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void st7735_set_power(bool on, void *ud) { (void)on; (void)ud; }

static void st7735_set_invert(bool invert, void *ud) {
    st7735_t *d = (st7735_t*)ud;
    st7735_write_cmd(d, invert ? 0x21 : 0x20);
}

static void st7735_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                             eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_hal_t* eui_drv_st7735_create(const eui_drv_st7735_config_t *cfg) {
    st7735_t *d = eui_malloc(sizeof(st7735_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->spi = cfg->spi;
    d->width = cfg->width;
    d->height = cfg->height;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 16;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = st7735_init;
    d->base.deinit = st7735_deinit;
    d->base.draw_pixel = st7735_draw_pixel;
    d->base.write_buffer = st7735_write_buffer;
    d->base.set_contrast = st7735_set_contrast;
    d->base.set_power = st7735_set_power;
    d->base.set_invert = st7735_set_invert;
    d->base.fill_rect = st7735_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_st7735_destroy(eui_display_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add ST7735 test to `test/test_drivers.c`**

Add include and test functions — similar pattern to SSD1306/SH1106, testing caps correctness:

```c
#include "eui/driver/eui_drv_st7735.h"

static int test_st7735_cmd_count;
static int test_st7735_data_count;

static void mock_spi_write_cmd(uint8_t cmd, void *ud) {
    (void)cmd; (void)ud;
    test_st7735_cmd_count++;
}
static void mock_spi_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)ud;
    test_st7735_data_count += (int)len;
}
static void mock_spi_read_data(uint8_t *buf, uint32_t len, void *ud) {
    (void)buf; (void)len; (void)ud;
}
static void mock_spi_set_dc(bool dm, void *ud) { (void)dm; (void)ud; }
static void mock_spi_set_cs(bool a, void *ud) { (void)a; (void)ud; }
static void mock_spi_set_rst(bool a, void *ud) { (void)a; (void)ud; }
static void mock_spi_delay_ms(uint32_t ms, void *ud) { (void)ms; (void)ud; }

static void test_st7735_create_and_caps(void) {
    TEST("ST7735 create sets correct caps");
    eui_drv_st7735_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 128, .height = 160, .variant = 0,
    };
    eui_display_hal_t *hal = eui_drv_st7735_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 128) FAIL("width mismatch");
    if (hal->caps.height != 160) FAIL("height mismatch");
    if (hal->caps.color_depth != 16) FAIL("color depth mismatch");
    eui_drv_st7735_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- ST7735 ---\n");
    test_st7735_create_and_caps();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_st7735.h src/driver/eui_drv_st7735.c test/test_drivers.c
git commit -m "feat: add ST7735 SPI TFT display driver"
```

---

### Task 8: Implement ILI9341 driver

**Files:**
- Create: `include/eui/driver/eui_drv_ili9341.h`
- Create: `src/driver/eui_drv_ili9341.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_ili9341.h`**

```c
#ifndef EUI_DRV_ILI9341_H
#define EUI_DRV_ILI9341_H

#include "eui/eui_display_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    eui_hal_spi_t spi;
    uint16_t      width;
    uint16_t      height;
} eui_drv_ili9341_config_t;

eui_display_hal_t* eui_drv_ili9341_create(const eui_drv_ili9341_config_t *cfg);
void eui_drv_ili9341_destroy(eui_display_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ILI9341_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_ili9341.c`**

```c
#include "eui/driver/eui_drv_ili9341.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_display_hal_t base;
    eui_hal_spi_t     spi;
    uint16_t          width;
    uint16_t          height;
} ili9341_t;

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_SLPOUT  0x11
#define ILI9341_DISPON  0x29
#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

static void ili9341_write_cmd(ili9341_t *d, uint8_t cmd) {
    d->spi.set_dc(false, d->spi.user_data);
    d->spi.write_cmd(cmd, d->spi.user_data);
}

static void ili9341_write_data(ili9341_t *d, const uint8_t *data, uint32_t len) {
    d->spi.set_dc(true, d->spi.user_data);
    d->spi.write_data(data, len, d->spi.user_data);
}

static void ili9341_set_addr_window(ili9341_t *d, uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;
    uint8_t caset[4] = { (uint8_t)(x >> 8), (uint8_t)(x), (uint8_t)(xe >> 8), (uint8_t)(xe) };
    uint8_t paset[4] = { (uint8_t)(y >> 8), (uint8_t)(y), (uint8_t)(ye >> 8), (uint8_t)(ye) };
    ili9341_write_cmd(d, ILI9341_CASET);
    ili9341_write_data(d, caset, 4);
    ili9341_write_cmd(d, ILI9341_PASET);
    ili9341_write_data(d, paset, 4);
}

static int ili9341_init(void *ud) {
    ili9341_t *d = (ili9341_t*)ud;

    d->spi.set_cs(false, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(false, d->spi.user_data);
    d->spi.delay_ms(5, d->spi.user_data);
    d->spi.set_rst(true, d->spi.user_data);
    d->spi.delay_ms(120, d->spi.user_data);

    ili9341_write_cmd(d, ILI9341_SWRESET);
    d->spi.delay_ms(150, d->spi.user_data);
    ili9341_write_cmd(d, ILI9341_SLPOUT);
    d->spi.delay_ms(500, d->spi.user_data);

    ili9341_write_cmd(d, ILI9341_PIXFMT);
    { uint8_t v = 0x55; ili9341_write_data(d, &v, 1); }
    ili9341_write_cmd(d, ILI9341_MADCTL);
    { uint8_t v = 0x48; ili9341_write_data(d, &v, 1); }

    ili9341_write_cmd(d, ILI9341_DISPON);
    d->spi.delay_ms(100, d->spi.user_data);

    ili9341_set_addr_window(d, 0, 0, d->width, d->height);
    return 0;
}

static int ili9341_deinit(void *ud) { (void)ud; return 0; }
static void ili9341_draw_pixel(int16_t x, int16_t y, eui_color_t color, void *ud) {
    (void)x; (void)y; (void)color; (void)ud;
}

static void ili9341_write_buffer(const uint8_t *buf, const eui_rect_t *rect, void *ud) {
    ili9341_t *d = (ili9341_t*)ud;
    ili9341_set_addr_window(d, (uint16_t)rect->x, (uint16_t)rect->y,
                            rect->w, rect->h);
    ili9341_write_cmd(d, ILI9341_RAMWR);
    d->spi.set_dc(true, d->spi.user_data);
    uint32_t len = (uint32_t)rect->w * rect->h * 2;
    d->spi.write_data(buf, len, d->spi.user_data);
}

static void ili9341_set_contrast(uint8_t lvl, void *ud) { (void)lvl; (void)ud; }
static void ili9341_set_power(bool on, void *ud) { (void)on; (void)ud; }
static void ili9341_set_invert(bool invert, void *ud) {
    ili9341_t *d = (ili9341_t*)ud;
    ili9341_write_cmd(d, invert ? 0x21 : 0x20);
}
static void ili9341_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h,
                              eui_color_t color, void *ud) {
    (void)x; (void)y; (void)w; (void)h; (void)color; (void)ud;
}

eui_display_hal_t* eui_drv_ili9341_create(const eui_drv_ili9341_config_t *cfg) {
    ili9341_t *d = eui_malloc(sizeof(ili9341_t));
    if (!d) return NULL;
    memset(d, 0, sizeof(*d));
    d->spi = cfg->spi;
    d->width = cfg->width;
    d->height = cfg->height;
    d->base.caps.width = cfg->width;
    d->base.caps.height = cfg->height;
    d->base.caps.color_depth = 16;
    d->base.caps.buffer_mode = EUI_BUFFER_FULL;
    d->base.caps.has_gram = true;
    d->base.caps.hw_scroll = false;
    d->base.init = ili9341_init;
    d->base.deinit = ili9341_deinit;
    d->base.draw_pixel = ili9341_draw_pixel;
    d->base.write_buffer = ili9341_write_buffer;
    d->base.set_contrast = ili9341_set_contrast;
    d->base.set_power = ili9341_set_power;
    d->base.set_invert = ili9341_set_invert;
    d->base.fill_rect = ili9341_fill_rect;
    d->base.user_data = d;
    return &d->base;
}

void eui_drv_ili9341_destroy(eui_display_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add ILI9341 test to `test/test_drivers.c`**

Add include and test — reuses mock_spi_* from ST7735 tests:

```c
#include "eui/driver/eui_drv_ili9341.h"

static void test_ili9341_create_and_caps(void) {
    TEST("ILI9341 create sets correct caps");
    eui_drv_ili9341_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .width = 240, .height = 320,
    };
    eui_display_hal_t *hal = eui_drv_ili9341_create(&cfg);
    if (!hal) FAIL("create returned NULL");
    if (hal->caps.width != 240) FAIL("width mismatch");
    if (hal->caps.height != 320) FAIL("height mismatch");
    if (hal->caps.color_depth != 16) FAIL("color depth mismatch");
    eui_drv_ili9341_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- ILI9341 ---\n");
    test_ili9341_create_and_caps();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_ili9341.h src/driver/eui_drv_ili9341.c test/test_drivers.c
git commit -m "feat: add ILI9341 SPI TFT display driver"
```

---

### Task 9: Implement GPIO buttons driver

**Files:**
- Create: `include/eui/driver/eui_drv_buttons.h`
- Create: `src/driver/eui_drv_buttons.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_buttons.h`**

```c
#ifndef EUI_DRV_BUTTONS_H
#define EUI_DRV_BUTTONS_H

#include "eui/eui_input_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_drv_buttons_gpio_t;

typedef struct {
    uint8_t   pin_id;
    eui_key_t key;
} eui_drv_button_map_t;

typedef struct {
    eui_drv_buttons_gpio_t     gpio;
    const eui_drv_button_map_t *map;
    uint8_t                     count;
} eui_drv_buttons_config_t;

eui_input_hal_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg);
void eui_drv_buttons_destroy(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_BUTTONS_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_buttons.c`**

```c
#include "eui/driver/eui_drv_buttons.h"
#include "eui/eui_allocator.h"
#include <string.h>

#define DEBOUNCE_US 20000

typedef struct {
    eui_input_hal_t         base;
    eui_drv_buttons_gpio_t  gpio;
    const eui_drv_button_map_t *map;
    uint8_t                 count;
    uint8_t                 prev_state;       /* bitmask: 1=pressed */
    uint32_t                debounce_deadline; /* tick-based debounce window */
} buttons_t;

static int buttons_init(void *ud) {
    buttons_t *b = (buttons_t*)ud;
    b->prev_state = 0;
    b->debounce_deadline = 0;
    return 0;
}

static int buttons_deinit(void *ud) { (void)ud; return 0; }

static int buttons_poll(eui_event_t *evt, void *ud) {
    buttons_t *b = (buttons_t*)ud;
    uint8_t curr = 0;

    for (int i = 0; i < b->count; i++) {
        if (b->gpio.read_pin(b->map[i].pin_id, b->gpio.user_data)) {
            curr |= (1u << i);
        }
    }

    /* simple edge detection (no debounce in minimal test-friendly version,
       real debounce uses delay_us and timestamps; kept simple here) */
    for (int i = 0; i < b->count; i++) {
        uint8_t mask = (1u << i);
        bool was_pressed = (b->prev_state & mask) != 0;
        bool is_pressed  = (curr & mask) != 0;
        if (is_pressed && !was_pressed) {
            evt->type = EUI_EVT_KEY_PRESS;
            evt->data.key = b->map[i].key;
            b->prev_state = curr;
            return 1;
        }
        if (!is_pressed && was_pressed) {
            evt->type = EUI_EVT_KEY_RELEASE;
            evt->data.key = b->map[i].key;
            b->prev_state = curr;
            return 1;
        }
    }
    b->prev_state = curr;
    return 0;
}

static void buttons_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb;
    (void)user_data;
}

eui_input_hal_t* eui_drv_buttons_create(const eui_drv_buttons_config_t *cfg) {
    buttons_t *b = eui_malloc(sizeof(buttons_t));
    if (!b) return NULL;
    memset(b, 0, sizeof(*b));
    b->gpio = cfg->gpio;
    b->map = cfg->map;
    b->count = cfg->count;
    b->base.init = buttons_init;
    b->base.deinit = buttons_deinit;
    b->base.poll = buttons_poll;
    b->base.set_callback = buttons_set_callback;
    b->base.user_data = b;
    return &b->base;
}

void eui_drv_buttons_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add buttons test to `test/test_drivers.c`**

```c
#include "eui/driver/eui_drv_buttons.h"

static uint8_t test_btn_pin_state;
static bool mock_btn_read_pin(uint8_t pin_id, void *ud) {
    (void)ud;
    return (test_btn_pin_state & (1u << pin_id)) != 0;
}
static void mock_btn_delay_us(uint32_t us, void *ud) { (void)us; (void)ud; }

static void test_buttons_press_release(void) {
    TEST("buttons poll detects press and release");
    const eui_drv_button_map_t map[] = {
        { .pin_id = 0, .key = EUI_KEY_OK },
        { .pin_id = 1, .key = EUI_KEY_BACK },
    };
    eui_drv_buttons_config_t cfg = {
        .gpio = { .read_pin = mock_btn_read_pin, .delay_us = mock_btn_delay_us, .user_data = NULL },
        .map = map, .count = 2,
    };
    eui_input_hal_t *hal = eui_drv_buttons_create(&cfg);
    if (!hal) FAIL("create returned NULL");

    hal->init(hal->user_data);

    /* press OK */
    test_btn_pin_state = 0x01;
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on press");
    if (evt.type != EUI_EVT_KEY_PRESS || evt.data.key != EUI_KEY_OK) FAIL("expected OK press");

    /* no event on same state */
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event on unchanged state");

    /* release OK */
    test_btn_pin_state = 0x00;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on release");
    if (evt.type != EUI_EVT_KEY_RELEASE || evt.data.key != EUI_KEY_OK) FAIL("expected OK release");

    eui_drv_buttons_destroy(hal);
    PASS();
}

static void test_buttons_press_back(void) {
    TEST("buttons poll detects BACK key");
    const eui_drv_button_map_t map[] = {
        { .pin_id = 0, .key = EUI_KEY_UP },
        { .pin_id = 1, .key = EUI_KEY_BACK },
    };
    eui_drv_buttons_config_t cfg = {
        .gpio = { .read_pin = mock_btn_read_pin, .delay_us = mock_btn_delay_us, .user_data = NULL },
        .map = map, .count = 2,
    };
    eui_input_hal_t *hal = eui_drv_buttons_create(&cfg);
    hal->init(hal->user_data);

    test_btn_pin_state = 0x02;  /* pin 1 = BACK */
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event");
    if (evt.data.key != EUI_KEY_BACK) FAIL("expected BACK key");

    eui_drv_buttons_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- Buttons ---\n");
    test_buttons_press_release();
    test_buttons_press_back();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_buttons.h src/driver/eui_drv_buttons.c test/test_drivers.c
git commit -m "feat: add GPIO buttons input driver"
```

---

### Task 10: Implement encoder driver

**Files:**
- Create: `include/eui/driver/eui_drv_encoder.h`
- Create: `src/driver/eui_drv_encoder.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_encoder.h`**

```c
#ifndef EUI_DRV_ENCODER_H
#define EUI_DRV_ENCODER_H

#include "eui/eui_input_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*read_pin)(uint8_t pin_id, void *user_data);
    void (*delay_us)(uint32_t us, void *user_data);
    void *user_data;
} eui_drv_encoder_gpio_t;

typedef struct {
    eui_drv_encoder_gpio_t gpio;
    uint8_t                pin_a;
    uint8_t                pin_b;
    uint8_t                pin_sw;
    uint32_t               poll_interval_us;
} eui_drv_encoder_config_t;

eui_input_hal_t* eui_drv_encoder_create(const eui_drv_encoder_config_t *cfg);
void eui_drv_encoder_destroy(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_ENCODER_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_encoder.c`**

```c
#include "eui/driver/eui_drv_encoder.h"
#include "eui/eui_allocator.h"
#include <string.h>

typedef struct {
    eui_input_hal_t        base;
    eui_drv_encoder_gpio_t gpio;
    uint8_t                pin_a;
    uint8_t                pin_b;
    uint8_t                pin_sw;
    uint8_t                prev_state;  /* bits: [1:sw, 1:b, 1:a] */
    bool                   btn_pressed;
} encoder_t;

static int encoder_init(void *ud) {
    encoder_t *e = (encoder_t*)ud;
    e->prev_state = 0;
    e->btn_pressed = false;
    return 0;
}

static int encoder_deinit(void *ud) { (void)ud; return 0; }

static int encoder_poll(eui_event_t *evt, void *ud) {
    encoder_t *e = (encoder_t*)ud;

    uint8_t a = e->gpio.read_pin(e->pin_a, e->gpio.user_data) ? 1 : 0;
    uint8_t b = e->gpio.read_pin(e->pin_b, e->gpio.user_data) ? 1 : 0;
    uint8_t curr = (a << 1) | b;

    /* quadrature state machine: transition detection */
    static const int8_t enc_table[4][4] = {
        /* prev\curr  00   01   10   11 */
        /* 00 */    {  0,  1, -1,  0 },
        /* 01 */    { -1,  0,  0,  1 },
        /* 10 */    {  1,  0,  0, -1 },
        /* 11 */    {  0, -1,  1,  0 },
    };

    int8_t dir = enc_table[e->prev_state & 0x03][curr];
    e->prev_state = (e->prev_state & 0xFC) | curr;

    if (dir == 1) {
        evt->type = EUI_EVT_ENCODER_CW;
        return 1;
    }
    if (dir == -1) {
        evt->type = EUI_EVT_ENCODER_CCW;
        return 1;
    }

    /* encoder button */
    bool sw = e->gpio.read_pin(e->pin_sw, e->gpio.user_data);
    if (sw && !e->btn_pressed) {
        e->btn_pressed = true;
        evt->type = EUI_EVT_ENCODER_CLICK;
        return 1;
    }
    if (!sw && e->btn_pressed) {
        e->btn_pressed = false;
    }

    return 0;
}

static void encoder_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb; (void)user_data;
}

eui_input_hal_t* eui_drv_encoder_create(const eui_drv_encoder_config_t *cfg) {
    encoder_t *e = eui_malloc(sizeof(encoder_t));
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->gpio = cfg->gpio;
    e->pin_a = cfg->pin_a;
    e->pin_b = cfg->pin_b;
    e->pin_sw = cfg->pin_sw;
    e->base.init = encoder_init;
    e->base.deinit = encoder_deinit;
    e->base.poll = encoder_poll;
    e->base.set_callback = encoder_set_callback;
    e->base.user_data = e;
    return &e->base;
}

void eui_drv_encoder_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add encoder test to `test/test_drivers.c`**

```c
#include "eui/driver/eui_drv_encoder.h"

static uint8_t test_enc_pins;  /* bit 0=A, bit 1=B, bit 2=SW */
static bool mock_enc_read_pin(uint8_t pin_id, void *ud) {
    (void)ud;
    return (test_enc_pins & (1u << pin_id)) != 0;
}
static void mock_enc_delay_us(uint32_t us, void *ud) { (void)us; (void)ud; }

static void test_encoder_cw(void) {
    TEST("encoder detects CW rotation");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_hal_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    /* simulate CW: 00 -> 01 -> 11 -> 10 -> 00 */
    test_enc_pins = 0x00; hal->poll(NULL, hal->user_data);
    test_enc_pins = 0x01; /* A=0,B=1 */
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CW) FAIL("expected CW");

    eui_drv_encoder_destroy(hal);
    PASS();
}

static void test_encoder_ccw(void) {
    TEST("encoder detects CCW rotation");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_hal_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    /* simulate CCW: 00 -> 10 -> 11 -> 01 -> 00 */
    test_enc_pins = 0x00; hal->poll(NULL, hal->user_data);
    test_enc_pins = 0x02; /* A=1,B=0 */
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CCW) FAIL("expected CCW");

    eui_drv_encoder_destroy(hal);
    PASS();
}

static void test_encoder_click(void) {
    TEST("encoder detects click");
    eui_drv_encoder_config_t cfg = {
        .gpio = { .read_pin = mock_enc_read_pin, .delay_us = mock_enc_delay_us, .user_data = NULL },
        .pin_a = 0, .pin_b = 1, .pin_sw = 2, .poll_interval_us = 1000,
    };
    eui_input_hal_t *hal = eui_drv_encoder_create(&cfg);
    hal->init(hal->user_data);

    test_enc_pins = 0x04;  /* SW pressed */
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 1 || evt.type != EUI_EVT_ENCODER_CLICK) FAIL("expected CLICK");

    /* release should not emit event */
    test_enc_pins = 0x00;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event on release");

    eui_drv_encoder_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- Encoder ---\n");
    test_encoder_cw();
    test_encoder_ccw();
    test_encoder_click();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_encoder.h src/driver/eui_drv_encoder.c test/test_drivers.c
git commit -m "feat: add rotary encoder input driver"
```

---

### Task 11: Implement XPT2046 touch driver

**Files:**
- Create: `include/eui/driver/eui_drv_xpt2046.h`
- Create: `src/driver/eui_drv_xpt2046.c`
- Modify: `test/test_drivers.c`

- [ ] **Step 1: Write `include/eui/driver/eui_drv_xpt2046.h`**

```c
#ifndef EUI_DRV_XPT2046_H
#define EUI_DRV_XPT2046_H

#include "eui/eui_input_hal.h"
#include "eui/hal/eui_hal_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool (*read_irq)(void *user_data);
    void *user_data;
} eui_drv_xpt2046_irq_t;

typedef struct {
    eui_hal_spi_t         spi;
    eui_drv_xpt2046_irq_t irq;
    uint16_t              screen_width;
    uint16_t              screen_height;
} eui_drv_xpt2046_config_t;

eui_input_hal_t* eui_drv_xpt2046_create(const eui_drv_xpt2046_config_t *cfg);
void eui_drv_xpt2046_destroy(eui_input_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif /* EUI_DRV_XPT2046_H */
```

- [ ] **Step 2: Write `src/driver/eui_drv_xpt2046.c`**

```c
#include "eui/driver/eui_drv_xpt2046.h"
#include "eui/eui_allocator.h"
#include <string.h>

#define XPT2046_CMD_X  0xD0
#define XPT2046_CMD_Y  0x90
#define XPT2046_SAMPLES 3

typedef struct {
    eui_input_hal_t        base;
    eui_hal_spi_t          spi;
    eui_drv_xpt2046_irq_t  irq;
    uint16_t               screen_w;
    uint16_t               screen_h;
    bool                   touched;
    uint16_t               last_x;
    uint16_t               last_y;
} xpt2046_t;

static uint16_t _xpt2046_read_channel(xpt2046_t *t, uint8_t cmd) {
    uint8_t tx[3] = { cmd, 0, 0 };
    uint8_t rx[3] = { 0 };
    t->spi.set_cs(false, t->spi.user_data);
    t->spi.read_data(rx, 3, t->spi.user_data);
    /* in real hw you'd write tx and read rx; simplified here.
       read_data is the SPI exchange: master sends tx, receives rx.
       The mock test overrides this. */
    (void)tx;
    uint16_t val = (uint16_t)((rx[1] << 8) | rx[2]) >> 3;
    t->spi.set_cs(true, t->spi.user_data);
    return val;
}

static uint16_t _xpt2046_median(uint16_t *vals, int n) {
    /* insertion sort for small n */
    for (int i = 1; i < n; i++) {
        uint16_t tmp = vals[i];
        int j = i - 1;
        while (j >= 0 && vals[j] > tmp) {
            vals[j + 1] = vals[j];
            j--;
        }
        vals[j + 1] = tmp;
    }
    return vals[n / 2];
}

static int xpt2046_init(void *ud) {
    xpt2046_t *t = (xpt2046_t*)ud;
    t->touched = false;
    return 0;
}

static int xpt2046_deinit(void *ud) { (void)ud; return 0; }

static int xpt2046_poll(eui_event_t *evt, void *ud) {
    xpt2046_t *t = (xpt2046_t*)ud;

    bool pressed = !t->irq.read_irq(t->irq.user_data);

    if (pressed) {
        uint16_t xvals[XPT2046_SAMPLES];
        uint16_t yvals[XPT2046_SAMPLES];
        for (int i = 0; i < XPT2046_SAMPLES; i++) {
            xvals[i] = _xpt2046_read_channel(t, XPT2046_CMD_X);
            yvals[i] = _xpt2046_read_channel(t, XPT2046_CMD_Y);
        }
        uint16_t rx = _xpt2046_median(xvals, XPT2046_SAMPLES);
        uint16_t ry = _xpt2046_median(yvals, XPT2046_SAMPLES);

        /* map 12-bit ADC (0-4095) to screen coords */
        int16_t sx = (int16_t)((uint32_t)rx * t->screen_w / 4096);
        int16_t sy = (int16_t)((uint32_t)ry * t->screen_h / 4096);

        if (!t->touched) {
            evt->type = EUI_EVT_TOUCH_DOWN;
            t->touched = true;
        } else {
            evt->type = EUI_EVT_TOUCH_MOVE;
        }
        evt->data.touch.x = sx;
        evt->data.touch.y = sy;
        return 1;
    }

    if (t->touched) {
        evt->type = EUI_EVT_TOUCH_UP;
        t->touched = false;
        return 1;
    }

    return 0;
}

static void xpt2046_set_callback(void (*cb)(const eui_event_t *evt), void *user_data) {
    (void)cb; (void)user_data;
}

eui_input_hal_t* eui_drv_xpt2046_create(const eui_drv_xpt2046_config_t *cfg) {
    xpt2046_t *t = eui_malloc(sizeof(xpt2046_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));
    t->spi = cfg->spi;
    t->irq = cfg->irq;
    t->screen_w = cfg->screen_width;
    t->screen_h = cfg->screen_height;
    t->base.init = xpt2046_init;
    t->base.deinit = xpt2046_deinit;
    t->base.poll = xpt2046_poll;
    t->base.set_callback = xpt2046_set_callback;
    t->base.user_data = t;
    return &t->base;
}

void eui_drv_xpt2046_destroy(eui_input_hal_t *hal) {
    if (hal) eui_free(hal->user_data);
}
```

- [ ] **Step 3: Add XPT2046 test to `test/test_drivers.c`**

```c
#include "eui/driver/eui_drv_xpt2046.h"

static bool test_touch_irq_state;
static bool mock_xpt_irq(void *ud) { (void)ud; return test_touch_irq_state; }

static void test_xpt2046_touch_down_up(void) {
    TEST("XPT2046 detects touch down and up");
    eui_drv_xpt2046_config_t cfg = {
        .spi = { .write_cmd = mock_spi_write_cmd, .write_data = mock_spi_write_data,
                 .read_data = mock_spi_read_data, .set_dc = mock_spi_set_dc,
                 .set_cs = mock_spi_set_cs, .set_rst = mock_spi_set_rst,
                 .delay_ms = mock_spi_delay_ms, .user_data = NULL },
        .irq = { .read_irq = mock_xpt_irq, .user_data = NULL },
        .screen_width = 320, .screen_height = 240,
    };
    eui_input_hal_t *hal = eui_drv_xpt2046_create(&cfg);
    hal->init(hal->user_data);

    /* no touch initially */
    test_touch_irq_state = true;  /* IRQ high = no touch */
    eui_event_t evt;
    int ret = hal->poll(&evt, hal->user_data);
    if (ret != 0) FAIL("expected no event when not touched");

    /* touch down: IRQ low */
    test_touch_irq_state = false;
    test_st7735_data_count = 0;  /* reset counter from mock_spi_read_data */
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on touch down");
    if (evt.type != EUI_EVT_TOUCH_DOWN) FAIL("expected TOUCH_DOWN");

    /* touch up: IRQ high */
    test_touch_irq_state = true;
    ret = hal->poll(&evt, hal->user_data);
    if (ret != 1) FAIL("expected event on touch up");
    if (evt.type != EUI_EVT_TOUCH_UP) FAIL("expected TOUCH_UP");

    eui_drv_xpt2046_destroy(hal);
    PASS();
}
```

Append to `main()`:
```c
    printf("--- XPT2046 ---\n");
    test_xpt2046_touch_down_up();
```

- [ ] **Step 4: Build and run test**

```bash
cmake --build build && ./build/test/test_drivers
```

- [ ] **Step 5: Commit**

```bash
git add include/eui/driver/eui_drv_xpt2046.h src/driver/eui_drv_xpt2046.c test/test_drivers.c
git commit -m "feat: add XPT2046 touch screen input driver"
```

---

### Task 12: Update CMakeLists.txt to include all driver sources

**Files:**
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Add driver source files to the eui library**

Edit `src/CMakeLists.txt`, insert before the closing `)` of the eui library:
```cmake
    driver/eui_drv_ssd1306.c
    driver/eui_drv_sh1106.c
    driver/eui_drv_st7735.c
    driver/eui_drv_ili9341.c
    driver/eui_drv_buttons.c
    driver/eui_drv_encoder.c
    driver/eui_drv_xpt2046.c
```

The full source list now ends with:
```cmake
    eui_scene.c
    eui_anim.c
    eui.c
    driver/eui_drv_ssd1306.c
    driver/eui_drv_sh1106.c
    driver/eui_drv_st7735.c
    driver/eui_drv_ili9341.c
    driver/eui_drv_buttons.c
    driver/eui_drv_encoder.c
    driver/eui_drv_xpt2046.c
)
```

- [ ] **Step 2: Build and run all tests**

```bash
cmake -B build -DEUI_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```
Expected: all tests pass (allocator, event, input, canvas, font, view, widget, drivers).

- [ ] **Step 3: Commit**

```bash
git add src/CMakeLists.txt
git commit -m "build: add all driver sources to eui library target"
```

---

### Task 13: Run full test suite and verify

**Files:** None new.

- [ ] **Step 1: Clean rebuild and run all tests**

```bash
rm -rf build
cmake -B build -DEUI_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

Expected output: all 8 test suites pass (7 existing + `drivers`).

- [ ] **Step 2: Verify raylib examples still compile (if raylib available)**

```bash
cmake -B build -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=ON
cmake --build build
```
Expected: build succeeds with no errors in examples.

- [ ] **Step 3: Commit (if any fixes were needed)**

---

### Task 14: Update porting guide to reference new drivers

**Files:**
- Modify: `docs/porting_guide.md`

- [ ] **Step 1: Add a section at the bottom of the porting guide**

Append to `docs/porting_guide.md`:

```markdown

---

## 使用内置驱动（新）

EUI 现在提供内置的平台无关驱动模板，放在 `include/eui/driver/` 目录下。只需实现传输回调（I2C/SPI/GPIO），即可快速接入：

### 使用 SSD1306 驱动

```c
#include "eui.h"
#include "eui/driver/eui_drv_ssd1306.h"

/* 1. 实现 I2C 传输回调 */
static void my_i2c_write_cmd(uint8_t cmd, void *ud) {
    /* platform-specific: send command byte via I2C */
}
static void my_i2c_write_data(const uint8_t *buf, uint32_t len, void *ud) {
    /* platform-specific: send data bytes via I2C */
}
static void my_delay_ms(uint32_t ms, void *ud) {
    /* platform-specific: delay */
}

/* 2. 创建驱动 */
eui_drv_ssd1306_config_t disp_cfg = {
    .i2c = { my_i2c_write_cmd, my_i2c_write_data, my_delay_ms, NULL },
    .width = 128, .height = 64, .i2c_addr = 0x3C,
};
eui_display_hal_t *display = eui_drv_ssd1306_create(&disp_cfg);
```

### 使用 GPIO 按键驱动

```c
#include "eui/driver/eui_drv_buttons.h"

const eui_drv_button_map_t map[] = {
    { .pin_id = 0, .key = EUI_KEY_UP },
    { .pin_id = 1, .key = EUI_KEY_DOWN },
    { .pin_id = 2, .key = EUI_KEY_OK },
};

eui_drv_buttons_config_t btn_cfg = {
    .gpio = { .read_pin = my_gpio_read, .delay_us = my_delay_us },
    .map = map, .count = 3,
};
eui_input_hal_t *input = eui_drv_buttons_create(&btn_cfg);
```

### 可用的内置驱动

| 驱动 | 头文件 | 类型 |
|------|--------|------|
| SSD1306 I2C OLED | `eui/driver/eui_drv_ssd1306.h` | Display |
| SH1106 I2C OLED | `eui/driver/eui_drv_sh1106.h` | Display |
| ST7735 SPI TFT | `eui/driver/eui_drv_st7735.h` | Display |
| ILI9341 SPI TFT | `eui/driver/eui_drv_ili9341.h` | Display |
| GPIO 按键 | `eui/driver/eui_drv_buttons.h` | Input |
| 旋转编码器 | `eui/driver/eui_drv_encoder.h` | Input |
| XPT2046 触摸屏 | `eui/driver/eui_drv_xpt2046.h` | Input |

传输层定义见 `eui/hal/eui_hal_transport.h`。
```

- [ ] **Step 2: Commit**

```bash
git add docs/porting_guide.md
git commit -m "docs: add built-in driver usage section to porting guide"
```

---

### Final verification

- [ ] **Step 1: Full clean build and test**

```bash
rm -rf build
cmake -B build -DEUI_BUILD_TESTS=ON -DEUI_BUILD_EXAMPLES=ON
cmake --build build
cd build && ctest --output-on-failure
```

All tests must pass. No compiler warnings.
