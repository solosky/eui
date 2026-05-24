# examples/cross/scene_view_demo/CMakeLists.txt

if(EUI_TARGET_PORT STREQUAL "raylib")
    set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/eui_example_config.h)
    add_custom_command(
        OUTPUT ${CONFIG_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DKCONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
            -DOUTPUT_HEADER=${CONFIG_HEADER}
            -P ${CMAKE_SOURCE_DIR}/tools/kconfig_to_header.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig
        COMMENT "Generating config header from Kconfig defaults"
    )
    set(CONFIG_GEN_TARGET generate_scene_view_demo_config)
    add_custom_target(${CONFIG_GEN_TARGET} DEPENDS ${CONFIG_HEADER})
endif()

add_executable(scene_view_demo scene_view_demo.c)
target_include_directories(scene_view_demo PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(scene_view_demo PRIVATE eui ${BOOTSTRAP_LIB})

if(EUI_TARGET_PORT STREQUAL "raylib")
    add_dependencies(scene_view_demo ${CONFIG_GEN_TARGET})
endif()
