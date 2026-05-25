cmake_minimum_required(VERSION 3.22)

if(ESP_PLATFORM)
    file(GLOB EUI_CORE_SRCS src/*.c)
    file(GLOB EUI_WIDGET_SRCS src/widget/*.c)
    file(GLOB EUI_DRIVER_SRCS src/driver/*.c)
    list(REMOVE_ITEM EUI_DRIVER_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/driver/eui_drv_raylib.c")
    idf_component_register(
        SRCS ${EUI_CORE_SRCS} ${EUI_WIDGET_SRCS} ${EUI_DRIVER_SRCS}
        INCLUDE_DIRS include third_party/motionc/include port/esp-idf/include
        REQUIRES driver freertos tlsf motionc
    )
    return()
endif()

project(eui VERSION 1.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(EUI_BUILD_TESTS "Build unit tests" ON)
option(EUI_BUILD_CROSS_EXAMPLES "Build cross-platform examples" OFF)

# Compile-time config
set(EUI_COLOR_DEPTH 1 CACHE STRING "Color depth: 1, 2, 4, 8, 16")
set(EUI_MEM_POOL_SIZE 8192 CACHE STRING "TLSF memory pool size in bytes")
set(EUI_MAX_VIEWS 8 CACHE STRING "Maximum concurrent views")
set(EUI_MAX_ANIMATIONS 8 CACHE STRING "Maximum concurrent animations")
set(EUI_MAX_WIDGETS 32 CACHE STRING "Maximum widget instances")
set(EUI_EVENT_QUEUE_SIZE 8 CACHE STRING "Event queue capacity")
set(EUI_MAX_OVERLAYS 4 CACHE STRING "Overlay stack depth")
set(EUI_FONT_ENABLE_U8G2 ON CACHE BOOL "Enable u8g2 font format support")
set(EUI_FONT_ENABLE_KERNING ON CACHE BOOL "Enable font kerning support")
set(EUI_FONT_ENABLE_MULTILINE ON CACHE BOOL "Enable multiline text support")

# Load config profile BEFORE configure_file() so that profile values
# (EUI_COLOR_DEPTH, EUI_MEM_POOL_SIZE, EUI_FONT_ENABLE_*, etc.)
# override the defaults when generating eui_config.h.
if(DEFINED EUI_CONFIG_PROFILE)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${EUI_CONFIG_PROFILE}")
        message(FATAL_ERROR "Config profile not found: ${EUI_CONFIG_PROFILE}")
    endif()
    message(STATUS "Using config profile: ${EUI_CONFIG_PROFILE}")
    include("${CMAKE_SOURCE_DIR}/${EUI_CONFIG_PROFILE}")
    if(DEFINED EUI_TARGET_PORT)
        set(EUI_TARGET_PORT "${EUI_TARGET_PORT}" CACHE STRING "Target port: raylib, esp-idf, nrf5, web")
    endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/eui/eui_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/eui/eui_config.h
)

# Build raylib when needed (must be before src for eui_drv_raylib)
# - raylib/desktop examples
# - cross-platform raylib port
# - unit tests (e.g. test_view_transition_render)
if((EUI_BUILD_CROSS_EXAMPLES AND (NOT DEFINED EUI_TARGET_PORT OR EUI_TARGET_PORT STREQUAL "raylib")) OR
   EUI_BUILD_TESTS)
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

# Cross-platform examples
if(EUI_BUILD_CROSS_EXAMPLES)
    # Config profile header generation (profile was already loaded above)
    if(DEFINED EUI_CONFIG_PROFILE)
        # Generate eui_profile_config.h from profile variables
        set(PROFILE_VARS "EUI_DISPLAY_DRIVER;EUI_DISPLAY_WIDTH;EUI_DISPLAY_HEIGHT;EUI_BUFFER_MODE;EUI_CANVAS_SCALE;EUI_PIN_SPI_HOST;EUI_PIN_MOSI;EUI_PIN_SCLK;EUI_PIN_CS;EUI_PIN_DC;EUI_PIN_RST;EUI_PIN_BL;EUI_PIN_I2C_PORT;EUI_PIN_SDA;EUI_PIN_SCL;EUI_PIN_I2C_FREQ;EUI_PIN_I2C_ADDR;EUI_INPUT_DRIVER;EUI_PIN_BTN_UP;EUI_PIN_BTN_DOWN;EUI_PIN_BTN_OK;EUI_PIN_BTN_BACK;EUI_DRV_ST7306")
        set(PROFILE_HEADER "${CMAKE_BINARY_DIR}/include/eui/eui_profile_config.h")
        # Build -DVAR=value args for each defined variable
        set(PROFILE_DEFINES "")
        foreach(var_name ${PROFILE_VARS})
            if(DEFINED ${var_name})
                list(APPEND PROFILE_DEFINES "-D${var_name}=${${var_name}}")
            endif()
        endforeach()
        add_custom_command(
            OUTPUT ${PROFILE_HEADER}
            COMMAND ${CMAKE_COMMAND}
                ${PROFILE_DEFINES}
                -DCONFIG_VARS="${PROFILE_VARS}"
                -DOUTPUT_HEADER=${PROFILE_HEADER}
                -P ${CMAKE_SOURCE_DIR}/tools/config_to_header.cmake
            COMMENT "Generating eui_profile_config.h from config profile"
        )
        add_custom_target(gen_profile_config DEPENDS ${PROFILE_HEADER})
    endif()

    if(NOT DEFINED EUI_TARGET_PORT)
        set(EUI_TARGET_PORT "raylib" CACHE STRING "Target port: raylib, esp-idf, nrf5")
    endif()
    set_property(CACHE EUI_TARGET_PORT PROPERTY STRINGS "raylib" "esp-idf" "nrf5" "web")

    if(EUI_TARGET_PORT STREQUAL "raylib")
        set(BOOTSTRAP_LIB eui_port_raylib)
        add_subdirectory(port/raylib)
    elseif(EUI_TARGET_PORT STREQUAL "esp-idf")
        set(BOOTSTRAP_LIB eui_port_esp_idf)
        add_subdirectory(port/esp-idf)
    elseif(EUI_TARGET_PORT STREQUAL "nrf5")
        set(BOOTSTRAP_LIB eui_port_nrf5)
        add_subdirectory(port/nrf5)
    elseif(EUI_TARGET_PORT STREQUAL "web")
        set(BOOTSTRAP_LIB eui_port_web)
        add_subdirectory(port/web)
    else()
        message(FATAL_ERROR "Unknown EUI_TARGET_PORT: ${EUI_TARGET_PORT}")
    endif()

    add_subdirectory(examples/cross)

    # Generate index.html for web port (scans built .html files at build time).
    if(EUI_TARGET_PORT STREQUAL "web")
        add_custom_target(web_index ALL
            COMMAND ${CMAKE_COMMAND}
                -DBUILD_DIR="${CMAKE_BINARY_DIR}"
                -DOUTPUT="${CMAKE_BINARY_DIR}/index.html"
                -P "${CMAKE_SOURCE_DIR}/port/web/gen_index.cmake"
            COMMENT "Generating index.html"
            VERBATIM
        )
        get_property(web_targets GLOBAL PROPERTY WEB_EXAMPLE_TARGETS)
        if(web_targets)
            add_dependencies(web_index ${web_targets})
        endif()
    endif()
endif()
