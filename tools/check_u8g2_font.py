#!/usr/bin/env python3
"""Parse u8g2 font header from C string literal format and print info."""
import sys, re

def parse_string_literal(data_str):
    """Parse C string literal with octal escapes into bytes."""
    result = bytearray()
    i = 0
    while i < len(data_str):
        if data_str[i:i+2] == '\\"':  # escaped quote
            result.append(0x22)
            i += 2
        elif data_str[i:i+1] == '\\' and i+1 < len(data_str):
            if data_str[i+1] in '01234567':  # octal
                octal = ''
                j = i+1
                while j < min(i+4, len(data_str)) and data_str[j] in '01234567':
                    octal += data_str[j]
                    j += 1
                result.append(int(octal, 8))
                i = j
            elif data_str[i+1] == 'n':
                result.append(0x0A); i += 2
            elif data_str[i+1] == 't':
                result.append(0x09); i += 2
            elif data_str[i+1] == 'r':
                result.append(0x0D); i += 2
            elif data_str[i+1] == '0':
                result.append(0x00); i += 2
            elif data_str[i+1] == '\\':
                result.append(0x5C); i += 2
            else:
                result.append(ord(data_str[i+1]))
                i += 2
        elif data_str[i:i+1] == '"':
            i += 1
        else:
            result.append(ord(data_str[i]))
            i += 1
    return bytes(result)

def read_font_data(filename):
    with open(filename, 'rb') as f:
        content = f.read().decode('utf-8', errors='replace')
    # Find the const uint8_t definition
    m = re.search(r'const uint8_t \w+\[\d+\].*?=\s*((?:"[^"]*"\s*)+);', content, re.DOTALL)
    if not m:
        print("Could not find font data")
        sys.exit(1)
    # Extract all string literals
    data_str = m.group(1)
    # Remove whitespace and newlines between strings
    data_str = data_str.replace('\n', ' ').replace('\r', '')
    # Find all quoted strings
    strings = re.findall(r'"([^"]*)"', data_str)
    combined = ''.join(strings)
    return parse_string_literal(combined)

data = read_font_data(sys.argv[1])
print(f"Total size: {len(data)} bytes")
print(f"Header (first 21 bytes):")
print(f"  glyph_cnt       = {data[0]}")
print(f"  bbx_mode        = {data[1]}")
print(f"  bits_per_0      = {data[2]}")
print(f"  bits_per_1      = {data[3]}")
print(f"  bits_per_char_w = {data[4]}")
print(f"  bits_per_char_h = {data[5]}")
print(f"  bits_per_char_x = {data[6]}")
print(f"  bits_per_char_y = {data[7]}")
print(f"  bits_per_delta_x= {data[8]}")
print(f"  max_char_w      = {data[9]}")
print(f"  max_char_h      = {data[10]}")
print(f"  start_pos_unicode= {data[17] | (data[18] << 8)}")
