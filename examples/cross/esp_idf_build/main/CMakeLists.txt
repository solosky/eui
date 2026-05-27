if(NOT DEFINED EUI_EXAMPLE)
    set(EUI_EXAMPLE "basic_label" CACHE STRING "Example name to build")
endif()

if(NOT DEFINED EUI_PROFILE)
    set(EUI_PROFILE "default" CACHE STRING "Display profile: default, st7306_400x300")
endif()

# Profile: ST7306 SPI 400x300 2bpp (landscape, for desktop_launcher)
if(EUI_PROFILE STREQUAL "st7306_400x300")
    set(ST7306_DEFINES
        "EUI_DRV_ST7306=1"
        "CONFIG_EUI_EXAMPLE_DISPLAY_WIDTH=400"
        "CONFIG_EUI_EXAMPLE_DISPLAY_HEIGHT=300"
        "CONFIG_EUI_EXAMPLE_SPI_HOST=1"
        "CONFIG_EUI_EXAMPLE_SPI_MOSI=23"
        "CONFIG_EUI_EXAMPLE_SPI_SCLK=18"
        "CONFIG_EUI_EXAMPLE_SPI_CS=5"
        "CONFIG_EUI_EXAMPLE_SPI_DC=16"
        "CONFIG_EUI_EXAMPLE_SPI_RST=17"
    )
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

if(DEFINED ST7306_DEFINES)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${ST7306_DEFINES})
endif()
