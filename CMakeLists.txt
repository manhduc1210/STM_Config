cmake_minimum_required(VERSION 3.20)

project(stm32f407_baremetal C ASM)

set(TARGET_NAME stm32f407_bld)

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(COMMON_FLAGS
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -Werror
    -ffunction-sections
    -fdata-sections
    -fno-common
    -fmessage-length=0
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/STM32F407VGTx_FLASH.ld)

file(GLOB APP_SOURCES
    core/src/*.c
)

file(GLOB DRIVER_SOURCES
    app/*.c
)

add_executable(${TARGET_NAME}.elf
    ${APP_SOURCES}
    ${DRIVER_SOURCES}
    cmsis/device/st/stm32f4xx/system_stm32f4xx.c
    startup/startup_stm32f407xx.s
)

target_include_directories(${TARGET_NAME}.elf PRIVATE
    cmsis/core/include
    cmsis/device/st/stm32f4xx/include
    app
    core/include
)

target_compile_definitions(${TARGET_NAME}.elf PRIVATE
    STM32F407xx
)

target_compile_options(${TARGET_NAME}.elf PRIVATE
    ${COMMON_FLAGS}
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
)

target_link_options(${TARGET_NAME}.elf PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${TARGET_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            ${TARGET_NAME}.elf
            ${TARGET_NAME}.hex

    COMMAND ${CMAKE_OBJCOPY} -O binary
            ${TARGET_NAME}.elf
            ${TARGET_NAME}.bin

    COMMAND ${CMAKE_SIZE}
            ${TARGET_NAME}.elf
)