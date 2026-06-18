cmake_minimum_required(VERSION 3.20)

project(stm32f407_baremetal C ASM)

set(TARGET_NAME stm32f407_blink)

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

function(add_stm32_app TARGET SOURCE)
    add_executable(${TARGET}.elf
        ${SOURCE}
        cmsis/device/st/stm32f4xx/system_stm32f4xx.c
        startup/startup_stm32f407xx.s
    )

    target_include_directories(${TARGET}.elf PRIVATE
        cmsis/core/include
        cmsis/device/st/stm32f4xx/include
    )

    target_compile_definitions(${TARGET}.elf PRIVATE
        STM32F407xx
    )

    target_compile_options(${TARGET}.elf PRIVATE
        ${COMMON_FLAGS}
        $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
    )

    target_link_options(${TARGET}.elf PRIVATE
        ${MCU_FLAGS}
        -T${LINKER_SCRIPT}
        --specs=nano.specs
        --specs=nosys.specs
        -Wl,-Map=${TARGET}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
    )

    add_custom_command(TARGET ${TARGET}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex
                ${TARGET}.elf
                ${TARGET}.hex

        COMMAND ${CMAKE_OBJCOPY} -O binary
                ${TARGET}.elf
                ${TARGET}.bin

        COMMAND ${CMAKE_SIZE}
                ${TARGET}.elf
    )
endfunction()

add_stm32_app(${TARGET_NAME} app/main.c)
add_stm32_app(stm32_app_v1_1_0 app/app_v1_1_0.c)
add_stm32_app(stm32_app_v1_2_0 app/app_v1_2_0.c)
