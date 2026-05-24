# Example Config Profiles: Platform × Driver × Depth Organization

## Problem Statement

EUI examples span three independent dimensions:

| Dimension | Values |
|-----------|--------|
| Platform | raylib (PC), esp-idf, nrf5 |
| Driver chip | SSD1306, SH1106, ST7735, ILI9341, raylib (simulated) |
| Color depth | 1bpp, 4bpp, 8bpp, 16bpp |

The current design (see [cross-platform examples spec](2026-05-23-cross-platform-examples-design.md)) handles **platform** via `EUI_TARGET_PORT` and driver/pins via per-example Kconfig. However, two limitations remain:

1. **Color depth is global** — `EUI_COLOR_DEPTH` is a root CMake variable that applies to *all* examples in a build. You cannot build `basic_label` (1bpp) and `color_demo` (16bpp) side-by-side.
2. **Driver config lives in example directories** — Each `cross/` example has its own Kconfig with SSD1306 vs ST7735 pin definitions. This couples example code to hardware config, and duplicates Kconfig across every example.

A `basic_label` should work on raylib-128x64-1bpp, esp32-ssd1306-1bpp, esp32-st7735-16bpp, and nrf52-ili9341-16bpp — without any code differences. The example author writes UI logic once; the build system combines it with the right hardware profile.

## Design: Config Profiles

Introduce **config profiles** — standalone configuration units that bundle a complete hardware environment: platform, display driver, color depth, resolution, and pin assignments.

### Key insight

```
Example logic       Config profile         Build output
(what to show)  ×   (what hardware)   →    (one executable)
```

An example is a pure function `eui_example_setup(cfg)`. A config profile is the hardware context. The build system pairs any example with any compatible profile and produces one executable.

This separates "what UI do I want to demonstrate" from "what hardware do I have", eliminating the N×M×D file explosion (no `ssd1306_basic_label`, `st7735_basic_label`, etc.).

### Constraint checking

Examples declare their requirements. The build system skips incompatible example+profile combinations:

```
Example            Requires              Compatible profiles
──────────────────────────────────────────────────────────────
basic_label       (none)                all profiles
color_demo        depth>=16             raylib_240x240_16bpp, esp32_st7735_16bpp
amiibo_demo       depth>=16, w>=240     raylib_240x240_16bpp, esp32_st7735_16bpp
list_nav          w>=128                all profiles
```

## Config Profile Structure

### File format

Each profile is a CMake snippet (not Kconfig) because profiles are consumed by CMake at configure time, not by an embedded menuconfig tool. Kconfig remains for the esp-idf/nrf5 interactive configuration path.

```
configs/
├── pc/
│   ├── raylib_128x64_1bpp.cmake
│   └── raylib_240x240_16bpp.cmake
├── esp32/
│   ├── ssd1306_i2c_128x64_1bpp.cmake
│   ├── st7735_spi_240x240_16bpp.cmake
│   └── ili9341_spi_320x240_16bpp.cmake
└── nrf52/
    ├── ssd1306_i2c_128x64_1bpp.cmake
    └── st7735_spi_240x240_16bpp.cmake
```

### Profile contents

```cmake
# configs/esp32/st7735_spi_240x240_16bpp.cmake
set(EUI_TARGET_PORT      "esp-idf")
set(EUI_COLOR_DEPTH      16)
set(EUI_DISPLAY_DRIVER   "st7735")
set(EUI_DISPLAY_WIDTH    240)
set(EUI_DISPLAY_HEIGHT   240)
set(EUI_BUFFER_MODE      "page")

# SPI pins
set(EUI_PIN_SPI_HOST     1)
set(EUI_PIN_MOSI         23)
set(EUI_PIN_SCLK         18)
set(EUI_PIN_CS           5)
set(EUI_PIN_DC           16)
set(EUI_PIN_RST          17)
set(EUI_PIN_BL           4)

# Input
set(EUI_INPUT_DRIVER     "buttons")
set(EUI_PIN_BTN_UP       34)
set(EUI_PIN_BTN_DOWN     35)
set(EUI_PIN_BTN_OK       32)
set(EUI_PIN_BTN_BACK     33)
```

