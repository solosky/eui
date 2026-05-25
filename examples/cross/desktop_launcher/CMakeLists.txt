# examples/cross/desktop_launcher/CMakeLists.txt
# 400x300 2bpp desktop launcher (standalone main, raylib or web)

if(EUI_TARGET_PORT STREQUAL "web")
    add_executable(desktop_launcher desktop_launcher.c)
    target_include_directories(desktop_launcher PRIVATE ${CMAKE_BINARY_DIR}/include)
    target_link_libraries(desktop_launcher PRIVATE eui)

    # Emscripten flags (not inherited from eui_port_web since we have our own main)
    target_link_options(desktop_launcher PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s EXPORTED_FUNCTIONS=[_main,_eui_web_push_key_event,_malloc,_free]"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=ccall,cwrap"
        "SHELL:-s EXIT_RUNTIME=0"
        "SHELL:-s INITIAL_MEMORY=16MB"
    )

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
