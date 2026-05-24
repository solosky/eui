if(DEFINED ENV{IDF_PATH} OR DEFINED IDF_PATH)
    idf_component_register(
        SRCS            ../eui_port_esp_idf.c
        INCLUDE_DIRS    .. ../include
        REQUIRES        driver freertos esp_timer eui
    )
endif()