### Profile → C code

The config profile sets CMake variables. At configure time, these are translated into a generated header:

```c
// auto-generated: build/include/eui/eui_profile_config.h
#define EUI_DISPLAY_DRIVER    EUI_DRIVER_ST7735
#define EUI_DISPLAY_WIDTH     240
#define EUI_DISPLAY_HEIGHT    240
#define EUI_PIN_MOSI          23
#define EUI_PIN_SCLK          18
// ...
```

The port bootstrap includes this header and uses the defines (not runtime strings) to select drivers:

```c
// Inside eui_port_bootstrap_main():
#if EUI_DISPLAY_DRIVER == EUI_DRIVER_ST7735
    eui_display_drv_t *display = eui_drv_st7735_create(EUI_PIN_SPI_HOST, EUI_PIN_MOSI, ...);
#elif EUI_DISPLAY_DRIVER == EUI_DRIVER_SSD1306
    eui_display_drv_t *display = eui_drv_ssd1306_create(EUI_PIN_I2C_PORT, EUI_PIN_SDA, ...);
#endif
```

This keeps driver selection compile-time (zero runtime overhead) and avoids string comparisons in embedded code.

### CMake presets integration

Profiles can be referenced from CMake presets for one-liner builds:

```json
{
  "configurePresets": [
    { "name": "pc-1bpp",   "cacheVariables": { "EUI_CONFIG_PROFILE": "configs/pc/raylib_128x64_1bpp.cmake" } },
    { "name": "esp-st7735","cacheVariables": { "EUI_CONFIG_PROFILE": "configs/esp32/st7735_spi_240x240_16bpp.cmake" } }
  ]
}
```

Usage: `cmake --preset esp-st7735 -B build && cmake --build build`

## Example Constraint Declarations

Each cross-platform example declares its minimum requirements in a `CMakeLists.txt` comment block that the build system can parse:

```cmake
# examples/cross/color_demo/CMakeLists.txt
# @requires EUI_COLOR_DEPTH >= 16
add_executable(color_demo color_demo.c)
target_link_libraries(color_demo PRIVATE eui ${BOOTSTRAP_LIB})
```

At configure time, the build system reads these constraints. If the active profile is incompatible, the example is excluded with a warning.

Alternatively, a simpler approach: each example directory contains a `requirements.cmake`:

```cmake
# examples/cross/color_demo/requirements.cmake
set(EXAMPLE_REQUIRES_COLOR_DEPTH_MIN 16)
set(EXAMPLE_REQUIRES_WIDTH_MIN       240)
set(EXAMPLE_REQUIRES_HEIGHT_MIN      240)
```

Examples with no requirements file are compatible with all profiles. This avoids parsing comments and is trivially `include()`-able.

At configure time, the build system checks each example's `requirements.cmake` against the active profile:

```cmake
# In root or cross/ CMakeLists.txt, per-example:
if(EXISTS "${example_dir}/requirements.cmake")
    include("${example_dir}/requirements.cmake")
    if(DEFINED EXAMPLE_REQUIRES_COLOR_DEPTH_MIN AND EUI_COLOR_DEPTH LESS EXAMPLE_REQUIRES_COLOR_DEPTH_MIN)
        message(WARNING "Skipping ${example_name}: requires color depth >= ${EXAMPLE_REQUIRES_COLOR_DEPTH_MIN}")
        return()  # skip this example
    endif()
    if(DEFINED EXAMPLE_REQUIRES_WIDTH_MIN AND EUI_DISPLAY_WIDTH LESS EXAMPLE_REQUIRES_WIDTH_MIN)
        message(WARNING "Skipping ${example_name}: requires width >= ${EXAMPLE_REQUIRES_WIDTH_MIN}")
        return()
    endif()
endif()
```

## Directory Layout (Target State)

