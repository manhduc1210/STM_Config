# Project Overview

## 1. Overview

This project is a bare-metal firmware project for the STM32F407. It focuses on a bootloader that can receive OTA commands over UART2 and write data into the flash region reserved for the application. The firmware is currently built with CMake and GCC ARM Embedded (`arm-none-eabi`), producing the artifacts `stm32f407_bld.elf`, `stm32f407_bld.hex`, and `stm32f407_bld.bin`.

Main hardware target:

- MCU: STM32F407VG / Cortex-M4F.
- Internal flash: 1 MB, starting at `0x08000000`.
- SRAM: 128 KB, starting at `0x20000000`.
- UART used for OTA: USART2 on PA2/PA3, baud rate 115200.
- Status LED: PD13.

The current target build does not use an RTOS. The `FreeRTOS-Kernel` directory is present in the repository as dependency/vendor source, but `CMakeLists.txt` does not currently add FreeRTOS to the executable, and no FreeRTOS APIs are called from `app/` or `core/`.

## 2. Directory Structure

```text
.
|-- app/
|   |-- main.c
|   |-- boot_app.c
|   `-- boot_app.h
|-- core/
|   |-- include/
|   |   |-- flash_manager.h
|   |   |-- ota_protocol.h
|   |   `-- uart_drv.h
|   `-- src/
|       |-- flash_manager.c
|       |-- ota_protocol.c
|       `-- uart_drv.c
|-- cmsis/
|   |-- core/include/
|   `-- device/st/stm32f4xx/
|-- startup/
|   `-- startup_stm32f407xx.s
|-- linker/
|   `-- STM32F407VGTx_FLASH.ld
|-- FreeRTOS-Kernel/
|-- test/
|   |-- send_hello.py
|   |-- test_erase.py
|   `-- test_write_chunk.py
|-- .vscode/
|   |-- tasks.json
|   `-- launch.json
|-- CMakeLists.txt
|-- arm-none-eabi-toolchain.cmake
`-- README.md
```

Main roles:

- `app/`: firmware entry point and application jump logic.
- `core/`: peripheral services/drivers and the OTA protocol.
- `cmsis/`: CMSIS core/device headers and `SystemInit`.
- `startup/`: vector table, reset handler, `.data`/`.bss` initialization, and the call to `main`.
- `linker/`: memory map and section layout.
- `test/`: Python scripts that send OTA frames over serial from a PC.
- `FreeRTOS-Kernel/`: FreeRTOS vendor source, not currently linked into the firmware.
- `.vscode/`: VS Code build/flash/debug tasks.

## 3. Build System

### Toolchain

`arm-none-eabi-toolchain.cmake` configures the cross compiler:

- `CMAKE_SYSTEM_NAME Generic`
- `CMAKE_SYSTEM_PROCESSOR arm`
- C compiler: `arm-none-eabi-gcc`
- ASM compiler: `arm-none-eabi-gcc`
- objcopy: `arm-none-eabi-objcopy`
- size: `arm-none-eabi-size`
- C standard: C11

### CMake Target

`CMakeLists.txt` creates this target:

```text
stm32f407_bld.elf
```

Sources included in the target:

- All `core/src/*.c`
- All `app/*.c`
- `cmsis/device/st/stm32f4xx/system_stm32f4xx.c`
- `startup/startup_stm32f407xx.s`

Include directories:

- `cmsis/core/include`
- `cmsis/device/st/stm32f4xx/include`
- `app`
- `core/include`

Main compile definition:

```c
STM32F407xx
```

MCU flags:

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

Common flags:

```text
-Wall
-Wextra
-Werror
-ffunction-sections
-fdata-sections
-fno-common
-fmessage-length=0
```

Link options:

- Linker script: `linker/STM32F407VGTx_FLASH.ld`
- Newlib nano: `--specs=nano.specs`
- No syscalls: `--specs=nosys.specs`
- Garbage collect unused sections: `-Wl,--gc-sections`
- Generate map file: `stm32f407_bld.map`
- Print memory usage.

