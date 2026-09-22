cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/../../..
    ${CMAKE_SOURCE_DIR}/../../../port/esp-idf/eui_port
    ${CMAKE_SOURCE_DIR}/../../../third_party/tlsf
    ${CMAKE_SOURCE_DIR}/../../../third_party/motionc
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(eui_example)