```
eui/
├── configs/                              # Hardware profiles (NEW)
│   ├── pc/
│   │   ├── raylib_128x64_1bpp.cmake
│   │   └── raylib_240x240_16bpp.cmake
│   ├── esp32/
│   │   ├── ssd1306_i2c_128x64_1bpp.cmake
│   │   ├── st7735_spi_240x240_16bpp.cmake
│   │   └── ili9341_spi_320x240_16bpp.cmake
│   └── nrf52/
│       ├── ssd1306_i2c_128x64_1bpp.cmake
│       └── st7735_spi_240x240_16bpp.cmake
│
├── examples/
│   ├── cross/                            # Migrated from legacy (all 11+ examples)
│   │   ├── basic_label/
│   │   │   ├── basic_label.c             # eui_example_setup() only
│   │   │   └── CMakeLists.txt
│   │   ├── button_test/
│   │   ├── list_nav/
│   │   ├── menu_system/
│   │   ├── dialog_overlay/
│   │   ├── animation_demo/
│   │   ├── custom_widget/
│   │   ├── page_buffer/
│   │   ├── scene_view_demo/
│   │   ├── benchmark/
│   │   ├── color_demo/
│   │   │   ├── color_demo.c
│   │   │   ├── CMakeLists.txt
│   │   │   └── requirements.cmake        # depth>=16, w>=240, h>=240
│   │   └── amiibo_demo/
│   │       ├── amiibo_demo.c
│   │       ├── CMakeLists.txt
│   │       └── requirements.cmake        # depth>=16, w>=240, h>=240
│   │
│   ├── esp-idf/                          # DEPRECATED (unified by config profiles)
│   ├── icons/                            # Shared assets (unchanged)
│   └── render_16bpp.c                    # Standalone offline tool (unchanged)
│
├── port/
│   ├── raylib/eui_port_bootstrap.c       # Reads config profile, creates drivers
│   ├── esp-idf/eui_port_bootstrap.c
│   └── nrf5/eui_port_bootstrap.c
│
└── include/eui/
    └── eui_port_bootstrap.h              # Bootstrap API (unchanged)
```

### Files to delete

| Path | Reason |
|------|--------|
| `examples/basic_label.c` through `examples/scene_view_demo.c` (10 files) | Migrated to `cross/` |
| `examples/amiibo_demo.c` | Migrated to `cross/amiibo_demo/` |
| `examples/amiibo_font.h` | Moved into `cross/amiibo_demo/` |
| `examples/esp-idf/ssd1306/` | Replaced by `configs/esp32/ssd1306_i2c_128x64_1bpp.cmake` + cross example |
| `examples/esp-idf/st7735/` | Replaced by `configs/esp32/st7735_spi_240x240_16bpp.cmake` + cross example |

### .cmake vs .Kconfig: two views of one profile

Each hardware profile has two representations:

| File | Purpose | Used by |
|------|---------|---------|
| `*.cmake` | Preset pin values, driver choice, depth. Read once at CMake configure time. | CI, quick desktop builds, anyone who knows their wiring |
| `*.Kconfig` | Interactive menuconfig tree for the same variables. User can adjust pins before building. | `idf.py menuconfig` on esp-idf, nRF Connect Kconfig on nrf5 |

Both files describe the same device. When building with esp-idf, the CMake profile sets defaults, then the user can run `menuconfig` to override pins. On PC (raylib), `.cmake` values are used directly — no menuconfig tool exists.

## Build Workflow

### Single build (one profile, all examples)

```bash
cmake -B build \
    -DEUI_CONFIG_PROFILE=configs/esp32/st7735_spi_240x240_16bpp.cmake
cmake --build build
```

This builds all compatible cross examples targeting the ST7735 on ESP32 at 16bpp. `color_demo` and `amiibo_demo` are included; others are included too (they'll render in 16bpp mode even if they don't need it).

### Multi-profile build (CI matrix)

```bash
# Build all examples for all profiles
for profile in configs/**/*.cmake; do
    name=$(basename $profile .cmake)
    cmake -B build_$name -DEUI_CONFIG_PROFILE=$profile
    cmake --build build_$name -j$(nproc)
done
```

