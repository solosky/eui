#!/usr/bin/env python3
"""Convert PNG images to RGB565 C byte arrays for EUI framework."""

from PIL import Image
import os

ICON_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT = os.path.join(ICON_DIR, "amiibo_icons.h")
SIZE = 120

# Map filenames to C variable names
IMAGES = [
    ("mario.png", "g_icon_mario"),
    ("link.png", "g_icon_link"),
    ("zelda.png", "g_icon_zelda"),
    ("pikachu.png", "g_icon_pikachu"),
    ("kirby.png", "g_icon_kirby"),
    ("samus.png", "g_icon_samus"),
    ("yoshi.png", "g_icon_yoshi"),
    ("donkey_kong.png", "g_icon_dk"),
    ("amiibo_logo.png", "g_icon_amiibo_app"),
    ("nfc_icon.png", "g_icon_nfc"),
    ("settings_icon.png", "g_icon_settings"),
]

def rgb565(r, g, b):
    """Convert RGB 8-bit to RGB565 16-bit."""
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def process_image(filename, var_name):
    path = os.path.join(ICON_DIR, filename)
    img = Image.open(path).convert("RGBA")
    
    w, h = img.size
    # Crop to square from center
    if w != h:
        side = min(w, h)
        left = (w - side) // 2
        top = (h - side) // 2
        img = img.crop((left, top, left + side, top + side))
    
    # Resize to target size
    img = img.resize((SIZE, SIZE), Image.LANCZOS)
    
    pixels = []
    has_alpha = False
    for y in range(SIZE):
        for x in range(SIZE):
            r, g, b, a = img.getpixel((x, y))
            if a < 128:
                # Transparent: use black
                r = g = b = 0
            else:
                has_alpha = True
            val = rgb565(r, g, b)
            pixels.append(val >> 8)   # high byte
            pixels.append(val & 0xFF) # low byte
    
    return pixels, has_alpha

def generate_header():
    lines = [
        "/* Auto-generated Amiibo icons, 120x120 RGB565 */",
        "#ifndef AMIIBO_ICONS_H",
        "#define AMIIBO_ICONS_H",
        "",
        '#include "eui/eui_types.h"',
        "",
    ]
    
    for filename, var_name in IMAGES:
        pixels, has_alpha = process_image(filename, var_name)
        total_bytes = len(pixels)
        lines.append(f"static const uint8_t {var_name}_data[{total_bytes}] = {{")
        # Write in rows of 16 bytes
        for i in range(0, len(pixels), 16):
            chunk = pixels[i:i+16]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            lines.append(f"  {hex_str},")
        lines.append("};")
        lines.append("")
    
    # Generate bitmap structs
    lines.append("/* Bitmap structs for use with eui_canvas_draw_bitmap() */")
    for filename, var_name in IMAGES:
        total_bytes = SIZE * SIZE * 2
        lines.append(f"static const eui_bitmap_t {var_name} = {{")
        lines.append(f"  .width = {SIZE},")
        lines.append(f"  .height = {SIZE},")
        lines.append(f"  .color_depth = 16,")
        lines.append(f"  .data = {var_name}_data,")
        lines.append("};")
        lines.append("")
    
    # Generate array of all Amiibo icon pointers
    lines.append("/* Array of all Amiibo icon bitmaps */")
    lines.append("static const eui_bitmap_t* g_amiibo_icons[8] = {")
    amiibo_vars = [v for f, v in IMAGES if f != "amiibo_logo.png" and f != "nfc_icon.png" and f != "settings_icon.png"]
    for v in amiibo_vars:
        lines.append(f"  &{v},")
    lines.append("};")
    lines.append("")
    
    lines.append("#endif /* AMIIBO_ICONS_H */")
    
    with open(OUTPUT, "w") as f:
        f.write("\n".join(lines) + "\n")
    
    total_size = sum(len(process_image(f, v)[0]) for f, v in IMAGES)
    print(f"Generated {OUTPUT}")
    print(f"Total icon data: {total_size} bytes ({total_size / 1024:.1f} KB)")

if __name__ == "__main__":
    generate_header()
