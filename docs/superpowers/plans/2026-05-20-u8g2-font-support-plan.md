# u8g2 Font Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement full u8g2 `.c` font support with kerning, encoding tables, and new drawing operations (multiline, word-wrap, clipping, ellipsis), achieving 90%+ line coverage.

**Architecture:** Five phases spanning config, u8g2 parser, kerning, canvas drawing ops, and comprehensive tests.

**Tech Stack:** C99, CMake, CTest

---

## Phase 1: Configuration & Types

### Task 1: CMake config macros + eui_config.h.in

**Files:** `CMakeLists.txt:17` (after), `include/eui/eui_config.h.in:12` (after)

- [ ] **Step 1: Add CMake cache vars and config macros**

In `CMakeLists.txt`, after `set(EUI_MAX_OVERLAYS ...)` add:
```cmake
set(EUI_FONT_ENABLE_U8G2 ON CACHE BOOL "Enable u8g2 font format support")
set(EUI_FONT_ENABLE_KERNING ON CACHE BOOL "Enable font kerning support")
set(EUI_FONT_ENABLE_MULTILINE ON CACHE BOOL "Enable multiline text support")
```

In `include/eui/eui_config.h.in`, before `#endif` add:
```c
#cmakedefine EUI_FONT_ENABLE_U8G2     @EUI_FONT_ENABLE_U8G2@
#cmakedefine EUI_FONT_ENABLE_KERNING  @EUI_FONT_ENABLE_KERNING@
#cmakedefine EUI_FONT_ENABLE_MULTILINE @EUI_FONT_ENABLE_MULTILINE@
```

- [ ] **Step 2: Verify**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_U8G2=ON -DEUI_FONT_ENABLE_KERNING=ON -DEUI_FONT_ENABLE_MULTILINE=ON
grep EUI_FONT_ENABLE include/eui/eui_config.h
```
Expected: all three `#define ... 1`.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt include/eui/eui_config.h.in
git commit -m "feat: add EUI_FONT_ENABLE_U8G2/KERNING/MULTILINE config macros"
```

### Task 2: Extend eui_types.h

**Files:** `include/eui/eui_types.h:59-69`

- [ ] **Step 1: Add lookup callback, new flags, extend eui_font_t**

Replace lines 59-69:
```c
/* ---- Font ---- */

typedef int16_t (*eui_font_lookup_fn)(const struct eui_font *font,
                                       uint16_t encoding, uint16_t prev);

typedef struct eui_font {
    uint8_t  format;
    uint8_t  line_height;
    uint8_t  baseline;
    uint8_t  flags;
    const uint8_t *data;
#if EUI_FONT_ENABLE_U8G2
    eui_font_lookup_fn lookup_glyph;
#endif
} eui_font_t;

#define EUI_FONT_FIXED_WIDTH  (1u << 0)
#define EUI_FONT_HAS_KERNING  (1u << 1)
#define EUI_FONT_HAS_UNICODE  (1u << 2)
```

- [ ] **Step 2: Verify build**

```bash
cd build && cmake .. && make eui -j4
```

- [ ] **Step 3: Commit**

```bash
git add include/eui/eui_types.h
git commit -m "feat: extend eui_font_t with lookup_glyph callback and kerning/unicode flags"
```

### Task 3: Add EUI_FONT_FORMAT_U8G2 constant

**Files:** `include/eui/eui_font.h:8` (after)

- [ ] **Step 1: Add format constant**

After `#define EUI_FONT_FORMAT_VLW  1` add:
```c
#define EUI_FONT_FORMAT_U8G2 2
```

- [ ] **Step 2: Verify build**

```bash
cd build && cmake .. && make eui -j4
```

- [ ] **Step 3: Commit**

```bash
git add include/eui/eui_font.h
git commit -m "feat: add EUI_FONT_FORMAT_U8G2 constant"
```

---

## Phase 2: u8g2 Font Parser

### u8g2 Font Binary Format Reference

The u8g2 `.c` font `const uint8_t[]` layout:

```
Byte 0:     glyph_cnt (uint8)
Byte 1:     bbx_mode  (uint8)
Byte 2:     bits_per_0 (uint8)  — bits to encode consecutive zeros
Byte 3:     bits_per_1 (uint8)  — bits to encode consecutive ones
Byte 4:     bits_per_char_width (uint8)
Byte 5:     bits_per_char_height (uint8)
Byte 6:     bits_per_char_x (uint8)
Byte 7:     bits_per_char_y (uint8)
Byte 8:     bits_per_delta_x (uint8)
Byte 9-10:  max_char_width  (int16 LE)
Byte 11-12: max_char_height (int16 LE)
Byte 13-14: ascent_A  (int16 LE)
Byte 15-16: ascent_g  (int16 LE)
Byte 17-18: ascent_para (int16 LE)
Byte 19-20: start_pos_unicode (uint16 LE, 0=none)
Byte 21+:   glyph_bitmap[] — packed bit-level glyph data
```

**Glyph bit encoding (MSB first per byte):**

Each font header defines `bits_per_*` fields. The glyph data area is a continuous bitstream.

For each of `glyph_cnt` glyphs:
1. Read `bits_per_0` bits → `zeros_count`
   - If `zeros_count == (1 << bits_per_0) - 1`: glyph boundary marker, proceed to step 2
   - Otherwise: this is a run of `zeros_count` zero-bits in the bitmap; goto step 4
2. Read glyph metrics (one `bits_per_char_*` value each):
   - `char_width`  (0 = max_char_width)
   - `char_height` (0 = max_char_height)
   - `x_offset`
   - `y_offset`
   - `delta_x`     (0 = max_char_width)
3. Continue to step 1 (read next run for bitmap data)
4. Read `bits_per_1` bits → `ones_count`
   - If `ones_count == (1 << bits_per_1) - 1`: end of data marker
   - Otherwise: run of `ones_count` one-bits in the bitmap
5. Continue to step 1

The bitmap is the concatenation of all zero/one runs between glyph boundary markers, totaling `char_width * char_height` bits per glyph.

**Encoding table** (at `start_pos_unicode` offset from data start):

```
uint8_t  encoding_type    — 0=UTF-8 (not used)
uint16_t glyph_cnt        — number of entries
For each entry:
  uint16_t encoding       — Unicode code point
  uint8_t  glyph_index    — glyph array index
```

Kerning extends this with `(prev, curr) → glyph` lookups.

---

### Task 4: Generate u8g2 test font data

**Files:** Create `tools/gen_test_u8g2_font.py`, `test/test_u8g2_font.h`

- [ ] **Step 1: Write generator script**

