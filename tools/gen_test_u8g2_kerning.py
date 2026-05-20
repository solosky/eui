#!/usr/bin/env python3
"""Generate a u8g2-format test font with kerning encoding table.
4 glyphs: regular T(0), regular A(1), kerning T(2), kerning A(3).
The encoding table maps (prev='T', curr='A') to kerned glyph variants
via combined encoding value (prev << 8) | curr.
"""
import struct

GLYPH_CNT = 4
BITS_PER_0 = 4
BITS_PER_1 = 4
BITS_PER_CHAR_W = 4
BITS_PER_CHAR_H = 4
BITS_PER_CHAR_X = 4
BITS_PER_CHAR_Y = 4
BITS_PER_DELTA_X = 4
MAX_CHAR_W = 8
MAX_CHAR_H = 8

# T: vertical bar + horizontal top (advance 8)
# A: A shape (advance 8)
# T_kern: vertical bar only, shorter right bearing (advance 6)
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
        self.current_byte = 0
        self.bit_pos = 7

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
data.append(GLYPH_CNT)               # byte 0
data.append(0)                        # byte 1: bbx_mode = 0
data.append(BITS_PER_0)              # byte 2
data.append(BITS_PER_1)              # byte 3
data.append(BITS_PER_CHAR_W)         # byte 4
data.append(BITS_PER_CHAR_H)         # byte 5
data.append(BITS_PER_CHAR_X)         # byte 6
data.append(BITS_PER_CHAR_Y)         # byte 7
data.append(BITS_PER_DELTA_X)        # byte 8
data.append(MAX_CHAR_W)              # byte 9: max_char_w
data.append(MAX_CHAR_H)              # byte 10: max_char_h
data += struct.pack('<h', 8)         # bytes 11-12: ascent_A
data += struct.pack('<h', 8)         # bytes 13-14: ascent_g
data += struct.pack('<h', 10)        # bytes 15-16: ascent_para
unicode_pos = len(data)
data += struct.pack('<H', 0)         # bytes 17-18: start_pos_unicode (placeholder)

# Glyph bitstream with RLE encoding (matches decoder expectations)
bw = BitWriter()
ALL_ONES_4 = (1 << BITS_PER_0) - 1  # 0b1111

for i, g in enumerate(glyphs):
    # Emit glyph boundary marker
    bw.write_bits(ALL_ONES_4, BITS_PER_0)      # escape prefix
    bw.write_bits(0, BITS_PER_1)               # escape type 0 = glyph boundary

    # Emit glyph metrics
    bw.write_bits(MAX_CHAR_W, BITS_PER_CHAR_W)  # char_width = 8
    bw.write_bits(MAX_CHAR_H, BITS_PER_CHAR_H)  # char_height = 8
    bw.write_bits(0, BITS_PER_CHAR_X)           # x_offset = 0
    bw.write_bits(0, BITS_PER_CHAR_Y)           # y_offset = 0
    bw.write_bits(advances[i], BITS_PER_DELTA_X) # delta_x

    # Emit bitmap as zero/one runs (RLE)
    run_type = 0
    run_length = 0
    max_zero_run = (1 << BITS_PER_0) - 2
    max_one_run  = (1 << BITS_PER_1) - 2

    for row in g:
        for col in range(8):
            bit = (row >> (7 - col)) & 1
            if bit == run_type:
                run_length += 1
            else:
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

# End marker
bw.write_bits(ALL_ONES_4, BITS_PER_0)  # escape prefix
bw.write_bits(ALL_ONES_4, BITS_PER_1)  # escape type all-1s = end of data

data += bw.get_data()

# Encoding table
enc_start = len(data)
struct.pack_into('<H', data, unicode_pos, enc_start)

enc = bytearray()
enc.append(1)  # encoding_type
enc += struct.pack('<H', 4)  # count = 4 entries
# Entry: encoding (2B LE), glyph_index (1B)
enc += struct.pack('<H', ord('T')) + bytes([0])  # 'T' -> glyph 0
enc += struct.pack('<H', ord('A')) + bytes([1])  # 'A' -> glyph 1
# Kerning pair: combined encoding (prev<<8 | curr) for kerning variants
enc += struct.pack('<H', (ord('T') << 8) | ord('A')) + bytes([2])  # T_kern
enc += struct.pack('<H', (ord('T') << 8) | ord('A')) + bytes([3])  # A_kern
data += enc

# Write C header
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
