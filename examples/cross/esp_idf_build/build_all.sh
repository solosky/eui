#!/bin/bash
# build_all.sh - Build all compatible EUI examples for a target chip
# Usage:
#   . $IDF_PATH/export.sh
#   ./build_all.sh [esp32s3|esp32|esp32s2]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"
OUTPUT_DIR="$PROJECT_DIR/build_output"
CHIP="${1:-esp32s3}"

# Examples requiring higher specs (skipped for 128x64 1bpp SSD1306)
SKIP_EXAMPLES=("amiibo_demo" "color_demo" "desktop_launcher")

EXAMPLES=(
    "animation_demo"  "basic_label"    "benchmark"
    "button_test"     "custom_widget"  "dialog_overlay"
    "list_nav"        "menu_system"    "page_buffer"
    "scene_view_demo"
)

if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH not set. Source export.sh first:"
    echo "  . \$IDF_PATH/export.sh"
    exit 1
fi

cd "$PROJECT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "Chip target: $CHIP"
echo "Project dir: $PROJECT_DIR"
echo "Build dir:   $BUILD_DIR"
echo "Output dir:  $OUTPUT_DIR"
echo ""

# ============================================================
# Step 1: Initial configure (sets target and IDF build env)
# ============================================================
rm -rf "$BUILD_DIR"
idf.py -B "$BUILD_DIR" -DEUI_EXAMPLE=basic_label set-target "$CHIP" 2>&1 | tail -1

echo "Initial configure done. Building examples..."

FAILED=()
SUCCESS=()
SKIPPED=()

build_example() {
    local example="$1"

    # Skip incompatible examples
    for skip in "${SKIP_EXAMPLES[@]}"; do
        if [ "$example" = "$skip" ]; then
            return 2
        fi
    done

    echo "  Building: $example ..."

    # Reconfigure cmake with new example name
    cd "$BUILD_DIR"
    cmake -DEUI_EXAMPLE="$example" "$PROJECT_DIR" > /dev/null 2>&1

    # Build (only recompiles changed main component, all IDF libs cached)
    if cmake --build . -- -j"$(nproc)" 2>&1 | tail -3; then
        # Copy output
        if [ -f "eui_example.bin" ]; then
            cp "eui_example.bin" "$OUTPUT_DIR/${example}_${CHIP}.bin"
            echo "    -> $OUTPUT_DIR/${example}_${CHIP}.bin"
        fi
        if [ -f "eui_example.elf" ]; then
            cp "eui_example.elf" "$OUTPUT_DIR/${example}_${CHIP}.elf"
        fi
        cd "$PROJECT_DIR"
        return 0
    else
        cd "$PROJECT_DIR"
        return 1
    fi
}

for example in "${EXAMPLES[@]}"; do
    build_example "$example"
    rc=$?
    if [ $rc -eq 0 ]; then
        SUCCESS+=("$example")
    elif [ $rc -eq 2 ]; then
        SKIPPED+=("$example")
        echo "  Skip: $example (incompatible with default profile)"
    else
        FAILED+=("$example")
        echo "  FAILED: $example"
    fi
done

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
echo "Skipped: ${#SKIPPED[@]}"
for e in "${SKIPPED[@]}"; do echo "  * $e"; done
echo ""
echo "Output: $OUTPUT_DIR/"
ls -lh "$OUTPUT_DIR"/*.bin 2>/dev/null || echo "  (no binaries)"

if [ ${#FAILED[@]} -gt 0 ]; then
    exit 1
fi
echo "All compatible examples built successfully."