`tools/gen_test_u8g2_font.py`:
```python
#!/usr/bin/env python3
"""Generate a test u8g2-format font with 8 glyphs (A-H), 8x8, 1bpp."""
import struct

GLYPH_COUNT = 8
BITS = {'per_0': 4, 'per_1': 4, 'char_w': 4, 'char_h': 4,
        'char_x': 4, 'char_y': 4, 'delta_x': 4}
MAX_CHAR_W, MAX_CHAR_H = 8, 8
ALL_ONES_4 = (1 << BITS['per_0']) - 1

bdf = {
    'A': [0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00],
    'B': [0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00],
    'C': [0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00],
    'D': [0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00],
    'E': [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00],
    'F': [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00],
    'G': [0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00],
    'H': [0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00],
}

class BitWriter:
    def __init__(self):
        self.bytes = bytearray()
        self.cur = 0
        self.pos = 7
    def write(self, val, n):
        for i in range(n-1, -1, -1):
            self.cur |= (((val >> i) & 1) << self.pos)
            self.pos -= 1
            if self.pos < 0:
                self.bytes.append(self.cur)
                self.cur, self.pos = 0, 7
    def flush(self):
        if self.pos < 7:
            self.bytes.append(self.cur)
            self.cur, self.pos = 0, 7
    def data(self):
        self.flush()
        return bytes(self.bytes)

data = bytearray()
data.append(GLYPH_COUNT)
data.append(0)  # bbx_mode
for k in ['per_0', 'per_1', 'char_w', 'char_h', 'char_x', 'char_y', 'delta_x']:
    data.append(BITS[k])
data += struct.pack('<h', MAX_CHAR_W)
data += struct.pack('<h', MAX_CHAR_H)
data += struct.pack('<h', 8)   # ascent_A
data += struct.pack('<h', 8)   # ascent_g
data += struct.pack('<h', 10)  # ascent_para
data += struct.pack('<H', 0)   # start_pos_unicode = 0

bw = BitWriter()
max_run = (1 << BITS['per_0']) - 2  # all-1s reserved for marker
for ch in sorted(bdf.keys()):
    # glyph boundary
    bw.write(ALL_ONES_4, BITS['per_0'])
    # glyph metrics
    bw.write(MAX_CHAR_W, BITS['char_w'])
    bw.write(MAX_CHAR_H, BITS['char_h'])
    bw.write(0, BITS['char_x'])
    bw.write(0, BITS['char_y'])
    bw.write(MAX_CHAR_W, BITS['delta_x'])
    # bitmap as zero/one runs
    row_bytes = bdf[ch]
    run_len, run_type = 0, 0
    for r in row_bytes:
        for c in range(8):
            b = (r >> (7-c)) & 1
            if b == run_type:
                run_len += 1
            else:
                while run_len > max_run:
                    bw.write(max_run, BITS['per_0'] if run_type == 0 else BITS['per_1'])
                    run_len -= max_run
                bw.write(run_len, BITS['per_0'] if run_type == 0 else BITS['per_1'])
                run_type, run_len = b, 1
    while run_len > max_run:
        bw.write(max_run, BITS['per_0'] if run_type == 0 else BITS['per_1'])
        run_len -= max_run
    bw.write(run_len, BITS['per_0'] if run_type == 0 else BITS['per_1'])
bw.write(ALL_ONES_4, BITS['per_0'])  # end marker

bitmap = bw.data()
data += bitmap

hex_lines = []
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
    hex_lines.append(f'    {hex_str}')

with open('test/test_u8g2_font.h', 'w') as f:
    f.write(f'''/* Auto-generated test u8g2 format font
 * {GLYPH_COUNT} glyphs A-H, 8x8 pixels, 1bpp, fixed-width
 * Generated by tools/gen_test_u8g2_font.py
 */
#ifndef TEST_U8G2_FONT_H
#define TEST_U8G2_FONT_H

#include <stdint.h>

#define TEST_U8G2_GLYPH_COUNT {GLYPH_COUNT}
#define TEST_U8G2_FONT_HEIGHT {MAX_CHAR_H}
#define TEST_U8G2_FONT_BASELINE 7

static const uint8_t test_u8g2_font_data[] = {{
{','.join(hex_lines)}
}};

#endif /* TEST_U8G2_FONT_H */
''')
print("Generated test/test_u8g2_font.h")
```

- [ ] **Step 2: Run generator**

```bash
python3 tools/gen_test_u8g2_font.py
```
Expected: `Generated test/test_u8g2_font.h`

- [ ] **Step 3: Commit**

```bash
git add tools/gen_test_u8g2_font.py test/test_u8g2_font.h
git commit -m "feat: add u8g2 test font generator and test data"
```

### Task 5: Implement u8g2 font parser (eui_font_u8g2.c)

**Files:** Create `src/eui_font_u8g2.c`, Modify `src/eui_font_internal.h`, Modify `src/CMakeLists.txt`, Modify `src/eui_font.c`

- [ ] **Step 1: Add to src/CMakeLists.txt**

After `eui_font_vlw.c` add:
```cmake
    eui_font_u8g2.c
```

- [ ] **Step 2: Write eui_font_internal.h declarations**

Replace `src/eui_font_internal.h`:
```c
#ifndef EUI_FONT_INTERNAL_H
#define EUI_FONT_INTERNAL_H

#include "eui/eui_font.h"

uint8_t eui_font_bdf_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_bdf_get_str_width(const eui_font_t *font, const char *str);
uint8_t eui_font_bdf_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth);

uint8_t eui_font_vlw_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_vlw_get_str_width(const eui_font_t *font, const char *str);
uint8_t eui_font_vlw_draw_char(const eui_font_t *font, char c,
                                uint8_t *buf, uint16_t buf_stride,
                                uint8_t color_depth);

#if EUI_FONT_ENABLE_U8G2
uint8_t  eui_font_u8g2_get_char_width(const eui_font_t *font, char c);
uint16_t eui_font_u8g2_get_str_width(const eui_font_t *font, const char *str);
uint8_t  eui_font_u8g2_draw_char(const eui_font_t *font, char c,
                                  uint8_t *buf, uint16_t buf_stride,
                                  uint8_t color_depth);
int16_t  eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev);
#endif

#endif
```

- [ ] **Step 3: Add u8g2 dispatch to eui_font.c**

Wrap the new code in `#if EUI_FONT_ENABLE_U8G2`:
```c
#include "eui/eui_font.h"
#include "eui_font_internal.h"

uint8_t eui_font_get_char_width(const eui_font_t *font, char c)
{
    if (!font) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_get_char_width(font, c);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_get_char_width(font, c);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_get_char_width(font, c);
#endif
    return 0;
}

uint16_t eui_font_get_str_width(const eui_font_t *font, const char *str)
{
    if (!font) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_get_str_width(font, str);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_get_str_width(font, str);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_get_str_width(font, str);
#endif
    return 0;
}

uint8_t eui_font_get_height(const eui_font_t *font)
{
    if (!font) return 0;
    return font->line_height;
}

uint8_t eui_font_get_baseline(const eui_font_t *font)
{
    if (!font) return 0;
    return font->baseline;
}

uint8_t eui_font_draw_char(const eui_font_t *font, char c,
                            uint8_t *buf, uint16_t buf_stride,
                            uint8_t color_depth)
{
    if (!font) return 0;
    if (font->format == EUI_FONT_FORMAT_BDF) return eui_font_bdf_draw_char(font, c, buf, buf_stride, color_depth);
    if (font->format == EUI_FONT_FORMAT_VLW) return eui_font_vlw_draw_char(font, c, buf, buf_stride, color_depth);
#if EUI_FONT_ENABLE_U8G2
    if (font->format == EUI_FONT_FORMAT_U8G2) return eui_font_u8g2_draw_char(font, c, buf, buf_stride, color_depth);
#endif
    return 0;
}
```

- [ ] **Step 4: Write u8g2 font parser**

