# examples/cross/desktop_launcher/CMakeLists.txt
# 400x300 2bpp desktop launcher (standalone main, raylib or web)

if(EUI_TARGET_PORT STREQUAL "web")
    add_executable(desktop_launcher desktop_launcher.c)
    target_include_directories(desktop_launcher PRIVATE ${CMAKE_BINARY_DIR}/include)
    target_link_libraries(desktop_launcher PRIVATE eui)

    if(COMMAND eui_web_add_html)
        eui_web_add_html(desktop_launcher "Desktop Launcher")
    endif()
else()
    add_executable(desktop_launcher desktop_launcher.c)
    target_include_directories(desktop_launcher PRIVATE
        ${CMAKE_BINARY_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(desktop_launcher PRIVATE eui eui_drv_raylib)
    if(UNIX)
        target_link_libraries(desktop_launcher PRIVATE m)
    endif()
endif()
