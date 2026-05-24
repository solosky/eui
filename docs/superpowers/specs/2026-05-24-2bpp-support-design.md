# 2bpp (2 Bits Per Pixel) Support for EUI

**Date:** 2026-05-24
**Status:** Design (pre-implementation)

## Overview

Add compile-time 2bpp color depth support to EUI, enabling 4-level grayscale rendering for displays and memory-constrained targets.

## Pixel Format

- **4-level grayscale:** 0 = black, 1 = dark gray, 2 = light gray, 3 = white
- **Packing:** 4 pixels per byte, MSB-first (consistent with existing 1bpp convention)
  - Pixel 0 → bits [7:6], Pixel 1 → bits [5:4], Pixel 2 → bits [3:2], Pixel 3 → bits [1:0]
  - Byte index: `(y * (width / 4)) + (x / 4)`
  - Shift: `6 - 2 * (x % 4)`
- **`eui_color_t`:** `uint8_t` (values 0-3)
- **`EUI_COLOR_WHITE`:** 3 (max 2-bit value)

## Configuration

- `CMakeLists.txt`: Accept `EUI_COLOR_DEPTH 2` in cache string
- `eui_config.h.in`: Update `#error` to list `2`
- `test/config_2bpp.h`: Test override config (like existing `config_16bpp.h`)

## Color Conversion (`eui_types.c`)

- `eui_color_from_rgb()`: Weighted grayscale → `gray * 3 / 255`
- `eui_color_from_gray()`: `gray * 3 / 255`

## Canvas Changes (`eui_canvas.c`)

### `canvas_set_pixel()`
New `#elif EUI_COLOR_DEPTH == 2` branch: pack 2-bit value into MSB-aligned nibble pair within byte.

### `canvas_buf_size()`
`pixels / 4` — each byte holds 4 pixels.

### `eui_canvas_clear()`, `begin_page()`, `next_page()`
New `#elif EUI_COLOR_DEPTH == 2` branch. Replicate `bg_color` (0-3) across each 2-bit slot within the fill byte:
```c
uint8_t fill = (uint8_t)(canvas->bg_color & 3u);
fill |= fill << 2;
fill |= fill << 4;
fill |= fill << 6;
memset(canvas->buffer, fill, size);
```
For bg_color=0: fill=0x00. bg_color=3: fill=0xFF. bg_color=1: fill=0x55. bg_color=2: fill=0xAA.

### `eui_canvas_invert_rect()`
New branch: each 2-bit pixel XOR'd with `0x03` (toggle black ↔ white).

### `eui_canvas_draw_bitmap()`
Add `depth == 2` case in pixel-reading loop: extract 2-bit values from packed source data.

## Font Engine

- `canvas_set_pixel()` handles packing → glyph rendering via `draw_glyph()` works transparently
- Raw-buffer `draw_char()` APIs (BDF, VLW, U8G2): add 2bpp branches using same MSB-first packing

## Display Drivers

### Raylib (`eui_drv_raylib.c`)
New `color_depth == 2` branch in `write_buffer()`:
- 0 → (0, 0, 0), 1 → (85, 85, 85), 2 → (170, 170, 170), 3 → (255, 255, 255)

### Web (`eui_drv_web.c`)
New `color_depth == 2` branch → call appropriate JS render function.

### Built-in drivers (SSD1306, ST7735, etc.)
Unchanged — they hardcode their native `color_depth`. A future 2bpp display driver would set `caps.color_depth = 2`.

## Testing

- `test/config_2bpp.h`: Predefine `EUI_COLOR_DEPTH 2` with appropriate pool size
- Test cases: set_pixel readback, clear, invert_rect, draw_bitmap with 2bpp source, color conversion (0-3 range), buffer size calculation
- Test CMake: add 2bpp config test target (or reuse existing multi-config test pattern)

## Files Modified

| File | Change |
|------|--------|
| `CMakeLists.txt` | Accept `2` in depth list |
| `include/eui/eui_config.h.in` | Update `#error` |
| `include/eui/eui_types.h` | `eui_color_t` case, `EUI_COLOR_WHITE` case |
| `src/eui_types.c` | `from_rgb` / `from_gray` 2bpp branches |
| `src/eui_canvas.c` | `set_pixel`, `buf_size`, `invert_rect`, `draw_bitmap` |
| `src/eui_font_bdf.c` | `draw_char` 2bpp |
| `src/eui_font_vlw.c` | `draw_char` 2bpp |
| `src/eui_font_u8g2.c` | `draw_glyph` / `rle_decode` 2bpp |
| `src/driver/eui_drv_raylib.c` | `write_buffer` 2bpp |
| `src/driver/eui_drv_web.c` | `write_buffer` 2bpp |
| `test/config_2bpp.h` | New test config |
| `test/` | 2bpp test cases |