### PC-only quick iteration

```bash
cmake -B build -DEUI_CONFIG_PROFILE=configs/pc/raylib_128x64_1bpp.cmake
cmake --build build -j$(nproc)
./build/examples/cross/basic_label/basic_label
```

## Color Depth Handling

### Config profile sets the depth

`EUI_COLOR_DEPTH` moves from a root CMake cache variable to a value set by the profile. Each profile specifies one depth. Building for ST7735 sets depth=16; building for SSD1306 sets depth=1.

### Multi-depth in one build

Not supported. A single CMake build tree has one `EUI_COLOR_DEPTH`. If you need to test both 1bpp and 16bpp, use two build directories with different profiles. This is the correct tradeoff: the framework config header is per-build, and different depths mean different `eui_config.h`.

### Example adaptation

Examples written for 1bpp should still work at 16bpp (the framework handles it — `eui_color_1bpp_to_native()` maps black/white to the native color). Conversely, a 16bpp example is simply not built when depth=1 (caught by `requirements.cmake`).

A 16bpp example can query depth at runtime to adapt:

```c
void eui_example_setup(const eui_example_config_t *cfg) {
    if (cfg->color_depth >= 16) {
        // Use RGB565 colors
    } else {
        // Monochrome fallback (or just exit early)
    }
}
```

## Migration Path

### Phase 1: Create config profiles (no example changes)

1. Create `configs/` directory with CMake profile files
2. Add `EUI_CONFIG_PROFILE` CMake variable; when set, skip `EUI_TARGET_PORT` and instead `include()` the profile
3. Profile sets legacy-compatible variables (`EUI_TARGET_PORT`, `EUI_COLOR_DEPTH`, etc.) so existing cross examples continue to build unchanged

### Phase 2: Migrate remaining legacy examples to cross/

1. Extract pure UI logic from each `examples/*.c` into `examples/cross/<name>/<name>.c`
2. Add `requirements.cmake` for depth/resolution constraints
3. Delete the original `examples/*.c` files and `examples/CMakeLists.txt` entries
4. Remove `EUI_BUILD_EXAMPLES` CMake option (only `EUI_BUILD_CROSS_EXAMPLES` remains)

### Phase 3: Move Kconfig out of example dirs

1. Create `configs/<platform>/*.Kconfig` files for each profile
2. Update port bootstraps to read Kconfig from config directory, not example directory
3. Remove Kconfig from individual example directories

### Phase 4: Deprecate platform-specific examples

1. Verify `configs/esp32/ssd1306_*.cmake` + `cross/basic_label` produces identical output to `examples/esp-idf/ssd1306/`
2. Remove `examples/esp-idf/` directory

## Scope Boundaries

**In scope:**
- `configs/` directory with CMake profile files for all current platforms and drivers
- `EUI_CONFIG_PROFILE` CMake variable to select a profile
- `requirements.cmake` per-example constraint declarations
- Build system filtering of incompatible example+profile combinations
- Migration of all 11+ legacy examples to `cross/`
- Migration of esp-idf examples to config profiles + cross examples
- Deletion of legacy example directories

**Out of scope:**
- Supporting multiple profiles in a single CMake build tree
- Auto-detecting available hardware at configure time
- Per-example custom Kconfig (beyond the shared hardware Kconfig)
- Bootstrap support for user-defined config profiles not in the `configs/` tree
- CI matrix generation scripts (use a simple shell loop)

## References

- Cross-platform bootstrap design: [2026-05-23-cross-platform-examples-design.md](2026-05-23-cross-platform-examples-design.md)
- HAL driver headers: `include/eui/driver/eui_drv_ssd1306.h`, `eui_drv_st7735.h`, etc.
- Framework config: `include/eui/eui_config.h.in`
- Root build: `CMakeLists.txt` (EUI_TARGET_PORT, EUI_BUILD_CROSS_EXAMPLES)