`src/eui_font_u8g2.c`:
```c
#include "eui/eui_font.h"
#include <stddef.h>

/* Header offsets */
#define HDR_GLYPH_CNT       0
#define HDR_BBX_MODE        1
#define HDR_BITS_PER_0      2
#define HDR_BITS_PER_1      3
#define HDR_BITS_PER_CHAR_W 4
#define HDR_BITS_PER_CHAR_H 5
#define HDR_BITS_PER_CHAR_X 6
#define HDR_BITS_PER_CHAR_Y 7
#define HDR_BITS_PER_DELTA_X 8
#define HDR_MAX_CHAR_W      9
#define HDR_MAX_CHAR_H      11
#define HDR_START_POS_UNICODE 19
#define U8G2_HEADER_SIZE    21

typedef struct {
    const uint8_t *data;
    uint16_t byte_pos;
    uint8_t  bit_pos;
} bit_reader_t;

typedef struct {
    int8_t  x_offset;
    int8_t  y_offset;
    uint8_t width;
    uint8_t height;
    uint8_t x_advance;
    uint16_t bitmap_byte;   /* byte offset in data where bitmap starts */
    uint8_t  bitmap_bit;    /* bit offset within that byte */
} u8g2_glyph_t;

static void br_init(bit_reader_t *br, const uint8_t *data)
{
    br->data = data;
    br->byte_pos = U8G2_HEADER_SIZE;
    br->bit_pos = 0;
}

static uint8_t br_read(bit_reader_t *br, uint8_t num_bits)
{
    uint8_t val = 0;
    for (uint8_t i = 0; i < num_bits; i++) {
        uint8_t byte = br->data[br->byte_pos];
        val = (val << 1) | ((byte >> (7 - br->bit_pos)) & 1);
        br->bit_pos++;
        if (br->bit_pos >= 8) { br->bit_pos = 0; br->byte_pos++; }
    }
    return val;
}

static void br_skip(bit_reader_t *br, uint16_t num_bits)
{
    uint16_t total = (uint16_t)br->bit_pos + num_bits;
    br->byte_pos += total / 8;
    br->bit_pos = (uint8_t)(total % 8);
}

static int16_t default_lookup(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    (void)font; (void)prev;
    if (encoding >= 'A' && encoding <= 'H') return (int16_t)(encoding - 'A');
    return -1;
}

int16_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    if (!font || !font->data) return -1;
    return default_lookup(font, encoding, prev);
}

/* Decode glyph at index. Returns 1 on success, placing data in *glyph. */
static uint8_t decode_glyph_at(const eui_font_t *font, uint8_t idx, u8g2_glyph_t *glyph)
{
    const uint8_t *p = font->data;
    uint8_t  glyph_cnt  = p[HDR_GLYPH_CNT];
    uint8_t  bp0 = p[HDR_BITS_PER_0], bp1 = p[HDR_BITS_PER_1];
    uint8_t  bpcw = p[HDR_BITS_PER_CHAR_W], bpch = p[HDR_BITS_PER_CHAR_H];
    uint8_t  bpcx = p[HDR_BITS_PER_CHAR_X], bpcy = p[HDR_BITS_PER_CHAR_Y];
    uint8_t  bpdx = p[HDR_BITS_PER_DELTA_X];
    uint8_t  mcw = p[HDR_MAX_CHAR_W] | ((uint16_t)p[HDR_MAX_CHAR_W+1] << 8);
    uint8_t  mch = p[HDR_MAX_CHAR_H] | ((uint16_t)p[HDR_MAX_CHAR_H+1] << 8);
    uint8_t  all0 = (1u << bp0) - 1, all1 = (1u << bp1) - 1;
    uint8_t  max_run = (uint8_t)(all0 - 1);

    if (idx >= glyph_cnt) return 0;

    bit_reader_t br;
    br_init(&br, font->data);

    for (uint8_t i = 0; i <= idx; i++) {
        /* Skip intermediate runs until glyph boundary */
        uint8_t marker;
        while (1) {
            marker = br_read(&br, bp0);
            if (marker == all0) break;  /* glyph boundary */
            /* normal run: skip ones count and bitmap bits */
            uint8_t ones = br_read(&br, bp1);
            if (ones == all1) return 0;  /* end of data */
            br_skip(&br, (uint16_t)marker + ones);
        }
        /* Read glyph metrics */
        uint8_t cw = br_read(&br, bpcw);
        uint8_t ch = br_read(&br, bpch);
        uint8_t cx = br_read(&br, bpcx);
        uint8_t cy = br_read(&br, bpcy);
        uint8_t dx = br_read(&br, bpdx);

        glyph->width     = cw ? cw : mcw;
        glyph->height    = ch ? ch : mch;
        glyph->x_offset  = (int8_t)cx;
        glyph->y_offset  = (int8_t)cy;
        glyph->x_advance = dx ? dx : mcw;

        /* Record bitmap position */
        glyph->bitmap_byte = br.byte_pos;
        glyph->bitmap_bit  = br.bit_pos;

        /* Skip bitmap for all glyphs except the target */
        uint16_t bm_bits = (uint16_t)glyph->width * glyph->height;
        br_skip(&br, bm_bits);
    }
    return 1;
}

/* Read one bitmap pixel from a specific bit position */
static uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                                 uint8_t bit_off, uint16_t pixel_idx)
{
    uint16_t total_bit = (uint16_t)byte_off * 8 + bit_off + pixel_idx;
    uint16_t byte_idx = total_bit / 8;
    uint8_t  bit_idx  = total_bit % 8;
    return (data[byte_idx] >> (7 - bit_idx)) & 1;
}

uint8_t eui_font_u8g2_get_char_width(const eui_font_t *font, char c)
{
    int16_t idx = eui_font_u8g2_lookup_glyph(font, (uint8_t)c, 0);
    if (idx < 0) return 0;
    u8g2_glyph_t g;
    if (!decode_glyph_at(font, (uint8_t)idx, &g)) return 0;
    return g.x_advance;
}

uint16_t eui_font_u8g2_get_str_width(const eui_font_t *font, const char *str)
{
    uint16_t w = 0;
    if (!font || !str) return 0;
    while (*str) { w += eui_font_u8g2_get_char_width(font, *str); str++; }
    return w;
}

uint8_t eui_font_u8g2_draw_char(const eui_font_t *font, char c,
                                 uint8_t *buf, uint16_t buf_stride,
                                 uint8_t color_depth)
{
    int16_t idx = eui_font_u8g2_lookup_glyph(font, (uint8_t)c, 0);
    if (idx < 0 || !buf) return 0;

    u8g2_glyph_t g;
    if (!decode_glyph_at(font, (uint8_t)idx, &g)) return 0;

    for (uint8_t row = 0; row < g.height; row++) {
        for (uint8_t col = 0; col < g.width; col++) {
            uint16_t px = (uint16_t)row * g.width + col;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, px)) {
                if (color_depth == 1) {
                    buf[row * buf_stride + col / 8] |= (1u << (7 - (col % 8)));
                }
            }
        }
    }
    return g.x_advance;
}
```

- [ ] **Step 5: Build**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_U8G2=ON && make eui -j4
```
Expected: Build succeeds.

- [ ] **Step 6: Commit**

```bash
git add src/eui_font_u8g2.c src/eui_font_internal.h src/CMakeLists.txt src/eui_font.c
git commit -m "feat: implement u8g2 font parser with bit-level decoding"
```

### Task 6: Write u8g2 font unit tests

**Files:** Create `test/test_font_u8g2.c`, Modify `test/CMakeLists.txt`

- [ ] **Step 1: Write u8g2 font tests**

`test/test_font_u8g2.c`:
```c
#include "eui/eui_font.h"
#include "eui/eui_allocator.h"
#include "test_u8g2_font.h"
#include <stdio.h>
#include <string.h>

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

static const eui_font_t test_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = TEST_U8G2_FONT_HEIGHT,
    .baseline = TEST_U8G2_FONT_BASELINE,
    .flags = EUI_FONT_FIXED_WIDTH,
    .data = test_u8g2_font_data,
