#!/usr/bin/env python3
"""Generate a test u8g2-format font with 8 glyphs (A-H), 8x8 pixels, 1bpp.
Format: real u8g2 binary format (LSB-first bits, paired runs with continuation).
"""
import struct

GLYPH_COUNT = 8
BITS_PER_0 = 4
BITS_PER_1 = 4
BITS_PER_CHAR_W = 4
BITS_PER_CHAR_H = 4
BITS_PER_CHAR_X = 4
BITS_PER_CHAR_Y = 4
BITS_PER_DELTA_X = 5    # need 5-bit signed to encode advance=8 (range -16..15)
MAX_CHAR_W = 8
MAX_CHAR_H = 8

bdf_glyphs = {
    'A': [0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00],
    'B': [0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00],
    'C': [0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00],
    'D': [0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00],
    'E': [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00],
    'F': [0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00],
    'G': [0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00],
    'H': [0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00],
}

def encode_signed(value, num_bits):
    """Encode signed value in u8g2 format: decode does v = raw - (1<<(cnt-1))
    So raw = value + (1 << (num_bits-1))"""
    center = 1 << (num_bits - 1)
    return (value + center) & ((1 << num_bits) - 1)

class BitWriter:
    """LSB-first bit writer (matches u8g2 u8g2_font_decode_get_unsigned_bits)"""
    def __init__(self):
        self.bytes = bytearray()
        self.current_byte = 0
        self.bit_pos = 0

    def write_bits(self, value, num_bits):
        """Write num_bits from value, LSB-first within each byte."""
        mask = (1 << num_bits) - 1
        value &= mask
        self.current_byte |= (value << self.bit_pos)
        self.bit_pos += num_bits
        while self.bit_pos >= 8:
            self.bytes.append(self.current_byte & 0xFF)
            self.current_byte >>= 8
            self.bit_pos -= 8

    def flush(self):
        if self.bit_pos > 0:
            self.bytes.append(self.current_byte & 0xFF)
            self.current_byte = 0
            self.bit_pos = 0

    def get_data(self):
        self.flush()
        return bytes(self.bytes)

def encode_glyph_u8g2(glyph_rows, width, height):
    """Encode glyph using u8g2 paired-runs-with-continuation format.
    
    The u8g2 decoder reads pairs of (zero_run, one_run) followed by a 1-bit
    continue flag. If continue=1, the same run lengths apply to the next pair.
    If continue=0, new zero/one run lengths are read.
    
    For simplicity, we emit each adjacent (zero, one) run pair with continue=0.
    """
    bp0 = BITS_PER_0
    bp1 = BITS_PER_1
    max_val_0 = (1 << bp0) - 1
    max_val_1 = (1 << bp1) - 1

    # Extract pixels into a linear stream
    pixels = []
    for row in glyph_rows:
        for col in range(width):
            pixels.append((row >> (7 - col)) & 1)

    # Run-length encode
    runs = []  # list of (length, color)
    current_color = 0
    current_len = 0
    for p in pixels:
        if p == current_color:
            current_len += 1
        else:
            if current_len > 0:
                runs.append((current_len, current_color))
            current_color = p
            current_len = 1
    if current_len > 0:
        runs.append((current_len, current_color))

    # Encode as u8g2 pairs
    bw = BitWriter()
    pos = 0

    while pos < len(runs):
        a = 0  # zero run count
        if pos < len(runs) and runs[pos][1] == 0:
            a = runs[pos][0]
            pos += 1
        a = min(a, max_val_0)

        b = 0  # one run count
        if pos < len(runs) and runs[pos][1] == 1:
            b = runs[pos][0]
            pos += 1
        b = min(b, max_val_1)

        bw.write_bits(a, bp0)
        bw.write_bits(b, bp1)
        bw.write_bits(0, 1)  # continue=0 -> read new a,b next

    return bw.get_data()


# Build font data
data = bytearray()

# 23-byte header (real u8g2 format)
data.append(GLYPH_COUNT)               # 0: glyph_cnt
data.append(0)                          # 1: bbx_mode
data.append(BITS_PER_0)                # 2
data.append(BITS_PER_1)                # 3
data.append(BITS_PER_CHAR_W)           # 4
data.append(BITS_PER_CHAR_H)           # 5
data.append(BITS_PER_CHAR_X)           # 6
data.append(BITS_PER_CHAR_Y)           # 7
data.append(BITS_PER_DELTA_X)          # 8
data.append(MAX_CHAR_W)                # 9: max_char_width
data.append(MAX_CHAR_H)                # 10: max_char_height
data.append(0)                          # 11: x_offset
data.append(0)                          # 12: y_offset
data.append(8)                          # 13: ascent_A
data.append(0)                          # 14: descent_g
data.append(10)                         # 15: ascent_para
data.append(0)                          # 16: descent_para
data += struct.pack('>H', 0)           # 17-18: start_pos_upper_A (BE)
data += struct.pack('>H', 0)           # 19-20: start_pos_lower_a (BE)
data += struct.pack('>H', 0)           # 21-22: start_pos_unicode (BE)

for ch in sorted(bdf_glyphs.keys()):
    glyph_rows = bdf_glyphs[ch]

    bw = BitWriter()

    # Glyph metrics (x_offset, y_offset, advance are signed; width/height are unsigned)
    bw.write_bits(MAX_CHAR_W, BITS_PER_CHAR_W)
    bw.write_bits(MAX_CHAR_H, BITS_PER_CHAR_H)
    bw.write_bits(encode_signed(0, BITS_PER_CHAR_X), BITS_PER_CHAR_X)     # x_offset=0
    bw.write_bits(encode_signed(-1, BITS_PER_CHAR_Y), BITS_PER_CHAR_Y)     # y_offset=-1: baseline(7) - height(8) = -1
    bw.write_bits(encode_signed(MAX_CHAR_W, BITS_PER_DELTA_X), BITS_PER_DELTA_X)  # advance=8

    # Encode glyph pixels
    pixel_bytes = encode_glyph_u8g2(glyph_rows, MAX_CHAR_W, MAX_CHAR_H)
    for b in pixel_bytes:
        bw.write_bits(b, 8)

    packed = bw.get_data()
    total_size = 2 + len(packed)

    data.append(ord(ch))
    data.append(total_size)
    data += packed

data.append(0)
data.append(0)

# Compute section offsets from byte 23 for uppercase/lowercase sections
pos = 23
idx = 0
while True:
    enc = data[pos]
    sz = data[pos + 1]
    if sz == 0:
        break
    if idx == ord('A') - 32:
        struct.pack_into('>H', data, 17, pos - 23)
    if idx == ord('a') - 32:
        struct.pack_into('>H', data, 19, pos - 23)
    pos += sz
    idx += 1

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
print(f"Generated test/test_u8g2_font.h ({len(data)} bytes)")
