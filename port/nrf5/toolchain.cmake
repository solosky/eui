# CMake toolchain file for nRF52832 (Cortex-M4) with ARM GCC
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=port/nrf5/toolchain.cmake ...

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prevent CMake from trying to link test executables (bare-metal has no OS)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# nRF52832 = Cortex-M4 with FPU
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(COMMON_FLAGS "${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")

# Use nosys specs for bare-metal syscall stubs
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs -Wl,--gc-sections")

# Chip defines (NRF5_CHIP / NRF5_FAMILY set from nrf5 port CMakeLists.txt)
add_compile_definitions(FLOAT_ABI_HARD)