Post-build outputs:

- `stm32f407_bld.hex`
- `stm32f407_bld.bin`
- size report from `arm-none-eabi-size`

### Build Commands

According to `README.md`:

```sh
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="$PWD/arm-none-eabi-toolchain.cmake"
cmake --build build
```

Flash with OpenOCD:

```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/stm32f407_bld.elf verify reset exit"
```

## 4. Memory Layout

### Linker Script

`linker/STM32F407VGTx_FLASH.ld` declares:

```text
FLASH  : ORIGIN = 0x08000000, LENGTH = 1024K
RAM    : ORIGIN = 0x20000000, LENGTH = 128K
CCMRAM : ORIGIN = 0x10000000, LENGTH = 64K
```

Stack top:

```text
_estack = ORIGIN(RAM) + LENGTH(RAM)
```

Main sections:

- `.isr_vector` is placed at the start of flash.
- `.text`, `.rodata`, `.ARM.extab`, and `.ARM.exidx` are placed in flash.
- `.data` runs in RAM but is loaded from flash.
- `.bss` is placed in RAM.
- `libc.a`, `libm.a`, and `libgcc.a` are discarded by the linker script.

### Bootloader/Application Flash Layout

The bootloader is linked to run from `0x08000000`.

The application slot is defined in code:

```c
APP_START_ADDRESS = 0x08020000
APP_END_ADDRESS   = 0x080EFFFF
```

Practical meaning:

- Region `0x08000000` up to before `0x08020000`: reserved for the bootloader.
- Region `0x08020000` to `0x080EFFFF`: reserved for the application image.
- Sector 11 (`0x080E0000` to `0x080FFFFF`) is noted as not fully erased because metadata is expected to be kept at `0x080F0000`.

Note: `APP_END_ADDRESS` is currently `0x080EFFFF`, but `flash_erase_app_region()` erases sectors 5 through 10, which only covers up to `0x080DFFFF`. Therefore, the region from `0x080E0000` to `0x080EFFFF` is valid for write/verify according to the address range check, but it is not erased by the current erase function. This should be synchronized when the final metadata/application layout is decided.

## 5. Startup and CMSIS

`startup/startup_stm32f407xx.s` is the GCC startup file for STM32F407:

1. Sets the stack pointer from `_estack`.
2. Calls `SystemInit`.
3. Copies `.data` from flash to RAM.
4. Zeroes `.bss`.
5. Calls `__libc_init_array`.
6. Calls `main`.
7. Provides the complete vector table for STM32F407.
8. Defines default interrupt handlers as weak aliases to `Default_Handler`.

`cmsis/device/st/stm32f4xx/system_stm32f4xx.c` keeps `SystemCoreClock = 16000000` by default. `SystemInit()` mainly enables FPU access if the compiler is configured to use the FPU and does not configure a new PLL. Therefore, the system currently relies on the default 16 MHz HSI clock, matching the UART driver comment that 115200 baud is configured for a 16 MHz APB1 clock.

## 6. Main Runtime Flow

`app/main.c`:

```c
int main(void)
{
    gpio_init();
    uart2_init();

    while (1)
    {
        ota_process_once();
        GPIOD->ODR ^= (1U << LED_ORANGE_PIN);
    }
}
```

Current flow:

1. Initializes GPIO for LED PD13.
2. Initializes UART2.
3. Loops forever:
   - Calls `ota_process_once()` to read and process exactly one OTA frame.
   - Toggles LED PD13 after the frame is processed.

Because UART reads are blocking, the LED only toggles after a complete frame has been received and processed. The firmware currently has no timeout or non-blocking receive path.

## 7. UART Driver

Files:

- `core/include/uart_drv.h`
- `core/src/uart_drv.c`

API:

