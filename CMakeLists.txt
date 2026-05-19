cmake_minimum_required(VERSION 3.16)
project(eui VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_EXAMPLES "Build raylib examples" ON)

# Compile-time config
set(EUI_COLOR_DEPTH 1 CACHE STRING "Color depth: 1, 4, 8, 16")
set(EUI_MEM_POOL_SIZE 8192 CACHE STRING "TLSF memory pool size in bytes")
set(EUI_MAX_VIEWS 8 CACHE STRING "Maximum concurrent views")
set(EUI_MAX_ANIMATIONS 8 CACHE STRING "Maximum concurrent animations")
set(EUI_MAX_WIDGETS 32 CACHE STRING "Maximum widget instances")
set(EUI_EVENT_QUEUE_SIZE 8 CACHE STRING "Event queue capacity")
set(EUI_MAX_OVERLAYS 4 CACHE STRING "Overlay stack depth")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/eui/eui_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/eui/eui_config.h
)

add_subdirectory(third_party/tlsf)
set(MC_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(MC_BUILD_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/motionc)
add_subdirectory(src)

if(EUI_BUILD_TESTS)
    enable_testing()
    find_package(raylib QUIET)
    add_subdirectory(test)
endif()

if(EUI_BUILD_EXAMPLES)
    find_package(raylib QUIET)
    add_subdirectory(examples)
endif()
