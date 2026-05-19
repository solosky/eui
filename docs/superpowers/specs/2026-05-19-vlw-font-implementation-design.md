# VLW Font Implementation Design

## Overview

Implement Processing `.vlw` font format reading and rendering in `eui_font_vlw.c`,
replacing the current stub implementation. The VLW (Very Light Weight) format
is Processing's binary bitmap font format, used for embedded-friendly grayscale
glyph storage.

## VLW Binary Format Specification

Source: [Processing PFont.java](https://github.com/processing/processing4/blob/main/core/src/processing/core/PFont.java)

All multi-byte integers are big-endian (MSB first, Java `DataOutputStream` convention).

### File Header (24 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | glyphCount | int32, number of glyphs |
| 4 | 4 | version | int32, format version (8-11) |
| 8 | 4 | size | int32, original font size in pixels |
| 12 | 4 | mboxY | int32, ignored (legacy padding) |
| 16 | 4 | ascent | int32, distance baseline→top |
| 20 | 4 | descent | int32, distance baseline→bottom |

`line_height = ascent + descent`.

### Glyph Headers (28 bytes each, glyphCount entries)

Code points are stored in **ascending order** (sorted at font creation time).
This enables binary search for glyph lookup.

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | value | int32, Unicode code point |
| 4 | 4 | height | int32, bitmap height in pixels |
| 8 | 4 | width | int32, bitmap width in pixels |
| 12 | 4 | setWidth | int32, horizontal advance width |
| 16 | 4 | topExtent | int32, pixels baseline→top of bitmap |
| 20 | 4 | leftExtent | int32, pixels from origin→left of bitmap |
| 24 | 4 | padding | int32, unused (0) |

### Glyph Bitmap Data

Concatenated sequentially in glyph header order (no per-glyph headers).  
Per glyph: `width × height` bytes, row-major, **8bpp grayscale**:
- `0` = black (ink, opaque foreground)
- `255` = white (background, transparent)
- Intermediate values = anti-aliased grayscale

### Footer (only if version >= 10)

- Modified UTF-8 string (2-byte length + data): font family name
- Modified UTF-8 string: PostScript name

### Footer (version >= 11)

- 1 byte boolean: smooth (anti-aliasing) flag

## Mapping to eui_font_t

```c
typedef struct {
    uint8_t  format;        // EUI_FONT_FORMAT_VLW (1)
    uint8_t  line_height;   // ascent + descent from VLW header
    uint8_t  baseline;      // ascent from VLW header
    uint8_t  flags;         // EUI_FONT_FIXED_WIDTH if all glyphs same setWidth
    const uint8_t *data;    // pointer to raw VLW binary
} eui_font_t;
```

The user pre-fills `format`, `line_height`, `baseline`, `flags`, and `data`.
The VLW parser reads `ascent`/`descent` from the binary header only for
integrity (they should match `line_height`/`baseline`).

## Implementation: eui_font_vlw.c

Replaces the current stub (3 functions all return 0) with real implementations.

### Helper: `vlw_read_int32(p)`

Read a big-endian int32 from byte pointer:
```c
static int32_t vlw_read_int32(const uint8_t *p) {
    return ((int32_t)p[0] << 24) | ((int32_t)p[1] << 16) |
           ((int32_t)p[2] << 8)  |  (int32_t)p[3];
}
```

### Helper: `find_glyph(font, c)`

Binary search the glyph table for Unicode code point `c`:
- `glyphCount` from header bytes [0..3]
- Glyph headers start at byte offset 24
- Each header is 28 bytes; `value` field is at offset 0 within header
- Return pointer to glyph header (28 bytes) or NULL

```c
static const uint8_t* find_glyph(const eui_font_t *font, char c) {
    int32_t count = vlw_read_int32(font->data);
    const uint8_t *glyphs = font->data + 24;
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const uint8_t *gh = glyphs + mid * 28;
        int32_t v = vlw_read_int32(gh);
        if ((int32_t)(uint8_t)c == v) return gh;
        if ((int32_t)(uint8_t)c < v) hi = mid - 1;
        else lo = mid + 1;
    }
    return NULL;
}
```

### `eui_font_vlw_get_char_width(font, c)`

1. Call `find_glyph(font, c)`
2. Read `setWidth` at offset 12 within glyph header → return it
3. Not found → return 0

### `eui_font_vlw_get_str_width(font, str)`

Sum `get_char_width` over each char in `str` (identical to BDF pattern).

### `eui_font_vlw_draw_char(font, c, buf, buf_stride, color_depth)`

1. Call `find_glyph(font, c)` → NULL → return 0
2. Read `width` (offset 4), `height` (offset 8), `setWidth` (offset 12) from glyph header
3. Calculate bitmap start: `header_end + sum(width[i] * height[i] for i < index)`
   - Glyph bitmaps follow consecutively after glyph headers
   - Must iterate through prior glyphs to compute bitmap offset
4. For each pixel (row, col) in [0..height) × [0..width):
   - Read byte: `bitmap[row * width + col]`
   - If byte < 128 (ink pixel), write to output buffer:

| color_depth | Write operation |
|-------------|----------------|
| 1 | `buf[row * buf_stride + col/8] \|= (1u << (7 - (col % 8)))` |
| 8 | `buf[row * buf_stride + col] = 0xFF` |
| else | `buf[row * buf_stride + col] = 0xFF` (fallback, treat as 8bpp) |

5. Return `setWidth`

**Bitmap offset calculation** — since glyph widths/heights vary, need to compute
running byte offset across prior glyphs:

```c
static const uint8_t* glyph_bitmap_start(const eui_font_t *font, int target_idx) {
    const uint8_t *glyphs = font->data + 24;
    int32_t count = vlw_read_int32(font->data);
    const uint8_t *bitmap_base = glyphs + count * 28;
    int32_t offset = 0;
    for (int i = 0; i < target_idx; i++) {
        const uint8_t *gh = glyphs + i * 28;
        int32_t w = vlw_read_int32(gh + 4);
        int32_t h = vlw_read_int32(gh + 8);
        offset += w * h;
    }
    return bitmap_base + offset;
}
```

An optimization: pre-compute a cumulative offset array for O(1) lookup if
needed in future. For now, O(n) scan is acceptable for fonts with <256 glyphs.

## Test Font

Create `test/test_vlw_font.h` with a small Processing VLW format font:
- 8 characters: A-H (matching the existing BDF builtin)
- 12px size, ascent=10, descent=2 → line_height=12, baseline=10
- Each glyph: 8×8 bitmap (1 bytes/pixel = 64 bytes per glyph)
- Version 11, no name strings
- Glyphs stored as `const uint8_t[]` with manual big-endian encoding

## Test Updates (`test/test_font.c`)

- Remove `test_vlw_format_returns_zero`
- Add VLW equivalents: char_width_for_vlw, str_width_for_vlw, draw_char_for_vlw,
  height_for_vlw, baseline_for_vlw, out_of_range_for_vlw, empty_str_for_vlw

## Scope

Only modifies:
- `src/eui_font_vlw.c` — replace stub with real implementation
- `test/test_vlw_font.h` — new VLW test font data (create)
- `test/test_font.c` — replace stub test with real VLW tests
- `test/CMakeLists.txt` — add test_vlw_font.h if needed

Does NOT touch: canvas text rendering (still stubbed), eui_font.c dispatch
(already routes correctly), eui_font_internal.h (signatures unchanged).

## Error Handling

- `font == NULL` → handled by dispatch layer in `eui_font.c`
- `font->data == NULL` → find_glyph returns NULL, all functions return 0
- Corrupted data (width/height overflow) → bounded by available memory; for
  safety, cap glyph dimensions at reasonable limits (e.g., 255×255)
- Out-of-range character → find_glyph returns NULL → return 0

## Edge Cases

- Empty font (glyphCount = 0) → all functions return 0
- Zero-width glyph → x_advance = 0, draw_char draws nothing, returns 0
- Multiple glyphs with same code point → undefined (should not occur in valid
  VLW files; binary search finds one arbitrarily)