```c
void uart2_init(void);
void uart2_write_byte(uint8_t data);
uint8_t uart2_read_byte_blocking(void);
void uart2_write(const uint8_t *data, uint32_t len);
void uart2_read_blocking(uint8_t *data, uint32_t len);
```

UART2 configuration:

- Enables GPIOA clock.
- Enables USART2 clock.
- Configures PA2/PA3 as alternate function pins.
- Uses AF7 for USART2.
- Sets `USART2->BRR = 0x008B`, corresponding to 115200 baud when APB1 is 16 MHz.
- Enables TE, RE, and UE.

I/O model:

- Byte write waits for TXE (`USART2->SR bit 7`).
- Byte read waits for RXNE (`USART2->SR bit 5`).
- Buffer read/write functions are implemented as blocking byte loops.

## 8. OTA Protocol

Files:

- `core/include/ota_protocol.h`
- `core/src/ota_protocol.c`

### Frame Format

`ota_frame_t` is packed:

```c
typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t  version;
    uint8_t  command;
    uint32_t sequence;
    uint32_t offset;
    uint16_t length;
    uint32_t crc32;
} ota_frame_t;
```

The current header size is 18 bytes.

Fields:

- `magic`: must be `0x4F54`.
- `version`: must be `0x01`.
- `command`: OTA command code.
- `sequence`: frame sequence number, echoed back in the response.
- `offset`: offset from `APP_START_ADDRESS` when writing a chunk.
- `length`: payload length.
- `crc32`: field exists but is not currently validated.

### Commands

```c
CMD_HELLO       = 0x01
CMD_ERASE_APP   = 0x04
CMD_WRITE_CHUNK = 0x05
CMD_ACK         = 0x80
CMD_NACK        = 0x81
```

### Status/Error Codes

```c
OTA_OK                 = 0x0000
OTA_ERR_INVALID_CMD    = 0x1003
OTA_ERR_INVALID_LENGTH = 0x1004
OTA_ERR_FLASH_ERASE    = 0x1008
OTA_ERR_FLASH_WRITE    = 0x1009
OTA_ERR_VERIFY         = 0x100A
```

### Response Format

Responses also use `ota_frame_t` as the header:

- `command`: `CMD_ACK` or `CMD_NACK`.
- `sequence`: request sequence.
- `offset`: 0.
- `length`: 2 bytes.
- payload: `uint16_t status`.
- `crc32`: 0.

### Processing Flow

`ota_process_once()`:

1. Blocking-reads an 18-byte header from UART2.
2. Checks `magic`.
3. Checks `version`.
4. Dispatches by `command`:
   - `CMD_HELLO`: returns ACK.
   - `CMD_ERASE_APP`: erases the application region.
   - `CMD_WRITE_CHUNK`: reads payload, writes flash, verifies, and returns ACK/NACK.
   - default: returns NACK with invalid command.

`CMD_WRITE_CHUNK`:

1. Checks that `length != 0` and `length <= OTA_MAX_PAYLOAD`.
2. Blocking-reads payload into `g_payload`.
3. Calculates the destination address:

```c
write_addr = APP_START_ADDRESS + frame->offset;
```

4. Calls `flash_write_bytes`.
5. Calls `flash_verify_bytes`.
6. Returns ACK if both steps succeed.

Current limitations:

- `crc32` is not used for integrity checking yet.
- There is no resynchronization if the byte stream becomes misaligned.
- There is no UART timeout.
- There is no boot/jump-app command in the current OTA protocol.

## 9. Flash Manager

Files:

- `core/include/flash_manager.h`
- `core/src/flash_manager.c`

API:

```c
flash_status_t flash_erase_app_region(void);
flash_status_t flash_write_bytes(uint32_t address, const uint8_t *data, uint32_t len);
flash_status_t flash_verify_bytes(uint32_t address, const uint8_t *data, uint32_t len);
```

Status:

