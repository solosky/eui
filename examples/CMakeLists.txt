set(EXAMPLES basic_label button_test list_nav menu_system dialog_overlay
             animation_demo custom_widget page_buffer benchmark color_demo
             scene_view_demo amiibo_demo)

foreach(ex ${EXAMPLES})
    add_executable(${ex} ${ex}.c)
    target_include_directories(${ex} PRIVATE ${CMAKE_BINARY_DIR}/include)
    target_link_libraries(${ex} PRIVATE eui_drv_raylib)
endforeach()
