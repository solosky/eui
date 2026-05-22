if(DEFINED ENV{IDF_PATH} OR DEFINED IDF_PATH)
    idf_component_register(
        SRCS            eui_port_esp_idf.c
        INCLUDE_DIRS    .
        REQUIRES        driver freertos
    )
else()
    add_library(eui_port_esp_idf STATIC eui_port_esp_idf.c)
    target_include_directories(eui_port_esp_idf PUBLIC .)
    target_link_libraries(eui_port_esp_idf PUBLIC eui)
endif()
