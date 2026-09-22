#!/bin/bash
# build_all.sh - Build all compatible EUI examples for a target chip
# Usage:
#   . $IDF_PATH/export.sh
#   ./build_all.sh [esp32s3|esp32|esp32s2]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
OUTPUT_DIR="$PROJECT_DIR/build_output"
CHIP="${1:-esp32s3}"

# ============================================================
# Example sets by profile
# ============================================================

# Default profile: SSD1306 I2C, 128x64, 1bpp
EXAMPLES_DEFAULT=(
    "animation_demo"  "basic_label"    "benchmark"
    "button_test"     "custom_widget"  "dialog_overlay"
    "list_nav"        "menu_system"    "page_buffer"
    "scene_view_demo"
)

# ST7306 profile: SPI, 400x300, 2bpp (landscape)
EXAMPLES_ST7306=(
    "desktop_launcher"
)

if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH not set. Source export.sh first:"
    echo "  . \$IDF_PATH/export.sh"
    exit 1
fi

cd "$PROJECT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "Chip target: $CHIP"
echo "Output dir:  $OUTPUT_DIR"
echo ""

FAILED=()
SUCCESS=()
GLOBAL_FAILED=0

# ============================================================
# Build pass: single profile, list of examples
# ============================================================
build_pass() {
    local profile="$1"
    local build_dir="$2"
    shift 2
    local examples=("$@")

    rm -rf "$build_dir"

    local profile_cmake_arg=""
    local profile_desc="default (SSD1306 128x64 1bpp)"
    if [ "$profile" != "default" ]; then
        profile_cmake_arg="-DEUI_PROFILE=$profile"
        profile_desc="$profile"
    fi

    echo "============================================"
    echo " Profile: $profile_desc"
    echo "============================================"

    # Initial configure with first example
    idf.py -B "$build_dir" -DEUI_EXAMPLE="${examples[0]}" $profile_cmake_arg set-target "$CHIP" > /dev/null 2>&1

    for example in "${examples[@]}"; do
        echo "  Building: $example ..."

        cd "$build_dir"
        cmake -DEUI_EXAMPLE="$example" "$PROJECT_DIR" > /dev/null 2>&1

        if cmake --build . -- -j"$(nproc)" > /dev/null 2>&1; then
            if [ -f "eui_example.bin" ]; then
                cp "eui_example.bin" "$OUTPUT_DIR/${example}_${CHIP}.bin"
                echo "    -> $OUTPUT_DIR/${example}_${CHIP}.bin"
            fi
            if [ -f "eui_example.elf" ]; then
                cp "eui_example.elf" "$OUTPUT_DIR/${example}_${CHIP}.elf"
            fi
            SUCCESS+=("$example")
        else
            FAILED+=("$example")
            echo "  FAILED: $example"
            GLOBAL_FAILED=1
        fi
        cd "$PROJECT_DIR"
    done
}

# ============================================================
# Pass 1: Default profile (SSD1306 I2C)
# ============================================================
build_pass "default" "$PROJECT_DIR/build_default" "${EXAMPLES_DEFAULT[@]}"

# ============================================================
# Pass 2: ST7306 SPI landscape (400x300 2bpp)
# ============================================================
build_pass "st7306_400x300" "$PROJECT_DIR/build_st7306" "${EXAMPLES_ST7306[@]}"

# ============================================================
# Summary
# ============================================================
echo ""
echo "============================================"
echo " Build Summary for $CHIP"
echo "============================================"
echo "Success: ${#SUCCESS[@]}"
for e in "${SUCCESS[@]}"; do echo "  + $e"; done
if [ ${#FAILED[@]} -gt 0 ]; then
    echo "Failed: ${#FAILED[@]}"
    for e in "${FAILED[@]}"; do echo "  - $e"; done
fi
echo ""
echo "Output: $OUTPUT_DIR/"
ls -lh "$OUTPUT_DIR"/*.bin 2>/dev/null || echo "  (no binaries)"

exit $GLOBAL_FAILED