```c
FLASH_OK
FLASH_ERR_INVALID_ADDRESS
FLASH_ERR_INVALID_LENGTH
FLASH_ERR_ERASE
FLASH_ERR_WRITE
FLASH_ERR_VERIFY
```

### Erase

`flash_erase_app_region()`:

1. Unlocks flash with these keys:
   - `0x45670123`
   - `0xCDEF89AB`
2. Clears common error flags.
3. Erases sectors 5 through 10.
4. Locks flash.
5. Returns `FLASH_OK`.

STM32F407 1 MB flash sector map in the code:

```text
Sector 0 : 0x08000000 - 0x08003FFF  16 KB
Sector 1 : 0x08004000 - 0x08007FFF  16 KB
Sector 2 : 0x08008000 - 0x0800BFFF  16 KB
Sector 3 : 0x0800C000 - 0x0800FFFF  16 KB
Sector 4 : 0x08010000 - 0x0801FFFF  64 KB
Sector 5 : 0x08020000 - 0x0803FFFF 128 KB
Sector 6 : 0x08040000 - 0x0805FFFF 128 KB
Sector 7 : 0x08060000 - 0x0807FFFF 128 KB
Sector 8 : 0x08080000 - 0x0809FFFF 128 KB
Sector 9 : 0x080A0000 - 0x080BFFFF 128 KB
Sector 10: 0x080C0000 - 0x080DFFFF 128 KB
Sector 11: 0x080E0000 - 0x080FFFFF 128 KB
```

### Write

`flash_write_bytes()`:

1. Checks the range:
   - `len` must be non-zero.
   - `address >= APP_START_ADDRESS`.
   - `address + len - 1 <= APP_END_ADDRESS`.
2. Unlocks flash.
3. Clears error flags.
4. Programs by byte (`PSIZE = x8`).
5. Reads each byte back immediately for checking.
6. Locks flash.

Byte programming is easy to test but slower than word or half-word programming.

### Verify

`flash_verify_bytes()` checks the range and compares each byte in flash against the input buffer.

## 10. Boot App Module

Files:

- `app/boot_app.h`
- `app/boot_app.c`

This module prepares the bootloader to jump to the application at:

```c
APP_START_ADDRESS = 0x08020000
```

`boot_app_is_valid()`:

1. Reads the initial MSP at `APP_START_ADDRESS`.
2. Reads the reset handler at `APP_START_ADDRESS + 4`.
3. Requires the MSP to be in SRAM:
   - `0x20000000` to `0x20020000`.
4. Requires the reset handler to be in application flash:
   - `APP_START_ADDRESS` to `0x080FFFFF`.

`boot_jump_to_app()`:

1. Reads the application MSP and reset handler.
2. Disables interrupts.
3. Disables SysTick.
4. Disables and clears pending IRQs in NVIC.
5. Sets `SCB->VTOR = APP_START_ADDRESS`.
6. Sets MSP with `__set_MSP(app_msp)`.
7. Casts the reset handler and calls the application entry point.

Currently, `main.c` does not call `boot_app_is_valid()` or `boot_jump_to_app()`. The module exists but is not yet connected to the main boot flow.

## 11. Test Scripts

The `test/` directory contains Python scripts that use `pyserial`.

### `test/test_erase.py`

- Default port: `COM5`.
- Baud rate: 115200.
- Sends command `CMD_ERASE_APP = 0x04`.
- Header format: `<HBBIIHI`, matching the current `ota_frame_t`.
- Reads an 18-byte response header and a 2-byte status payload.

### `test/test_write_chunk.py`

- Default port: `COM5`.
- Baud rate: 115200.
- Sends erase first.
- Then sends `CMD_WRITE_CHUNK = 0x05`.
- Test payload is 16 bytes.
- Expects response ACK (`0x80`) and status `0x0000`.

### `test/send_hello.py`

This script appears to follow an older protocol:

- `CMD_HELLO = 0x55`, while the current firmware defines `CMD_HELLO = 0x01`.
- Header format `<HBBIHI` is 14 bytes and does not include the `offset` field.
- The current firmware reads an 18-byte header.

