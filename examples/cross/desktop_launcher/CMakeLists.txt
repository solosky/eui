# examples/cross/desktop_launcher/CMakeLists.txt
# Standalone 400x300 2bpp desktop launcher demo (own main, direct raylib link)

add_executable(desktop_launcher desktop_launcher.c)

target_include_directories(desktop_launcher PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(desktop_launcher PRIVATE eui eui_drv_raylib)

if(UNIX)
    target_link_libraries(desktop_launcher PRIVATE m)
endif()
