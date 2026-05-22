if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT $ENV{NRF5_SDK_ROOT})

    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)

    target_include_directories(eui_port_nrf5 PUBLIC
        .
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

    target_compile_definitions(eui_port_nrf5 PRIVATE
        NRF52
        NRF52832_XXAA
    )

    target_link_libraries(eui_port_nrf5 PUBLIC eui)
else()
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c)
    target_include_directories(eui_port_nrf5 PUBLIC .)
    target_link_libraries(eui_port_nrf5 PUBLIC eui)
endif()
