cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/../../../port/esp-idf
    ${CMAKE_SOURCE_DIR}/../../..
)

if(NOT DEFINED EUI_ST7735_BUFFER_MODE)
    set(EUI_ST7735_BUFFER_MODE PAGE CACHE STRING "Buffer mode: FULL or PAGE")
endif()
add_compile_definitions(EUI_ST7735_BUFFER_${EUI_ST7735_BUFFER_MODE})

project(st7735_demo)
