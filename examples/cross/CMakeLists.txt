# examples/cross/CMakeLists.txt
# Cross-platform examples — constraint-checked against active config profile

function(add_cross_example name)
    set(example_dir "${CMAKE_CURRENT_SOURCE_DIR}/${name}")

    # Skip if requirements not met
    if(EXISTS "${example_dir}/requirements.cmake")
        include("${example_dir}/requirements.cmake")

        if(DEFINED EXAMPLE_REQUIRES_COLOR_DEPTH_MIN AND EUI_COLOR_DEPTH LESS EXAMPLE_REQUIRES_COLOR_DEPTH_MIN)
            message(WARNING "Skipping ${name}: requires color_depth >= ${EXAMPLE_REQUIRES_COLOR_DEPTH_MIN} (got ${EUI_COLOR_DEPTH})")
            return()
        endif()
        if(DEFINED EXAMPLE_REQUIRES_WIDTH_MIN AND EUI_DISPLAY_WIDTH LESS EXAMPLE_REQUIRES_WIDTH_MIN)
            message(WARNING "Skipping ${name}: requires width >= ${EXAMPLE_REQUIRES_WIDTH_MIN} (got ${EUI_DISPLAY_WIDTH})")
            return()
        endif()
        if(DEFINED EXAMPLE_REQUIRES_HEIGHT_MIN AND EUI_DISPLAY_HEIGHT LESS EXAMPLE_REQUIRES_HEIGHT_MIN)
            message(WARNING "Skipping ${name}: requires height >= ${EXAMPLE_REQUIRES_HEIGHT_MIN} (got ${EUI_DISPLAY_HEIGHT})")
            return()
        endif()
    endif()

    add_subdirectory(${name})

    if(EUI_TARGET_PORT STREQUAL "web")
        eui_web_add_html(${name} "${name}")
    endif()

    # Ensure profile config header is generated before building any cross example
    if(TARGET gen_profile_config)
        add_dependencies(${name} gen_profile_config)
    endif()
endfunction()

add_cross_example(basic_label)
add_cross_example(button_test)
add_cross_example(list_nav)
add_cross_example(menu_system)
add_cross_example(dialog_overlay)
add_cross_example(animation_demo)
add_cross_example(custom_widget)
add_cross_example(page_buffer)
add_cross_example(benchmark)
add_cross_example(scene_view_demo)
add_cross_example(color_demo)
add_cross_example(amiibo_demo)
add_cross_example(desktop_launcher)
