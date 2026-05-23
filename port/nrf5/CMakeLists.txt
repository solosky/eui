# NRF5_SDK_ROOT from env or cache, persisted across cmake re-runs
if(DEFINED ENV{NRF5_SDK_ROOT})
    set(NRF5_SDK_ROOT "$ENV{NRF5_SDK_ROOT}" CACHE PATH "nRF5 SDK root directory")
endif()
if(NOT DEFINED NRF5_SDK_ROOT OR NRF5_SDK_ROOT STREQUAL "")
    message(FATAL_ERROR "NRF5_SDK_ROOT is not set. "
        "Export it: export NRF5_SDK_ROOT=/path/to/nRF5_SDK_17.1.0_ddde560\n"
        "Or pass via cmake: cmake -DNRF5_SDK_ROOT=/path/to/nrf5-sdk ...")
endif()

    # ── nRF5 SDK support library ────────────────────────────
    add_library(nrf5_sdk STATIC
        ${NRF5_SDK_ROOT}/modules/nrfx/mdk/system_nrf52.c
        ${NRF5_SDK_ROOT}/modules/nrfx/mdk/gcc_startup_nrf52.S
        ${NRF5_SDK_ROOT}/modules/nrfx/soc/nrfx_atomic.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/prs/nrfx_prs.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_twi.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_twim.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_spim.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_clock.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_power.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_timer.c
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_rtc.c
        ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_twi.c
        ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_spi.c
        ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_clock.c
        ${NRF5_SDK_ROOT}/integration/nrfx/legacy/nrf_drv_power.c
        ${NRF5_SDK_ROOT}/components/libraries/timer/app_timer.c
        ${NRF5_SDK_ROOT}/components/libraries/util/app_error.c
        ${NRF5_SDK_ROOT}/components/libraries/util/app_error_handler_gcc.c
        ${NRF5_SDK_ROOT}/components/libraries/util/app_error_weak.c
        ${NRF5_SDK_ROOT}/components/libraries/util/app_util_platform.c
        ${NRF5_SDK_ROOT}/components/libraries/util/nrf_assert.c
        ${NRF5_SDK_ROOT}/components/libraries/atomic/nrf_atomic.c
        ${NRF5_SDK_ROOT}/components/libraries/balloc/nrf_balloc.c
        ${NRF5_SDK_ROOT}/components/libraries/memobj/nrf_memobj.c
        ${NRF5_SDK_ROOT}/components/libraries/ringbuf/nrf_ringbuf.c
        ${NRF5_SDK_ROOT}/components/libraries/strerror/nrf_strerror.c
        ${NRF5_SDK_ROOT}/external/fprintf/nrf_fprintf.c
        ${NRF5_SDK_ROOT}/external/fprintf/nrf_fprintf_format.c
        ${NRF5_SDK_ROOT}/components/boards/boards.c
    )

    set(SDK_INC_DIRS
        .
        ${NRF5_SDK_ROOT}/components
        ${NRF5_SDK_ROOT}/components/boards
        ${NRF5_SDK_ROOT}/components/drivers_nrf/nrf_soc_nosd
        ${NRF5_SDK_ROOT}/components/libraries/util
        ${NRF5_SDK_ROOT}/components/libraries/delay
        ${NRF5_SDK_ROOT}/components/libraries/timer
        ${NRF5_SDK_ROOT}/components/libraries/log
        ${NRF5_SDK_ROOT}/components/libraries/log/src
        ${NRF5_SDK_ROOT}/components/libraries/experimental_section_vars
        ${NRF5_SDK_ROOT}/components/libraries/atomic
        ${NRF5_SDK_ROOT}/components/libraries/balloc
        ${NRF5_SDK_ROOT}/components/libraries/memobj
        ${NRF5_SDK_ROOT}/components/libraries/ringbuf
        ${NRF5_SDK_ROOT}/components/libraries/strerror
        ${NRF5_SDK_ROOT}/components/toolchain
        ${NRF5_SDK_ROOT}/components/toolchain/cmsis/include
        ${NRF5_SDK_ROOT}/external/fprintf
        ${NRF5_SDK_ROOT}/integration/nrfx
        ${NRF5_SDK_ROOT}/integration/nrfx/legacy
        ${NRF5_SDK_ROOT}/modules/nrfx
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/include
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src
        ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/prs
        ${NRF5_SDK_ROOT}/modules/nrfx/hal
        ${NRF5_SDK_ROOT}/modules/nrfx/mdk
        ${NRF5_SDK_ROOT}/modules/nrfx/soc
    )

    target_include_directories(nrf5_sdk PUBLIC ${SDK_INC_DIRS})

    set(NRF5_CHIP "NRF52832_XXAA" CACHE STRING "Target nRF5 chip variant")
    set(NRF5_FAMILY "NRF52" CACHE STRING "Target nRF5 family")

    set(SDK_DEFINES
        ${NRF5_FAMILY}
        ${NRF5_CHIP}
        NRF52832
        BOARD_PCA10040
        BSP_DEFINES_ONLY
        CONFIG_GPIO_AS_PINRESET
    )

    target_compile_definitions(nrf5_sdk PRIVATE ${SDK_DEFINES})

    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")

    # ── EUI nrf5 port library ───────────────────────────────
    add_library(eui_port_nrf5 STATIC eui_port_nrf5.c eui_port_bootstrap.c)

    target_include_directories(eui_port_nrf5 PUBLIC
        .
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${SDK_INC_DIRS}
    )

    target_compile_definitions(eui_port_nrf5 PRIVATE ${SDK_DEFINES})

    target_link_libraries(eui_port_nrf5 PUBLIC eui nrf5_sdk)