Therefore, `send_hello.py` is currently incompatible with `ota_protocol.c` unless it is updated.

## 12. VS Code Configuration

`.vscode/tasks.json` contains tasks for:

- `CMake Configure`
- `CMake Build`
- `Flash OpenOCD`
- `Flash STM32CubeProgrammer`

`.vscode/launch.json` configures Cortex-Debug/OpenOCD debugging.

Current note:

- The current CMake target is `stm32f407_bld`.
- Some task/debug entries in `.vscode` still point to old artifacts:
  - `build/stm32f407_blink.elf`
  - `build/stm32f407_blink.hex`

If VS Code tasks are used for flashing/debugging, these should be changed to:

```text
build/stm32f407_bld.elf
build/stm32f407_bld.hex
```

## 13. FreeRTOS-Kernel

The repository contains a full `FreeRTOS-Kernel` directory with source, includes, portable ports, examples, license, and documentation. However, the main firmware target does not currently use it:

- There is no `add_subdirectory(FreeRTOS-Kernel)` in the root `CMakeLists.txt`.
- `FreeRTOS.h` is not included from `app/` or `core/`.
- No tasks or scheduler are created.

This can be treated as a prepared dependency for future integration.

## 14. Build Output Status

The `build/` directory currently contains:

- `stm32f407_bld.elf`
- `stm32f407_bld.hex`
- `stm32f407_bld.bin`
- `stm32f407_bld.map`

Observed file sizes:

- `.bin`: about 1.8 KB.
- `.elf`: about 11.8 KB.
- `.hex`: about 5.2 KB.
- `.map`: about 68.9 KB.

## 15. Current Technical Notes

Important points for future development:

- `ota_process_once()` is fully blocking on UART; if a full frame is not received, the firmware waits indefinitely.
- `crc32` in the frame is not verified yet.
- There is no timeout, retry, frame resynchronization, or framing marker beyond `magic`.
- `flash_erase_app_region()` does not check erase errors after each sector and currently always returns `FLASH_OK`.
- The write range allows addresses up to `0x080EFFFF`, but erase only covers sectors 5 through 10, meaning `0x080E0000` to `0x080EFFFF` is not erased.
- `FLASH_CR_PSIZE_X32` is defined but unused.
- `boot_app.c` has application jump logic but it is not called from `main`.
- `.vscode` and `README.md` contain traces of the old target name `stm32f407_blink`, while CMake currently builds `stm32f407_bld`.
- `test/send_hello.py` does not match the current protocol.
- `FreeRTOS-Kernel` is not part of the current build.

## 16. Expected OTA Flow From Host

A minimal OTA flow based on the current code:

1. Host opens serial `COMx` at 115200 baud.
2. Sends `CMD_HELLO` to check bootloader response.
3. Sends `CMD_ERASE_APP` to erase the application region.
4. Splits the application firmware into chunks of up to 1024 bytes.
5. For each chunk:
   - `offset` is the chunk position relative to `APP_START_ADDRESS`.
   - `length` is the chunk size in bytes.
   - payload is binary data.
   - waits for ACK before sending the next chunk.
6. After writing is complete, the flow still needs an image validation and jump-app step, but this command/flow is not implemented in the current OTA protocol yet.

## 17. Short Summary

This project is currently a minimal bare-metal STM32F407 bootloader:

- Built with CMake and `arm-none-eabi-gcc`.
- Startup/linker/CMSIS code handles reset, vector table, and memory sections.
- Blocking UART2 is used as the OTA transport.
- OTA uses a binary frame with an 18-byte header and supports hello/erase/write-chunk.
- Flash manager erases sectors 5-10 and writes the app from `0x08020000`.
- Application jump logic exists but is not connected to the main loop yet.
- Serial test scripts exist for erase/write.
- FreeRTOS is present in the repository but not used by the main firmware.
