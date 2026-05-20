#!/usr/bin/env python3
"""Generate a test u8g2-format font with 8 glyphs (A-H), 8x8 pixels, 1bpp.
Format: u8g2 .c font binary with bit-level packed glyphs.
"""
import struct

GLYPH_COUNT = 8
BITS_PER_0 = 4
BITS_PER_1 = 4
BITS_PER_CHAR_W = 4
BITS_PER_CHAR_H = 4
BITS_PER_CHAR_X = 4
BITS_PER_CHAR_Y = 4
BITS_PER_DELTA_X = 4
MAX_CHAR_W = 8
MAX_CHAR_H = 8

# BDF glyph bitmaps (8x8) for chars A-H
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

class BitWriter:
    def __init__(self):
        self.bytes = bytearray()
        self.current_byte = 0
        self.bit_pos = 7  # 7 -> 0, MSB first

    def write_bits(self, value, num_bits):
        for i in range(num_bits - 1, -1, -1):
            bit = (value >> i) & 1
            self.current_byte |= (bit << self.bit_pos)
            self.bit_pos -= 1
            if self.bit_pos < 0:
                self.bytes.append(self.current_byte)
                self.current_byte = 0
                self.bit_pos = 7

    def flush(self):
        if self.bit_pos < 7:
            self.bytes.append(self.current_byte)
            self.current_byte = 0
            self.bit_pos = 7

    def get_data(self):
        self.flush()
        return bytes(self.bytes)

# Build font data
data = bytearray()

# Header (19 bytes)
data.append(GLYPH_COUNT)               # byte 0
data.append(0)                          # byte 1: bbx_mode = 0
data.append(BITS_PER_0)                # byte 2
data.append(BITS_PER_1)                # byte 3
data.append(BITS_PER_CHAR_W)           # byte 4
data.append(BITS_PER_CHAR_H)           # byte 5
data.append(BITS_PER_CHAR_X)           # byte 6
data.append(BITS_PER_CHAR_Y)           # byte 7
data.append(BITS_PER_DELTA_X)          # byte 8
data.append(MAX_CHAR_W)                # byte 9: max_char_w (1 byte)
data.append(MAX_CHAR_H)                # byte 10: max_char_h (1 byte)
data += struct.pack('<h', 8)           # bytes 11-12: ascent_A
data += struct.pack('<h', 8)           # bytes 13-14: ascent_g
data += struct.pack('<h', 10)          # bytes 15-16: ascent_para
data += struct.pack('<H', 0)           # bytes 17-18: start_pos_unicode (no unicode table)

# Glyph bitstream encoding
bw = BitWriter()
ALL_ONES_4 = (1 << BITS_PER_0) - 1  # 0b1111

for ch in sorted(bdf_glyphs.keys()):
    glyph_rows = bdf_glyphs[ch]

    # Emit glyph boundary marker (all0 escape prefix + escape type 0)
    bw.write_bits(ALL_ONES_4, BITS_PER_0)      # escape prefix
    bw.write_bits(0, BITS_PER_1)               # escape type 0 = glyph boundary

    # Emit glyph metrics (order: cw, ch, cx, cy, dx — matches parser)
    bw.write_bits(MAX_CHAR_W, BITS_PER_CHAR_W)  # char_width = 8
    bw.write_bits(MAX_CHAR_H, BITS_PER_CHAR_H)  # char_height = 8
    bw.write_bits(0, BITS_PER_CHAR_X)           # x_offset = 0
    bw.write_bits(0, BITS_PER_CHAR_Y)           # y_offset = 0
    bw.write_bits(MAX_CHAR_W, BITS_PER_DELTA_X) # delta_x = 8

    # Emit bitmap as zero/one runs
    run_type = 0  # 0 = zero run, 1 = one run
    run_length = 0
    max_zero_run = (1 << BITS_PER_0) - 2  # all0 - 1, all0 reserved for escape
    max_one_run  = (1 << BITS_PER_1) - 2  # all1 - 1, all1 reserved for end marker

    # Flatten bitmap into bit sequence row by row, MSB first
    for row in glyph_rows:
        for col in range(8):
            bit = (row >> (7 - col)) & 1
            if bit == run_type:
                run_length += 1
            else:
                # Emit previous run
                max_run = max_zero_run if run_type == 0 else max_one_run
                while run_length > max_run:
                    bw.write_bits(max_run, BITS_PER_0 if run_type == 0 else BITS_PER_1)
                    run_length -= max_run
                bw.write_bits(run_length, BITS_PER_0 if run_type == 0 else BITS_PER_1)
                run_type = bit
                run_length = 1
    # Emit final run
    max_run = max_zero_run if run_type == 0 else max_one_run
    while run_length > max_run:
        bw.write_bits(max_run, BITS_PER_0 if run_type == 0 else BITS_PER_1)
        run_length -= max_run
    bw.write_bits(run_length, BITS_PER_0 if run_type == 0 else BITS_PER_1)

# Emit end-of-data marker (all0 escape prefix + all1 escape type)
bw.write_bits(ALL_ONES_4, BITS_PER_0)       # escape prefix
bw.write_bits(ALL_ONES_4, BITS_PER_1)       # escape type all1 = end of data

bitmap_data = bw.get_data()
data += bitmap_data

# Generate C header
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
