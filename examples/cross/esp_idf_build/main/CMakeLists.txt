if(NOT DEFINED EUI_EXAMPLE)
    set(EUI_EXAMPLE "basic_label" CACHE STRING "Example name to build")
endif()

idf_component_register(
    SRCS
        "../../../../port/esp-idf/eui_port_bootstrap.c"
        "../../${EUI_EXAMPLE}/${EUI_EXAMPLE}.c"
    INCLUDE_DIRS
        "."
        "../../../../port/esp-idf"
        "../../../../port/esp-idf/include"
    REQUIRES
        eui
        eui_port
)
