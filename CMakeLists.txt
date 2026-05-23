cmake_minimum_required(VERSION 3.22)
project(eui VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_EXAMPLES "Build raylib examples" ON)
option(EUI_BUILD_CROSS_EXAMPLES "Build cross-platform examples" OFF)

# Compile-time config
set(EUI_COLOR_DEPTH 1 CACHE STRING "Color depth: 1, 4, 8, 16")
set(EUI_MEM_POOL_SIZE 8192 CACHE STRING "TLSF memory pool size in bytes")
set(EUI_MAX_VIEWS 8 CACHE STRING "Maximum concurrent views")
set(EUI_MAX_ANIMATIONS 8 CACHE STRING "Maximum concurrent animations")
set(EUI_MAX_WIDGETS 32 CACHE STRING "Maximum widget instances")
set(EUI_EVENT_QUEUE_SIZE 8 CACHE STRING "Event queue capacity")
set(EUI_MAX_OVERLAYS 4 CACHE STRING "Overlay stack depth")
set(EUI_FONT_ENABLE_U8G2 ON CACHE BOOL "Enable u8g2 font format support")
set(EUI_FONT_ENABLE_KERNING ON CACHE BOOL "Enable font kerning support")
set(EUI_FONT_ENABLE_MULTILINE ON CACHE BOOL "Enable multiline text support")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/eui/eui_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/eui/eui_config.h
)

# Build raylib if targeting raylib port (must be before src for eui_drv_raylib)
if(EUI_BUILD_EXAMPLES OR (EUI_BUILD_CROSS_EXAMPLES AND
   (NOT DEFINED EUI_TARGET_PORT OR EUI_TARGET_PORT STREQUAL "raylib")))
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/raylib)
endif()

add_subdirectory(third_party/tlsf)
set(MC_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(MC_BUILD_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/motionc)

add_subdirectory(src)

if(EUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Build raylib for legacy examples
if(EUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Cross-platform examples
if(EUI_BUILD_CROSS_EXAMPLES)
    if(NOT DEFINED EUI_TARGET_PORT)
        set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
    endif()
    set_property(CACHE EUI_TARGET_PORT PROPERTY STRINGS "raylib" "esp-idf" "nrf5")

    if(EUI_TARGET_PORT STREQUAL "raylib")
        set(BOOTSTRAP_LIB eui_port_raylib)
        add_subdirectory(port/raylib)
    elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
        set(BOOTSTRAP_LIB eui_port_esp_idf)
        add_subdirectory(port/esp-idf)
    elseif(EUI_TARGET_PORT STREQUAL "nrf5")
        set(BOOTSTRAP_LIB eui_port_nrf5)
        add_subdirectory(port/nrf5)
    else()
        message(FATAL_ERROR "Unknown EUI_TARGET_PORT: ${EUI_TARGET_PORT}")
    endif()

    add_subdirectory(examples/cross)
endif()
