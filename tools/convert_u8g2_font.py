#!/usr/bin/env python3
"""Extract a single font from u8g2_fonts.c, convert string-literals to raw hex bytes."""
import sys, re

def parse_octal_string(data_str):
    result = bytearray()
    i = 0
    while i < len(data_str):
        if data_str[i:i+1] == '\\' and i+1 < len(data_str):
            if data_str[i+1] in '01234567':
                octal = ''
                j = i+1
                while j < min(i+4, len(data_str)) and data_str[j] in '01234567':
                    octal += data_str[j]; j += 1
                result.append(int(octal, 8))
                i = j
            elif data_str[i+1] == 'n': result.append(10); i += 2
            elif data_str[i+1] == 't': result.append(9); i += 2
            elif data_str[i+1] == 'r': result.append(13); i += 2
            elif data_str[i+1] == '0': result.append(0); i += 2
            elif data_str[i+1] == '"': result.append(0x22); i += 2
            elif data_str[i+1] == '\\': result.append(0x5C); i += 2
            elif data_str[i+1] == 'x': i += 2;  # skip \x escapes
            else: result.append(ord(data_str[i+1])); i += 2
        elif data_str[i:i+1] == '"':
            i += 1
        elif data_str[i:i+1] not in '\n\r ':
            result.append(ord(data_str[i])); i += 1
        else:
            i += 1
    return bytes(result)

def extract_font(filename, font_name):
    with open(filename, 'r', errors='replace') as f:
        content = f.read()
    
    pattern = rf'(const uint8_t {re.escape(font_name)}\[\d+\]).*?=\s*((?:"[^"]*"\s*)+);'
    m = re.search(pattern, content, re.DOTALL)
    if not m:
        print(f"Font {font_name} not found")
        sys.exit(1)
    
    decl = m.group(1)
    str_data = m.group(2)
    
    # Parse string literals into bytes
    raw = bytearray()
    for s in re.findall(r'"([^"]*)"', str_data):
        raw += parse_octal_string(s)
    
    print(f"/* Extracted from u8g2, font: {font_name}, {len(raw)} bytes */")
    print(f"static const uint8_t {font_name}_data[] = {{")
    for i in range(0, len(raw), 16):
        hex_vals = ', '.join(f'0x{b:02X}' for b in raw[i:i+16])
        print(f"    {hex_vals},")
    print(f"}};")
    print(f"#define {font_name.upper()}_SIZE {len(raw)}")

if __name__ == '__main__':
    extract_font('/tmp/u8g2/csrc/u8g2_fonts.c', 'u8g2_font_profont10_tf')
