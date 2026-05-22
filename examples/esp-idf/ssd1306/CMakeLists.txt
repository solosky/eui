cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/../../../port/esp-idf
    ${CMAKE_SOURCE_DIR}/../../..
)
project(ssd1306_demo)
