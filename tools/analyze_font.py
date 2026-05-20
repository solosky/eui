#!/usr/bin/env python3
import re

with open('/home/solosky/eui/test/test_u8g2_profont10_data.h') as f:
    content = f.read()
hex_vals = re.findall(r'0x([0-9A-F]+),?', content)
data = bytes(int(h, 16) for h in hex_vals)
print(f'Total bytes: {len(data)}')

hdr_size = 23
start_upper = (data[17] << 8) | data[18]
start_lower = (data[19] << 8) | data[20]
print(f'start_upper_A={start_upper}, start_lower_a={start_lower}')
print(f'Uppercase starts at byte {hdr_size + start_upper}')
print(f'Lowercase starts at byte {hdr_size + start_lower}')

# First section (non-alphabetic, before uppercase)
print(f'\nFirst section entries (bytes {hdr_size} to {hdr_size+start_upper}):')
pos = hdr_size
count = 0
while pos + 1 < len(data) and data[pos+1] != 0 and count < 10:
    enc = data[pos]
    size = data[pos+1]
    ch = chr(enc) if 32 <= enc < 127 else '?'
    print(f'  byte {pos}: enc=0x{enc:02X} ({ch}), size={size}')
    pos += size
    count += 1
if pos < len(data) - 1:
    ch = chr(data[pos]) if 32 <= data[pos] < 127 else '?'
    print(f'  Terminator at byte {pos}: enc=0x{data[pos]:02X} ({ch}), size={data[pos+1]}')

# Uppercase section
print(f'\nUppercase entries (bytes {hdr_size+start_upper} to {hdr_size+start_lower}):')
pos = hdr_size + start_upper
count = 0
while pos + 1 < len(data) and data[pos+1] != 0 and count < 30:
    enc = data[pos]
    size = data[pos+1]
    ch = chr(enc) if 32 <= enc < 127 else '?'
    print(f'  byte {pos}: enc=0x{enc:02X} ({ch}), size={size}')
    pos += size
    count += 1
print(f'  ... total uppercase entries: {count}')

# Lowercase section
print(f'\nLowercase entries (bytes {hdr_size+start_lower} onwards):')
pos = hdr_size + start_lower
count = 0
while pos + 1 < len(data) and data[pos+1] != 0 and count < 30:
    enc = data[pos]
    size = data[pos+1]
    ch = chr(enc) if 32 <= enc < 127 else '?'
    print(f'  byte {pos}: enc=0x{enc:02X} ({ch}), size={size}')
    pos += size
    count += 1
print(f'  ... total lowercase entries: {count}')
print(f'  Terminator after lowercase at byte {pos}: size={data[pos+1]}')
