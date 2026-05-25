# configs/web/canvas_400x300_2bpp.cmake
# Desktop launcher: 400x300 2bpp via Emscripten/web
set(EUI_TARGET_PORT      "web")
set(EUI_COLOR_DEPTH      2)
set(EUI_DISPLAY_DRIVER   "web")
set(EUI_DISPLAY_WIDTH    400)
set(EUI_DISPLAY_HEIGHT   300)
set(EUI_BUFFER_MODE      "full")
set(EUI_MEM_POOL_SIZE    65536)
set(EUI_MAX_VIEWS        16)
set(EUI_INPUT_DRIVER     "web")
set(EUI_CANVAS_SCALE     2)