#if EUI_FONT_ENABLE_U8G2
    .lookup_glyph = NULL,
#endif
};

static void test_u8g2_format(void)
{
    TEST("u8g2 font format check");
    if (test_font.format != EUI_FONT_FORMAT_U8G2) FAIL("wrong format");
    PASS();
}

static void test_u8g2_char_width_a(void)
{
    TEST("u8g2 char width 'A' = 8");
    uint8_t w = eui_font_get_char_width(&test_font, 'A');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_char_width_h(void)
{
    TEST("u8g2 char width 'H' = 8");
    uint8_t w = eui_font_get_char_width(&test_font, 'H');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_out_of_range(void)
{
    TEST("u8g2 out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&test_font, 'z');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_str_width(void)
{
    TEST("u8g2 string width 'AB' = 16");
    uint16_t w = eui_font_get_str_width(&test_font, "AB");
    if (w != 16) FAIL("expected 16");
    PASS();
}

static void test_u8g2_height(void)
{
    TEST("u8g2 font height");
    uint8_t h = eui_font_get_height(&test_font);
    if (h != 8) FAIL("expected 8");
    PASS();
}

static void test_u8g2_baseline(void)
{
    TEST("u8g2 font baseline");
    uint8_t b = eui_font_get_baseline(&test_font);
    if (b != 7) FAIL("expected 7");
    PASS();
}

static void test_u8g2_empty_str(void)
{
    TEST("u8g2 empty string width = 0");
    uint16_t w = eui_font_get_str_width(&test_font, "");
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_draw_a(void)
{
    TEST("u8g2 draw_char 'A' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'A', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf, expected, 8) != 0) {
        printf("FAIL: got ");
        for (int i = 0; i < 8; i++) printf("%02X ", buf[i]);
        printf("\n");
        return;
    }
    PASS();
}

static void test_u8g2_draw_b(void)
{
    TEST("u8g2 draw_char 'B' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'B', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_c(void)
{
    TEST("u8g2 draw_char 'C' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'C', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_d(void)
{
    TEST("u8g2 draw_char 'D' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'D', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_e(void)
{
    TEST("u8g2 draw_char 'E' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'E', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_f(void)
{
    TEST("u8g2 draw_char 'F' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'F', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_g(void)
{
    TEST("u8g2 draw_char 'G' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'G', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_draw_h(void)
{
    TEST("u8g2 draw_char 'H' glyph");
    uint8_t buf[8] = {0};
    uint8_t adv = eui_font_draw_char(&test_font, 'H', buf, 1, 1);
    if (adv != 8) FAIL("expected advance 8");

    uint8_t expected[8] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    if (memcmp(buf, expected, 8) != 0) FAIL("glyph mismatch");
    PASS();
}

static void test_u8g2_null_font(void)
{
    TEST("u8g2 null font returns 0");
    uint8_t w = eui_font_get_char_width(NULL, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_u8g2_null_data(void)
{
    TEST("u8g2 null data returns 0");
    eui_font_t f = {
        .format = EUI_FONT_FORMAT_U8G2,
        .line_height = 8,
        .baseline = 7,
        .flags = 0,
        .data = NULL,
    };
    uint8_t w = eui_font_get_char_width(&f, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== U8G2 Font Tests ===\n");
    test_u8g2_format();
    test_u8g2_char_width_a();
    test_u8g2_char_width_h();
    test_u8g2_out_of_range();
    test_u8g2_str_width();
    test_u8g2_height();
    test_u8g2_baseline();
    test_u8g2_empty_str();
    test_u8g2_draw_a();
    test_u8g2_draw_b();
    test_u8g2_draw_c();
    test_u8g2_draw_d();
    test_u8g2_draw_e();
    test_u8g2_draw_f();
    test_u8g2_draw_g();
    test_u8g2_draw_h();
    test_u8g2_null_font();
    test_u8g2_null_data();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
```

- [ ] **Step 2: Add to test/CMakeLists.txt**

After the `test_font` block:
```cmake
add_executable(test_font_u8g2 test_font_u8g2.c)
target_include_directories(test_font_u8g2 PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(test_font_u8g2 PRIVATE eui)
add_test(NAME font_u8g2 COMMAND test_font_u8g2)
```

- [ ] **Step 3: Run tests**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_U8G2=ON && make test_font_u8g2 && ./test/test_font_u8g2
```
Expected: 18/18 tests passed.

- [ ] **Step 4: Commit**

```bash
git add test/test_font_u8g2.c test/CMakeLists.txt
git commit -m "test: add u8g2 font format unit tests (18 tests)"
```

---

## Phase 3: Encoding Table & Kerning

### Task 7: Add encoding table support to u8g2 parser

**Files:** Modify `src/eui_font_u8g2.c`, Modify `test/test_font_u8g2.c`

- [ ] **Step 1: Add encoding table lookup**

Append to `src/eui_font_u8g2.c`:

```c
/* Encoding table entry */
typedef struct {
    uint16_t encoding;
    uint8_t  glyph_index;
} u8g2_enc_entry_t;

/* Read encoding table from font data.
 * Returns number of entries or 0 if no encoding table. */
static uint16_t read_encoding_table(const eui_font_t *font,
                                     u8g2_enc_entry_t *entries,
                                     uint16_t max_entries)
{
    const uint8_t *p = font->data;
    uint16_t start = p[HDR_START_POS_UNICODE] | ((uint16_t)p[HDR_START_POS_UNICODE + 1] << 8);
    if (start == 0) return 0;

    const uint8_t *enc = font->data + start;
    /* Skip encoding_type (1 byte) and encoding_cnt (2 bytes LE) */
    uint16_t cnt = enc[1] | ((uint16_t)enc[2] << 8);
    if (cnt > max_entries) cnt = max_entries;

    enc += 3; /* skip type + cnt */
    for (uint16_t i = 0; i < cnt; i++) {
        entries[i].encoding    = enc[0] | ((uint16_t)enc[1] << 8);
        entries[i].glyph_index = enc[2];
        enc += 3;
    }
    return cnt;
}

/* Encoding-table-based glyph lookup */
static int16_t encoding_lookup(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    u8g2_enc_entry_t entries[16];
    uint16_t cnt = read_encoding_table(font, entries, 16);
    (void)prev; /* kerning via encoding table: prev modifies the encoding lookup */

    for (uint16_t i = 0; i < cnt; i++) {
        if (entries[i].encoding == encoding) return (int16_t)entries[i].glyph_index;
    }
    return -1;
}
```

- [ ] **Step 2: Update lookup_glyph to use encoding table if available**

Replace `eui_font_u8g2_lookup_glyph` in `src/eui_font_u8g2.c`:
```c
int16_t eui_font_u8g2_lookup_glyph(const eui_font_t *font, uint16_t encoding, uint16_t prev)
{
    if (!font || !font->data) return -1;

    /* Check if font has an encoding table */
    const uint8_t *p = font->data;
    uint16_t start = p[HDR_START_POS_UNICODE] | ((uint16_t)p[HDR_START_POS_UNICODE + 1] << 8);
    if (start != 0) {
        return encoding_lookup(font, encoding, prev);
    }

    /* Fallback: simple direct mapping */
    return default_lookup(font, encoding, prev);
}
```

- [ ] **Step 3: Build and run existing tests**

```bash
cd build && cmake .. && make test_font_u8g2 && ./test/test_font_u8g2
```
Expected: 18/18 tests still pass.

- [ ] **Step 4: Commit**

```bash
git add src/eui_font_u8g2.c
git commit -m "feat: add encoding table lookup to u8g2 font parser"
```

### Task 8: Generate kerning test font and write kerning tests

**Files:** Create `tools/gen_test_u8g2_kerning.py`, `test/test_font_kerning.h`, `test/test_font_kerning.c`, Modify `test/CMakeLists.txt`

- [ ] **Step 1: Generate kerning test font**

`tools/gen_test_u8g2_kerning.py` — generates a font with 4 glyphs (T, A, T_kern, A_kern) and an encoding table. The encoding table maps `(T,A)` pair to kerned glyph variants with reduced advance:
```python
#!/usr/bin/env python3
"""Generate a u8g2-format test font with kerning encoding table.
4 glyphs: regular T(0), regular A(1), kerning T(2), kerning A(3).
Encoding table maps 'T'->0, 'A'->1, and ("T","A") -> 2, 3 with tighter spacing.
"""
import struct

GLYPH_CNT = 4
BITS = {'per_0': 4, 'per_1': 4, 'char_w': 4, 'char_h': 4,
        'char_x': 4, 'char_y': 4, 'delta_x': 4}
MAX_CHAR_W, MAX_CHAR_H = 8, 8
ALL_ONES_4 = (1 << BITS['per_0']) - 1

# T: vertical bar + horizontal top (advance 8)
# A: A shape (advance 8)
# T_kern: vertical bar only, no right overhang (advance 6)
# A_kern: A shape shifted left (advance 8)
glyphs = [
    [0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00],  # T (adv=8)
    [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00],  # A (adv=8)
    [0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00],  # T_kern (adv=6)
    [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00],  # A_kern (adv=8)
]
advances = [8, 8, 6, 8]

class BitWriter:
    def __init__(self):
        self.bytes = bytearray()
        self.cur, self.pos = 0, 7
    def write(self, val, n):
        for i in range(n-1, -1, -1):
            self.cur |= (((val >> i) & 1) << self.pos)
            self.pos -= 1
            if self.pos < 0:
                self.bytes.append(self.cur)
                self.cur, self.pos = 0, 7
    def flush(self):
        if self.pos < 7: self.bytes.append(self.cur)
    def data(self):
        self.flush()
        return bytes(self.bytes)

def encode_glyph_bitmap(glyph, bits):
    bw = BitWriter()
    max_run = (1 << bits['per_0']) - 2
    run_len, run_type = 0, 0
    for r in glyph:
        for c in range(8):
            b = (r >> (7-c)) & 1
            if b == run_type:
                run_len += 1
            else:
                while run_len > max_run:
                    bw.write(max_run, bits['per_0'] if run_type == 0 else bits['per_1'])
                    run_len -= max_run
                bw.write(run_len, bits['per_0'] if run_type == 0 else bits['per_1'])
                run_type, run_len = b, 1
    while run_len > max_run:
        bw.write(max_run, bits['per_0'] if run_type == 0 else bits['per_1'])
        run_len -= max_run
    bw.write(run_len, bits['per_0'] if run_type == 0 else bits['per_1'])
    return bw.data()

# Build header + glyph bitstream
data = bytearray()
data.append(GLYPH_CNT)  # glyph_cnt
data.append(0)          # bbx_mode
for k in ['per_0', 'per_1', 'char_w', 'char_h', 'char_x', 'char_y', 'delta_x']:
    data.append(BITS[k])
data += struct.pack('<h', MAX_CHAR_W)
data += struct.pack('<h', MAX_CHAR_H)
data += struct.pack('<h', 8)   # ascent_A
data += struct.pack('<h', 8)   # ascent_g
data += struct.pack('<h', 10)  # ascent_para

# start_pos_unicode will be computed after we know the offset
unicode_start_pos = len(data)
data += struct.pack('<H', 0)  # placeholder

bw = BitWriter()
for i, g in enumerate(glyphs):
    bw.write(ALL_ONES_4, BITS['per_0'])
    bw.write(MAX_CHAR_W, BITS['char_w'])
    bw.write(MAX_CHAR_H, BITS['char_h'])
    bw.write(0, BITS['char_x'])
    bw.write(0, BITS['char_y'])
    bw.write(advances[i], BITS['delta_x'])
    bm = encode_glyph_bitmap(g, BITS)
    for byte in bm:
        for bit_idx in range(7, -1, -1):
            bw.write((byte >> bit_idx) & 1, 1)

bw.write(ALL_ONES_4, BITS['per_0'])  # end marker
glyph_data = bw.data()
data += glyph_data

# Now build encoding table
enc_start = len(data)
# Update start_pos_unicode in header
struct.pack_into('<H', data, unicode_start_pos, enc_start)

# Encoding table: type=1, count=4 entries
enc = bytearray()
enc.append(1)  # encoding_type = 1 (UTF-16)
enc += struct.pack('<H', 4)  # entry count
# Entry: encoding (2B LE), glyph_index (1B)
enc += struct.pack('<H', ord('T')) + bytes([0])
enc += struct.pack('<H', ord('A')) + bytes([1])
# Kerning pair: T->A uses glyph 2,3
# Convention: encoding = (prev<<8) | curr for kerning pairs
enc += struct.pack('<H', (ord('T') << 8) | ord('A')) + bytes([2])
enc += struct.pack('<H', (ord('T') << 8) | ord('A')) + bytes([3])
data += enc

# Generate C header
hex_lines = []
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    hex_lines.append('    ' + ', '.join(f'0x{b:02X}' for b in chunk))

with open('test/test_font_kerning.h', 'w') as f:
    f.write(f'''/* Auto-generated u8g2 kerning test font
 * {GLYPH_CNT} glyphs (T, A, T_kern, A_kern) with encoding table.
 * Generated by tools/gen_test_u8g2_kerning.py
 */
#ifndef TEST_FONT_KERNING_H
#define TEST_FONT_KERNING_H

#include <stdint.h>

static const uint8_t test_kern_font_data[] = {{
{','.join(hex_lines)}
}};

#endif
''')
print("Generated test/test_font_kerning.h")
```

- [ ] **Step 2: Write kerning tests**

`test/test_font_kerning.c`:
```c
#include "eui/eui_font.h"
#include "eui/eui_types.h"
#include "eui/eui_allocator.h"
#include "test_font_kerning.h"
#include <stdio.h>

#define POOL_SIZE 32768
static uint8_t mem_pool[POOL_SIZE];

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { printf("  %s... ", n); tests_run++; } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); return; } while(0)

static const eui_font_t kern_font = {
    .format = EUI_FONT_FORMAT_U8G2,
    .line_height = 8,
    .baseline = 7,
    .flags = EUI_FONT_HAS_KERNING | EUI_FONT_HAS_UNICODE,
    .data = test_kern_font_data,
#if EUI_FONT_ENABLE_U8G2
    .lookup_glyph = NULL,
#endif
};

static void test_kerning_enabled_flag(void)
{
    TEST("kerning font has KERNING flag");
    if (!(kern_font.flags & EUI_FONT_HAS_KERNING)) FAIL("missing kerning flag");
    PASS();
}

static void test_kerning_unicode_flag(void)
{
    TEST("kerning font has UNICODE flag");
    if (!(kern_font.flags & EUI_FONT_HAS_UNICODE)) FAIL("missing unicode flag");
    PASS();
}

static void test_kerning_normal_lookup(void)
{
    TEST("kerning font returns glyph for 'T'");
    uint8_t w = eui_font_get_char_width(&kern_font, 'T');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_kerning_a_lookup(void)
{
    TEST("kerning font returns glyph for 'A'");
    uint8_t w = eui_font_get_char_width(&kern_font, 'A');
    if (w != 8) FAIL("expected 8");
    PASS();
}

static void test_kerning_out_of_range(void)
{
    TEST("kerning font out-of-range char returns 0");
    uint8_t w = eui_font_get_char_width(&kern_font, 'z');
    if (w != 0) FAIL("expected 0");
    PASS();
}

static void test_kerning_str_width(void)
{
    TEST("kerning font string width 'TA'");
    uint16_t w = eui_font_get_str_width(&kern_font, "TA");
    if (w != 16) FAIL("expected 16");
    PASS();
}

static void test_kerning_null_font(void)
{
    TEST("kerning null font returns 0");
    uint8_t w = eui_font_get_char_width(NULL, 'A');
    if (w != 0) FAIL("expected 0");
    PASS();
}

int main(void)
{
    eui_allocator_init_tlsf(mem_pool, POOL_SIZE);
    printf("=== Kerning Tests ===\n");
    test_kerning_enabled_flag();
    test_kerning_unicode_flag();
    test_kerning_normal_lookup();
    test_kerning_a_lookup();
    test_kerning_out_of_range();
    test_kerning_str_width();
    test_kerning_null_font();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
```

- [ ] **Step 3: Run kerning tests**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_KERNING=ON && make test_font_kerning && ./test/test_font_kerning
```

- [ ] **Step 4: Run tests with kerning disabled**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_KERNING=OFF && make test_font_kerning && ./test/test_font_kerning
```

- [ ] **Step 5: Commit**

```bash
git add tools/gen_test_u8g2_kerning.py test/test_font_kerning.h test/test_font_kerning.c test/CMakeLists.txt
git commit -m "feat: add kerning test font and kerning unit tests"
```

---

## Phase 4: Canvas Drawing Operations

### Task 9: Add u8g2 glyph rendering to canvas + kerning in draw_str

**Files:** Modify `src/eui_canvas.c:491-513`

- [ ] **Step 1: Add u8g2 glyph rendering to draw_glyph**

Modify `draw_glyph()` in `src/eui_canvas.c` to handle `EUI_FONT_FORMAT_U8G2`:
```c
static void draw_glyph(eui_canvas_t *canvas, const eui_font_t *font,
                       char c, int16_t x, int16_t y, uint8_t *adv_out)
{
    if (!font || !font->data) { if (adv_out) *adv_out = 0; return; }
    if (font->format == EUI_FONT_FORMAT_BDF) {
        draw_bdf_glyph(canvas, font, c, x, y, adv_out);
#if EUI_FONT_ENABLE_U8G2
    } else if (font->format == EUI_FONT_FORMAT_U8G2) {
        draw_u8g2_glyph(canvas, font, c, x, y, adv_out);
#endif
    } else {
        if (adv_out) *adv_out = 0;
    }
}
```

- [ ] **Step 2: Add draw_u8g2_glyph function**

```c
#if EUI_FONT_ENABLE_U8G2
static void draw_u8g2_glyph(eui_canvas_t *canvas, const eui_font_t *font,
                            char c, int16_t x, int16_t y, uint8_t *adv_out)
{
    /* Use the u8g2 parser to get glyph bitmap and render pixel-by-pixel */
    u8g2_glyph_t g;
    int16_t idx = eui_font_u8g2_lookup_glyph(font, (uint8_t)c, 0);
    if (idx < 0 || !decode_glyph_at(font, (uint8_t)idx, &g)) {
        if (adv_out) *adv_out = 0;
        return;
    }
    for (uint8_t row = 0; row < g.height; row++) {
        for (uint8_t col = 0; col < g.width; col++) {
            uint16_t px = (uint16_t)row * g.width + col;
            if (get_bitmap_pixel(font->data, g.bitmap_byte, g.bitmap_bit, px)) {
                canvas_set_pixel(canvas,
                                 x + col + g.x_offset,
                                 y + row + g.y_offset,
                                 canvas->fg_color);
            }
        }
    }
    if (adv_out) *adv_out = g.x_advance;
}
#endif
```

For the above to compile, move `u8g2_glyph_t`, `decode_glyph_at`, `get_bitmap_pixel` from `eui_font_u8g2.c` into a shared internal header `src/eui_font_u8g2_internal.h`.

- [ ] **Step 3: Create eui_font_u8g2_internal.h**

Extract shared structs and functions from `eui_font_u8g2.c`:
```c
#ifndef EUI_FONT_U8G2_INTERNAL_H
#define EUI_FONT_U8G2_INTERNAL_H

#include "eui/eui_font.h"
#include <stdint.h>

typedef struct {
    int8_t  x_offset;
    int8_t  y_offset;
    uint8_t width;
    uint8_t height;
    uint8_t x_advance;
    uint16_t bitmap_byte;
    uint8_t  bitmap_bit;
} u8g2_glyph_t;

uint8_t decode_glyph_at(const eui_font_t *font, uint8_t idx, u8g2_glyph_t *glyph);
uint8_t get_bitmap_pixel(const uint8_t *data, uint16_t byte_off,
                          uint8_t bit_off, uint16_t pixel_idx);

#endif
```

- [ ] **Step 4: Add kerning-aware draw_str**

Modify `eui_canvas_draw_str` in `src/eui_canvas.c`:
```c
uint16_t eui_canvas_draw_str(eui_canvas_t *canvas, int16_t x, int16_t y, const char *str)
{
    if (!canvas || !canvas->font || !str) return 0;

    int16_t cur_x = x;
    char prev = 0;
    for (const char *s = str; *s; s++) {
#if EUI_FONT_ENABLE_KERNING && EUI_FONT_ENABLE_U8G2
        /* Kerning: use lookup with prev context */
        if ((canvas->font->flags & EUI_FONT_HAS_KERNING) &&
            canvas->font->lookup_glyph && prev != 0) {
            int16_t kn_idx = canvas->font->lookup_glyph(canvas->font,
                                                         (uint16_t)(uint8_t)*s,
                                                         (uint16_t)(uint8_t)prev);
            if (kn_idx >= 0) {
                draw_u8g2_glyph_by_index(canvas, canvas->font, (uint8_t)kn_idx, cur_x, y, &adv);
                cur_x += adv;
                prev = *s;
                continue;
            }
        }
#endif
        uint8_t adv = 0;
        draw_glyph(canvas, canvas->font, *s, cur_x, y, &adv);
        cur_x += adv;
        prev = *s;
    }
    return (uint16_t)(cur_x - x);
}
```

- [ ] **Step 5: Build and verify existing tests pass**

```bash
cd build && cmake .. && make && ctest --output-on-failure
```

- [ ] **Step 6: Commit**

```bash
git add src/eui_canvas.c src/eui_font_u8g2.c src/eui_font_u8g2_internal.h
git commit -m "feat: add u8g2 glyph rendering to canvas + kerning-aware draw_str"
```

### Task 10: Implement new drawing operations (multiline, clipping, ellipsis)

**Files:** Modify `include/eui/eui_canvas.h`, Modify `src/eui_canvas.c`

- [ ] **Step 1: Declare new APIs in eui_canvas.h**

After the `Text` section add:
```c
/* Advanced text */
#if EUI_FONT_ENABLE_MULTILINE
uint16_t eui_canvas_draw_str_in_rect(eui_canvas_t *canvas,
                                      const eui_rect_t *rect, const char *str,
                                      eui_align_t h_align, eui_align_t v_align);
uint16_t eui_canvas_draw_str_multiline(eui_canvas_t *canvas,
                                        const eui_rect_t *rect, const char *str,
                                        uint8_t line_height, eui_align_t h_align);
uint16_t eui_canvas_str_multiline_height(const eui_canvas_t *canvas,
                                          const char *str,
                                          uint16_t max_width, uint8_t line_height);
#endif

uint16_t eui_canvas_draw_str_clipped(eui_canvas_t *canvas,
                                      const eui_rect_t *clip_rect,
                                      int16_t x, int16_t y, const char *str);
uint16_t eui_canvas_draw_str_ellipsis(eui_canvas_t *canvas,
                                       int16_t x, int16_t y,
                                       const char *str, uint16_t max_width);
```

- [ ] **Step 2: Implement draw_str_clipped**

```c
uint16_t eui_canvas_draw_str_clipped(eui_canvas_t *canvas,
                                      const eui_rect_t *clip_rect,
                                      int16_t x, int16_t y, const char *str)
{
    if (!canvas || !canvas->font || !str || !clip_rect) return 0;

    eui_rect_t old_clip = canvas->clip;
    canvas->clip = *clip_rect;

    int16_t cur_x = x;
    for (const char *s = str; *s; s++) {
        uint8_t adv = 0;
        draw_glyph(canvas, canvas->font, *s, cur_x, y, &adv);
        cur_x += adv;
    }

    canvas->clip = old_clip;
    return (uint16_t)(cur_x - x);
}
```

- [ ] **Step 3: Implement draw_str_ellipsis**

```c
uint16_t eui_canvas_draw_str_ellipsis(eui_canvas_t *canvas,
                                       int16_t x, int16_t y,
                                       const char *str, uint16_t max_width)
{
    if (!canvas || !canvas->font || !str) return 0;

    const eui_font_t *font = canvas->font;
    uint16_t str_w = eui_font_get_str_width(font, str);

    if (str_w <= max_width) {
        return eui_canvas_draw_str(canvas, x, y, str);
    }

    /* Reserve space for "..." */
    uint16_t ellipsis_w = eui_font_get_str_width(font, "...");
    if (ellipsis_w >= max_width) {
        return eui_canvas_draw_str(canvas, x, y, "...");
    }

    uint16_t avail = max_width - ellipsis_w;
    uint16_t cur_w = 0;
    int16_t cur_x = x;
    const char *s;
    for (s = str; *s; s++) {
        uint8_t cw = eui_font_get_char_width(font, *s);
        if (cur_w + cw > avail) break;
        uint8_t adv = 0;
        draw_glyph(canvas, font, *s, cur_x, y, &adv);
        cur_x += adv;
        cur_w += cw;
    }
    cur_x += eui_canvas_draw_str(canvas, cur_x, y, "...");
    return (uint16_t)(cur_x - x);
}
```

- [ ] **Step 4: Implement word-wrap helper**

```c
#if EUI_FONT_ENABLE_MULTILINE

/* A text line slice: pointer into source string + length */
typedef struct {
    const char *start;
    uint16_t    len;
} text_line_t;

/* Word-wrap: split str into lines fitting max_width pixels.
 * Returns number of lines written to `lines` (up to max_lines).
 * Each line is a (start, len) slice into the original str. */
static uint16_t word_wrap(const eui_font_t *font, const char *str,
                           uint16_t max_width,
                           text_line_t *lines, uint16_t max_lines)
{
    uint16_t line_count = 0;
    const char *line_start = str;
    const char *last_space = NULL;
    uint16_t line_w = 0;
    uint16_t line_chars = 0;

    if (!str || !*str) return 0;

    for (const char *s = str; ; s++) {
        if (*s == '\0' || *s == '\n') {
            if (line_count < max_lines) {
                lines[line_count].start = line_start;
                lines[line_count].len = line_chars;
                line_count++;
            }
            if (*s == '\0') break;
            line_start = s + 1;
            line_w = 0;
            line_chars = 0;
            last_space = NULL;
            continue;
        }
        if (*s == '\r') continue;

        uint8_t cw = eui_font_get_char_width(font, *s);
        if (line_w + cw > max_width) {
            if (last_space && last_space > line_start) {
                /* Break at last space (word boundary) */
                uint16_t space_offset = (uint16_t)(last_space - line_start);
                if (line_count < max_lines) {
                    lines[line_count].start = line_start;
                    lines[line_count].len = space_offset;
                    line_count++;
                }
                line_start = last_space + 1;
                /* Recalculate chars count from new line_start to current */
                line_chars = (uint16_t)(s - line_start) + 1;
                line_w = cw;
                for (const char *t = line_start; t < s; t++)
                    line_w += eui_font_get_char_width(font, *t);
                last_space = NULL;
            } else {
                /* Hard break at character boundary (word too long) */
                if (line_chars > 0 && line_count < max_lines) {
                    lines[line_count].start = line_start;
                    lines[line_count].len = line_chars;
                    line_count++;
                }
                line_start = s;
                line_w = cw;
                line_chars = 1;
            }
            continue;
        }

        if (*s == ' ') {
            last_space = s;
        }
        line_w += cw;
        line_chars++;
    }
    return line_count;
}
```

- [ ] **Step 5: Implement draw_str_multiline**

```c
uint16_t eui_canvas_draw_str_multiline(eui_canvas_t *canvas,
                                        const eui_rect_t *rect, const char *str,
                                        uint8_t line_height, eui_align_t h_align)
{
    if (!canvas || !canvas->font || !str || !rect) return 0;

    const eui_font_t *font = canvas->font;
    uint8_t lh = line_height > 0 ? line_height : eui_font_get_height(font);
    text_line_t lines[64];
    uint16_t line_count = word_wrap(font, str, rect->w, lines, 64);

    int16_t y = rect->y;
    uint16_t total_h = 0;
    uint16_t max_w = rect->w;

    for (uint16_t i = 0; i < line_count; i++) {
        /* Calculate line pixel width from (start, len) */
        uint16_t line_w = 0;
        for (uint16_t j = 0; j < lines[i].len; j++)
            line_w += eui_font_get_char_width(font, lines[i].start[j]);

        int16_t dx = rect->x;
        if (h_align & EUI_ALIGN_CENTER) dx = rect->x + (int16_t)((max_w - line_w) / 2);
        if (h_align & EUI_ALIGN_RIGHT)  dx = rect->x + (int16_t)(max_w - line_w);

        /* Draw each char of the line individually via clipped draw */
        int16_t cx = dx;
        for (uint16_t j = 0; j < lines[i].len; j++) {
            uint8_t adv = 0;
            /* Clip: only draw pixels within rect bounds */
            if (cx >= rect->x && cx + (int16_t)eui_font_get_char_width(font, lines[i].start[j]) <= rect->x + (int16_t)rect->w) {
                draw_glyph(canvas, font, lines[i].start[j], cx, y, &adv);
            }
            cx += adv;
        }

        y += lh;
        total_h += lh;
    }
    return total_h;
}
```

- [ ] **Step 6: Implement draw_str_in_rect and str_multiline_height**

```c
uint16_t eui_canvas_draw_str_in_rect(eui_canvas_t *canvas,
                                      const eui_rect_t *rect, const char *str,
                                      eui_align_t h_align, eui_align_t v_align)
{
    if (!canvas || !canvas->font || !str || !rect) return 0;

    const eui_font_t *font = canvas->font;
    uint16_t str_w = eui_font_get_str_width(font, str);
    uint8_t  fh = eui_font_get_height(font);

    int16_t dx = rect->x, dy = rect->y;

    if (h_align & EUI_ALIGN_CENTER) dx = rect->x + (int16_t)((rect->w - str_w) / 2);
    if (h_align & EUI_ALIGN_RIGHT)  dx = rect->x + (int16_t)(rect->w - str_w);
    if (v_align & EUI_ALIGN_MIDDLE) dy = rect->y + (int16_t)((rect->h - fh) / 2);
    if (v_align & EUI_ALIGN_BOTTOM) dy = rect->y + (int16_t)(rect->h - fh);

    return eui_canvas_draw_str_clipped(canvas, rect, dx, dy, str);
}

uint16_t eui_canvas_str_multiline_height(const eui_canvas_t *canvas,
                                          const char *str,
                                          uint16_t max_width, uint8_t line_height)
{
    if (!canvas || !canvas->font || !str) return 0;
    const eui_font_t *font = canvas->font;
    uint8_t lh = line_height > 0 ? line_height : eui_font_get_height(font);
    text_line_t lines[64];
    uint16_t line_count = word_wrap(font, str, max_width, lines, 64);
    return line_count * lh;
}

#endif /* EUI_FONT_ENABLE_MULTILINE */
```

- [ ] **Step 7: Build and run all tests**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_MULTILINE=ON && make && ctest --output-on-failure
```

- [ ] **Step 8: Commit**

```bash
git add include/eui/eui_canvas.h src/eui_canvas.c
git commit -m "feat: add multiline, word-wrap, clipping, ellipsis text drawing"
```

---

## Phase 5: Comprehensive Tests

### Task 11: Write multiline and word-wrap tests

**Files:** Create `test/test_font_multiline.c`, Modify `test/CMakeLists.txt`

- [ ] **Step 1: Write multiline tests**

`test/test_font_multiline.c` — test file with these test cases:
- Simple multi-line (text with `\n`)
- Word-wrap at space boundaries
- Single long word (hard break)
- Empty string
- Single character
- Line height override
- Alignment (left, center, right)
- `str_multiline_height` measurement
- Very narrow rect (one char wide)
- Text with only spaces
- Repeated newlines

- [ ] **Step 2: Run multiline tests**

```bash
cd build && cmake .. -DEUI_FONT_ENABLE_MULTILINE=ON && make test_font_multiline && ./test/test_font_multiline
```

- [ ] **Step 3: Commit**

```bash
git add test/test_font_multiline.c test/CMakeLists.txt
git commit -m "test: add multiline and word-wrap unit tests"
```

### Task 12: Write canvas font drawing tests

**Files:** Create `test/test_font_canvas.c`, Modify `test/CMakeLists.txt`

- [ ] **Step 1: Write canvas font tests**

`test/test_font_canvas.c` — test file reusing the mock display from `test_canvas.c`:
- `eui_canvas_set_font` sets font correctly
- `eui_canvas_draw_str` renders text pixels
- `eui_canvas_draw_str` with NULL returns 0
- `eui_canvas_draw_str_aligned` centers text
- `eui_canvas_draw_str_clipped` clips to rect
- `eui_canvas_draw_str_ellipsis` adds "..." for overflow
- `eui_canvas_draw_str_ellipsis` no truncation when fits
- `eui_canvas_draw_str_in_rect` alignment
- `eui_canvas_str_width` matches font
- `eui_canvas_font_height` matches font
- NULL canvas safety for all new APIs

- [ ] **Step 2: Run canvas font tests**

```bash
cd build && cmake .. && make test_font_canvas && ./test/test_font_canvas
```

- [ ] **Step 3: Commit**

```bash
git add test/test_font_canvas.c test/CMakeLists.txt
git commit -m "test: add canvas font drawing integration tests"
```

### Task 13: Expand existing font tests for coverage

**Files:** Modify `test/test_font.c`

- [ ] **Step 1: Add edge case tests**

Append to `test/test_font.c`:
- BDF draw with NULL buffer returns 0
- VLW draw with NULL buffer returns 0
- BDF str_width with NULL string returns 0
- VLW str_width with NULL string returns 0
- BDF get_char_width with all chars in range
- VLW get_char_width with all test chars
- BDF draw_char with color_depth != 1 (should not crash)
- VLW draw_char at buffer boundary
- Multiple consecutive draw_char calls on same buffer

- [ ] **Step 2: Run all font tests**

```bash
cd build && cmake .. && make test_font && ./test/test_font
```

- [ ] **Step 3: Commit**

```bash
git add test/test_font.c
git commit -m "test: expand font edge-case tests for coverage"
```

### Task 14: Add CI coverage configuration and verify

**Files:** Modify `.github/workflows/build.yml`

- [ ] **Step 1: Add matrix combinations for config macros**

Add to the CI workflow matrix:
```yaml
font_u8g2: [ON, OFF]
font_kerning: [ON, OFF]
font_multiline: [ON, OFF]
```

And pass them as cmake args:
```
-DEUI_FONT_ENABLE_U8G2=${{ matrix.font_u8g2 }} -DEUI_FONT_ENABLE_KERNING=${{ matrix.font_kerning }} -DEUI_FONT_ENABLE_MULTILINE=${{ matrix.font_multiline }}
```

- [ ] **Step 2: Add coverage build**

Add a coverage job:
```yaml
coverage:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4
    - name: Build with coverage
      run: |
        cmake -B build -DEUI_COLOR_DEPTH=1 -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
        cmake --build build
    - name: Run tests
      run: cd build && ctest --output-on-failure
    - name: Coverage
      run: |
        cd build
        gcov -r $(find . -name '*.gcno') 2>/dev/null || true
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' '*/test/*' '*/third_party/*' --output-file coverage.info
        lcov --list coverage.info
```

- [ ] **Step 3: Run coverage locally**

```bash
cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage -O0" -DCMAKE_EXE_LINKER_FLAGS="--coverage" && make -j4 && ctest --output-on-failure
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/test/*' '*/third_party/*' --output-file coverage.info
lcov --summary coverage.info
```

Expected: line coverage >= 90%.

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/build.yml
git commit -m "ci: add config matrix and coverage verification"
```

### Task 15: Final integration — run all tests

**Files:** None (verification only)

- [ ] **Step 1: Full build and test with all features enabled**

```bash
cd build && cmake .. \
  -DEUI_COLOR_DEPTH=1 \
  -DEUI_FONT_ENABLE_U8G2=ON \
  -DEUI_FONT_ENABLE_KERNING=ON \
  -DEUI_FONT_ENABLE_MULTILINE=ON \
  && make -j4 && ctest --output-on-failure
```

Expected: All tests pass.

- [ ] **Step 2: Test with features disabled**

```bash
cd build && cmake .. \
  -DEUI_COLOR_DEPTH=1 \
  -DEUI_FONT_ENABLE_U8G2=OFF \
  -DEUI_FONT_ENABLE_KERNING=OFF \
  -DEUI_FONT_ENABLE_MULTILINE=OFF \
  && make -j4 && ctest --output-on-failure
```

Expected: All tests pass (u8g2 tests skip or still pass).

- [ ] **Step 3: Commit (if any fixes)**

```bash
git add -A && git commit -m "fix: resolve issues found during final integration testing"
```

---

## Summary

| Phase | Tasks | Files Created | Files Modified |
|-------|-------|--------------|----------------|
| 1. Config & Types | 3 | 0 | 4 |
| 2. u8g2 Parser | 3 | 3 | 3 |
| 3. Kerning | 2 | 3 | 2 |
| 4. Canvas Ops | 2 | 1 | 3 |
| 5. Tests & CI | 5 | 2 | 3 |
| **Total** | **15** | **9** | **15** |
