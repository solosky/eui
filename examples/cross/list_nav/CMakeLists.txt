# examples/cross/list_nav/CMakeLists.txt

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
    set(CONFIG_GEN_TARGET generate_list_nav_config)
    add_custom_target(${CONFIG_GEN_TARGET} DEPENDS ${CONFIG_HEADER})
endif()

add_executable(list_nav list_nav.c)
target_include_directories(list_nav PRIVATE
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(list_nav PRIVATE eui ${BOOTSTRAP_LIB})

if(EUI_TARGET_PORT STREQUAL "raylib")
    add_dependencies(list_nav ${CONFIG_GEN_TARGET})
endif()
