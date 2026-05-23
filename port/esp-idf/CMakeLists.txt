if(DEFINED ENV{IDF_PATH} OR DEFINED IDF_PATH)
    idf_component_register(
        SRCS            eui_port_esp_idf.c eui_port_bootstrap.c
        INCLUDE_DIRS    . ${CMAKE_SOURCE_DIR}/include
        REQUIRES        driver freertos
    )
else()
    add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c eui_port_bootstrap.c)
    target_include_directories(eui_port_esp_idf PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(eui_port_esp_idf PUBLIC eui)
endif()
