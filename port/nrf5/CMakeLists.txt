if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT $ENV{NRF5_SDK_ROOT})

    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c eui_port_bootstrap.c)

    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
        ${NRF5_SDK_ROOT}/components
        ${NRF5_SDK_ROOT}/components/drivers_nrf/twi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/spi_master
        ${NRF5_SDK_ROOT}/components/drivers_nrf/delay
        ${NRF5_SDK_ROOT}/components/libraries/util
        ${NRF5_SDK_ROOT}/integration/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
        ${NRF5_SDK_ROOT}/modules/nrfx/hal
        ${NRF5_SDK_ROOT}/components/toolchain
    )

    set(NRF5_CHIP "NRF52832_XXAA" CACHE STRING "Target nRF5 chip variant")
    set(NRF5_FAMILY "NRF52" CACHE STRING "Target nRF5 family")

    target_compile_definitions(eui_port_nrf5 PRIVATE
        ${NRF5_FAMILY}
        ${NRF5_CHIP}
    )

    target_link_libraries(eui_port_nrf5 PUBLIC eui)
else()
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c eui_port_bootstrap.c)
    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(eui_port_nrf5 PUBLIC eui)
endif()
