#!/usr/bin/env python3
"""Generate a u8g2-format test font with kerning encoding table.
2 glyphs in index table (T, A), plus 1 kerning pair in encoding table (T<<8|A -> A_kern).
A_kern has x_offset=-2, advance=6 to pull A closer to preceding T.
"""
import struct

GLYPH_CNT = 2
BITS_PER_0 = 4
BITS_PER_1 = 4
BITS_PER_CHAR_W = 4
BITS_PER_CHAR_H = 4
BITS_PER_CHAR_X = 4
BITS_PER_CHAR_Y = 4
BITS_PER_DELTA_X = 4
MAX_CHAR_W = 8
MAX_CHAR_H = 8

# Bitmaps: T and A (8x8)
T_BITMAP = [0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00]
A_BITMAP = [0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00]

# Index table: [char, bitmap, advance, x_offset, y_offset] — must be sorted by codepoint
index_entries = [
    ('A', A_BITMAP, 8, 0, -1),
    ('T', T_BITMAP, 8, 0, -1),
]

# Encoding table: [encoding, bitmap, advance, x_offset, y_offset]
enc_entries = [
    (0x0054, T_BITMAP, 8, 0, -1),
    (0x0041, A_BITMAP, 8, 0, -1),
    (0x5441, A_BITMAP, 6, -2, -1),  # T<<8|A -> A_kern
]


def encode_signed(value, num_bits):
    """Encode signed value in u8g2 format: decode does v = raw - (1<<(cnt-1))"""
    center = 1 << (num_bits - 1)
    return (value + center) & ((1 << num_bits) - 1)


class BitWriter:
    def __init__(self):
        self.bytes = bytearray()
        self.current_byte = 0
        self.bit_pos = 0

    def write_bits(self, value, num_bits):
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


def encode_glyph_header_and_bitmap(advance, x_offset, y_offset, bitmap):
    """Pack glyph header (cW,cH,cX,cY,dX) + bitmap into a BitWriter and return packed bytes."""
    bw = BitWriter()
    if advance == MAX_CHAR_W:
        dx_signed = 0
    else:
        dx_signed = advance

    bw.write_bits(MAX_CHAR_W, BITS_PER_CHAR_W)
    bw.write_bits(MAX_CHAR_H, BITS_PER_CHAR_H)
    bw.write_bits(encode_signed(x_offset, BITS_PER_CHAR_X), BITS_PER_CHAR_X)
    bw.write_bits(encode_signed(y_offset, BITS_PER_CHAR_Y), BITS_PER_CHAR_Y)
    bw.write_bits(encode_signed(dx_signed, BITS_PER_DELTA_X), BITS_PER_DELTA_X)

    pixel_bytes = encode_glyph_u8g2(bitmap, MAX_CHAR_W, MAX_CHAR_H)
    for b in pixel_bytes:
        bw.write_bits(b, 8)

    return bw.get_data()


data = bytearray()

# --- 23-byte header ---
data.append(GLYPH_CNT)       # 0: glyph count
data.append(0)               # 1: bbx mode
data.append(BITS_PER_0)      # 2
data.append(BITS_PER_1)      # 3
data.append(BITS_PER_CHAR_W) # 4
data.append(BITS_PER_CHAR_H) # 5
data.append(BITS_PER_CHAR_X) # 6
data.append(BITS_PER_CHAR_Y) # 7
data.append(BITS_PER_DELTA_X)# 8
data.append(MAX_CHAR_W)      # 9
data.append(MAX_CHAR_H)      # 10
data.append(0)               # 11
data.append(0)               # 12
data.append(8)               # 13
data.append(0)               # 14
data.append(10)              # 15
data.append(0)               # 16
data += struct.pack('>H', 0) # 17-18: start_pos_upper_A (patched later)
data += struct.pack('>H', 0) # 19-20: start_pos_lower_a
data += struct.pack('>H', 0) # 21-22: start_pos_unicode (patched later)

# --- Main index table ---
for char, bitmap, advance, x_off, y_off in index_entries:
    packed = encode_glyph_header_and_bitmap(advance, x_off, y_off, bitmap)
    total_size = 2 + len(packed)
    data.append(ord(char))
    data.append(total_size)
    data += packed

# Terminator for main index table
data.append(0)
data.append(0)

# Patch start_pos_upper_A and start_pos_lower_a
pos = 23
idx = 0
for char, _, _, _, _ in index_entries:
    sz = data[pos + 1]
    if sz == 0:
        break
    if ord(char) == ord('A'):
        struct.pack_into('>H', data, 17, pos - 23)
    pos += sz
    idx += 1

# --- Encoding table ---
# Jump table: (block_off, last_unicode) pairs, terminated by 0xFFFF
enc = bytearray()
enc += struct.pack('>H', 4)        # block_offset: 1 entry × 4 bytes = 4
enc += struct.pack('>H', 0xFFFF)   # last_unicode: covers all codes

# Encode each entry as: [2B encoding][1B total_size][N bytes glyph_data]
for encoding, bitmap, advance, x_off, y_off in enc_entries:
    packed = encode_glyph_header_and_bitmap(advance, x_off, y_off, bitmap)
    total_size = 3 + len(packed)
    enc += struct.pack('>H', encoding)
    enc += bytes([total_size])
    enc += packed

# Null terminator entry (code=0)
enc += bytes([0, 0, 0])

data += enc

# Patch unicode offset (offset from byte 23 to start of encoding data area)
unicode_offset = len(data) - len(enc) - 23
struct.pack_into('>H', data, 21, unicode_offset)

# --- Generate C header ---
hex_lines = []
for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    hex_lines.append('    ' + ', '.join(f'0x{b:02X}' for b in chunk))

glyph_desc = 'T (adv=8), A (adv=8)'
kerning_desc = '(T<<8|A) -> A_kern (adv=6, x_off=-2)'

with open('test/test_font_kerning.h', 'w') as f:
    f.write(f'''/* Auto-generated u8g2 kerning test font
 * Index table: {glyph_desc}
 * Encoding table: {kerning_desc}
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
