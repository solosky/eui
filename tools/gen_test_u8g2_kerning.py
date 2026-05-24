#!/usr/bin/env python3
"""Generate a u8g2-format test font with kerning encoding table.
4 glyphs: regular T(0), regular A(1), kerning T(2), kerning A(3).
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

glyphs = [
    [0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00],
    [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00],
    [0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00],
    [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00],
]
advances = [8, 8, 6, 8]


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
    """Encode glyph using u8g2 paired-runs-with-continuation format."""
    bp0 = BITS_PER_0
    bp1 = BITS_PER_1
    max_val_0 = (1 << bp0) - 1
    max_val_1 = (1 << bp1) - 1

    pixels = []
    for row in glyph_rows:
        for col in range(width):
            pixels.append((row >> (7 - col)) & 1)

    runs = []
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

    bw = BitWriter()
    pos = 0

    while pos < len(runs):
        a = 0
        if pos < len(runs) and runs[pos][1] == 0:
            a = runs[pos][0]
            pos += 1
        a = min(a, max_val_0)

        b = 0
        if pos < len(runs) and runs[pos][1] == 1:
            b = runs[pos][0]
            pos += 1
        b = min(b, max_val_1)

        bw.write_bits(a, bp0)
        bw.write_bits(b, bp1)
        bw.write_bits(0, 1)

    return bw.get_data()


data = bytearray()

# 23-byte header
data.append(GLYPH_CNT)
data.append(0)
data.append(BITS_PER_0)
data.append(BITS_PER_1)
data.append(BITS_PER_CHAR_W)
data.append(BITS_PER_CHAR_H)
data.append(BITS_PER_CHAR_X)
data.append(BITS_PER_CHAR_Y)
data.append(BITS_PER_DELTA_X)
data.append(MAX_CHAR_W)
data.append(MAX_CHAR_H)
data.append(0)
data.append(0)
data.append(8)
data.append(0)
data.append(10)
data.append(0)
data += struct.pack('>H', 0)           # 17-18: start_pos_upper_A (BE)
data += struct.pack('>H', 0)           # 19-20: start_pos_lower_a (BE)
data += struct.pack('>H', 0)           # 21-22: start_pos_unicode (BE)

for i, g in enumerate(glyphs):
    bw = BitWriter()

    # Delta X is a signed value. If 0, mcw is used instead of the delta.
    # For advance == mcw: encode 0 (use mcw).
    # For advance != mcw: encode the advance as a signed delta.
    if advances[i] == MAX_CHAR_W:
        dx_signed = 0
    else:
        dx_signed = advances[i]

    bw.write_bits(MAX_CHAR_W, BITS_PER_CHAR_W)
    bw.write_bits(MAX_CHAR_H, BITS_PER_CHAR_H)
    bw.write_bits(encode_signed(0, BITS_PER_CHAR_X), BITS_PER_CHAR_X)
    bw.write_bits(encode_signed(-1, BITS_PER_CHAR_Y), BITS_PER_CHAR_Y)
    bw.write_bits(encode_signed(dx_signed, BITS_PER_DELTA_X), BITS_PER_DELTA_X)

    # Encode glyph pixels
    pixel_bytes = encode_glyph_u8g2(g, MAX_CHAR_W, MAX_CHAR_H)
    for b in pixel_bytes:
        bw.write_bits(b, 8)

    packed = bw.get_data()
    total_size = 2 + len(packed)

    if i == 0:
        data.append(ord('T'))
    elif i == 1:
        data.append(ord('A'))
    else:
        data.append(i)
    data.append(total_size)
    data += packed

data.append(0)
data.append(0)

# Unicode encoding table (big-endian, matching get_be16 in C reader)
# Format: for each block: 2 bytes block_offset, 2 bytes last_unicode
# Followed by per-glyph entries: 2 bytes encoding, 1 byte glyph_index
enc = bytearray()
# Jump table: sequence of (block_off, last_unicode) pairs, terminated by last_unicode == 0xFFFF
# block_off is cumulative bytes from jump_table start to the glyph entries for this block
enc += struct.pack('>H', 8)              # block_offset = 2 blocks × 4 bytes = 8 (past jump table to glyphs)
enc += struct.pack('>H', 0xFFFF)         # last_unicode for block (covers all, terminates loop)

# Glyph entries in unicode encoding table (encoding, index)
enc += struct.pack('>H', ord('T')) + bytes([0])
enc += struct.pack('>H', ord('A')) + bytes([1])
enc += struct.pack('>H', (ord('T') << 8) | ord('A')) + bytes([2])
enc += struct.pack('>H', (ord('T') << 8) | ord('A')) + bytes([3])
data += enc

# Compute section offsets from byte 23
unicode_offset = len(data) - len(enc) - 23
struct.pack_into('>H', data, 21, unicode_offset)

pos = 23
idx = 0
while True:
    enc_byte = data[pos]
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
