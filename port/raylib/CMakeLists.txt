# port/raylib/CMakeLists.txt
add_library(eui_port_raylib STATIC eui_port_bootstrap.c)
target_include_directories(eui_port_raylib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(eui_port_raylib PUBLIC eui eui_drv_raylib)
