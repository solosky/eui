add_library(eui STATIC
    eui_types.c
    eui_allocator.c
    eui_event.c
    eui_input.c
    eui_canvas.c
    eui_font.c
    eui_font_bdf.c
    eui_font_vlw.c
    eui_font_u8g2.c
    eui_font_builtin.c
    eui_font_wqy13.c
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
    driver/eui_drv_st7735.c
    driver/eui_drv_ili9341.c
    driver/eui_drv_buttons.c
    driver/eui_drv_encoder.c
    driver/eui_drv_xpt2046.c
    eui.c
)
target_include_directories(eui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/motionc/include
)
target_include_directories(eui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(eui PUBLIC tlsf mc)

if(EUI_BUILD_EXAMPLES)
add_library(eui_drv_raylib STATIC driver/eui_drv_raylib.c)
target_include_directories(eui_drv_raylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(eui_drv_raylib PUBLIC eui raylib)
if(UNIX)
    target_link_libraries(eui_drv_raylib PUBLIC m)
endif()
endif()
