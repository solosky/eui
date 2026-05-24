cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/../../../port/esp-idf/eui_port
    ${CMAKE_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/../../../third_party/tlsf
    ${CMAKE_SOURCE_DIR}/../../../third_party/motionc
)
project(ssd1306_demo)
