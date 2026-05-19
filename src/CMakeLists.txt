add_library(eui STATIC
    eui_types.c
    eui_allocator.c
    eui_event.c
    eui_input.c
    eui_canvas.c
    eui_font.c
    eui_font_bdf.c
    eui_font_vlw.c
    eui_font_builtin.c
    eui_view.c
    eui_view_dispatcher.c
    eui_widget.c
    eui_widget_button.c
    eui_widget_label.c
    eui_widget_menu.c
    eui_widget_list.c
    eui_widget_progress.c
    eui_widget_slider.c
    eui_widget_scroll.c
    eui_widget_dialog.c
    eui_scene.c
    eui_anim.c
    driver/eui_drv_ssd1306.c
    driver/eui_drv_sh1106.c
    eui.c
)
target_include_directories(eui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/motionc/include
)
target_link_libraries(eui PUBLIC tlsf mc)

find_package(raylib QUIET)
if(raylib_FOUND)
    add_library(eui_hal_raylib STATIC hal/eui_hal_raylib.c)
    target_include_directories(eui_hal_raylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(eui_hal_raylib PUBLIC eui raylib)
endif()
