# STM32 Bootloader Project Overview

## 1. Current Scope

This repository contains a bare-metal STM32F407 bootloader and a nested ESP32-S3
project that can act as the OTA sender over UART.

The STM32 side builds the `stm32f407_bld` target with CMake and
`arm-none-eabi-gcc`. Its current job is:

- start from internal flash at `0x08000000`;
- expose an OTA command protocol on USART2;
- erase, write, and verify the application slot starting at `0x08020000`;
- jump to a valid application image when no OTA traffic arrives during the boot
  window.

The ESP32 side lives in `ESP32_OTA_Config/esp32_ota`. It mounts a SPIFFS image,
loads a local STM32 firmware binary, and sends it to the STM32 bootloader using
the same 18-byte OTA frame format.

Main STM32 hardware assumptions:

- MCU: STM32F407VG / Cortex-M4F.
- Flash: 1 MB at `0x08000000`.
- SRAM: 128 KB at `0x20000000`.
- CCMRAM: 64 KB at `0x10000000`.
- OTA UART: USART2 on PA2/PA3 at 115200 8N1.
- Status GPIO: PD13 is configured as output, but the current main loop no longer
  toggles it after each frame.

`FreeRTOS-Kernel/` exists in the repository, but the current STM32 firmware
target does not link it and does not call FreeRTOS APIs.

## 2. Repository Layout

```text
.
|-- app/
|   |-- main.c                 STM32 boot flow and OTA/app-jump selection
|   |-- boot_app.c             application validation and jump helper
|   `-- boot_app.h
|-- core/
|   |-- include/
|   |   |-- flash_manager.h     app-slot address constants and flash API
|   |   |-- ota_protocol.h      OTA command/status/frame definitions
|   |   `-- uart_drv.h         USART2 API
|   `-- src/
|       |-- flash_manager.c     erase/write/verify implementation
|       |-- ota_protocol.c      OTA frame parser and command handlers
|       `-- uart_drv.c         USART2 register-level driver
|-- cmsis/                     CMSIS core/device headers and SystemInit
|-- startup/
|   `-- startup_stm32f407xx.s  vector table and reset handler
|-- linker/
|   `-- STM32F407VGTx_FLASH.ld STM32 memory map and section placement
|-- test/                      PC-side pyserial smoke tests
|-- ESP32_OTA_Config/          nested ESP-IDF OTA sender project
|-- FreeRTOS-Kernel/           present but unused by the STM32 target
|-- CMakeLists.txt
|-- arm-none-eabi-toolchain.cmake
|-- README.md
`-- Overview.md
```

## 3. STM32 Build

The root `CMakeLists.txt` defines:

```text
project(stm32f407_baremetal C ASM)
TARGET_NAME = stm32f407_bld
```

The executable target is:

```text
stm32f407_bld.elf
```

Source inputs:

- `core/src/*.c`
- `app/*.c`
- `cmsis/device/st/stm32f4xx/system_stm32f4xx.c`
- `startup/startup_stm32f407xx.s`

Important build settings:

- compile definition: `STM32F407xx`
- C standard: C11 from `arm-none-eabi-toolchain.cmake`
- CPU/FPU flags: `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- warnings: `-Wall -Wextra -Werror`
- linker script: `linker/STM32F407VGTx_FLASH.ld`
- libraries/specs: `--specs=nano.specs --specs=nosys.specs`
- map output: `stm32f407_bld.map`
- post-build outputs: `stm32f407_bld.hex` and `stm32f407_bld.bin`

Typical build commands:

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Flash the STM32 bootloader with OpenOCD:

```powershell
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/stm32f407_bld.elf verify reset exit"
```

## 4. STM32 Memory Layout

The linker script declares:

```text
FLASH  : ORIGIN = 0x08000000, LENGTH = 1024K
RAM    : ORIGIN = 0x20000000, LENGTH = 128K
CCMRAM : ORIGIN = 0x10000000, LENGTH = 64K
```

The bootloader is linked at the start of flash. The application slot is defined
in `core/include/flash_manager.h`:

```c
#define APP_START_ADDRESS   0x08020000UL
#define APP_END_ADDRESS     0x080EFFFFUL
#define APP_SLOT_SIZE       (APP_END_ADDRESS - APP_START_ADDRESS + 1UL)
```

The current flash erase implementation erases STM32F407 sectors 5 through 10:

```text
Sector 5 : 0x08020000 - 0x0803FFFF
Sector 6 : 0x08040000 - 0x0805FFFF
Sector 7 : 0x08060000 - 0x0807FFFF
Sector 8 : 0x08080000 - 0x0809FFFF
Sector 9 : 0x080A0000 - 0x080BFFFF
Sector 10: 0x080C0000 - 0x080DFFFF
```

Sector 11 starts at `0x080E0000`. The write/verify range currently allows bytes
up to `0x080EFFFF`, but erase does not cover sector 11 because the source notes
reserve metadata near `0x080F0000`. Keep this in mind before placing an app image
or metadata in the `0x080E0000` range.

## 5. Startup and Clock State

`startup/startup_stm32f407xx.s` provides the vector table, reset handler,
`.data` copy, `.bss` clear, C library init, and call into `main`.

`cmsis/device/st/stm32f4xx/system_stm32f4xx.c` leaves
`SystemCoreClock = 16000000` by default. `SystemInit()` enables FPU access when
configured, but the current project does not configure a PLL. The UART driver
therefore assumes the default 16 MHz clock when setting USART2 baud to 115200.

## 6. STM32 Boot Flow

`app/main.c` now implements a real bootloader decision flow:

1. Initialize PD13 GPIO.
2. Initialize USART2.
3. Write the boot banner `BOOTLOADER_UART_OK\r\n`.
4. Poll for incoming UART data for `BOOTLOADER_OTA_WINDOW_LOOPS`
   (`8000000UL`) iterations.
5. If a byte is waiting during that window, enter OTA mode and write
   `BOOTLOADER_OTA_MODE\r\n`.
6. If no OTA byte arrives and `boot_app_is_valid()` returns true, write
   `BOOTLOADER_JUMP_APP\r\n` and jump to the app at `0x08020000`.
7. If no valid app exists, stay in OTA mode forever.

Once OTA mode is entered, the firmware repeatedly calls `ota_process_once()` and
does not return to the app-jump decision.

## 7. UART Driver

Files:

- `core/include/uart_drv.h`
- `core/src/uart_drv.c`

API:

```c
void uart2_init(void);
int uart2_rx_available(void);
void uart2_write_byte(uint8_t data);
uint8_t uart2_read_byte_blocking(void);
void uart2_write(const uint8_t *data, uint32_t len);
void uart2_read_blocking(uint8_t *data, uint32_t len);
```

USART2 configuration:

- GPIOA clock enabled.
- USART2 clock enabled.
- PA2 and PA3 configured as alternate function AF7.
- `USART2->BRR = 0x008B`, matching 115200 baud with a 16 MHz APB1 clock.
- Transmit, receive, and USART enable bits are set.

The driver is intentionally simple:

- `uart2_rx_available()` checks RXNE without blocking.
- byte write waits for TXE.
- byte read waits for RXNE.
- buffer read/write functions loop over the byte functions.

## 8. OTA Protocol

Files:

- `core/include/ota_protocol.h`
- `core/src/ota_protocol.c`

STM32 and ESP32 share this packed 18-byte header:

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

Constants:

```c
#define OTA_MAGIC        0x4F54U
#define OTA_VERSION      0x01U
#define OTA_MAX_PAYLOAD  1024U

#define CMD_HELLO        0x01U
#define CMD_ERASE_APP    0x04U
#define CMD_WRITE_CHUNK  0x05U
#define CMD_ACK          0x80U
#define CMD_NACK         0x81U
```

Status values:

```c
OTA_OK                   = 0x0000
OTA_ERR_INVALID_CMD       = 0x1003
OTA_ERR_INVALID_LENGTH    = 0x1004
OTA_ERR_FLASH_ERASE       = 0x1008
OTA_ERR_FLASH_WRITE       = 0x1009
OTA_ERR_VERIFY            = 0x100A
```

Responses reuse the same header:

- `command` is `CMD_ACK` or `CMD_NACK`;
- `sequence` echoes the request sequence;
- `offset` is 0;
- `length` is 2;
- payload is a little-endian `uint16_t status`;
- `crc32` is currently 0.

The STM32 parser now resynchronizes on the little-endian magic bytes
`54 4F`. `ota_read_frame_resync()` reads bytes until it finds that pair, then
reads the remaining 16 bytes of the header. This lets the bootloader ignore text
banners or stale bytes on the same UART before a binary frame.

Implemented command behavior:

- `CMD_HELLO`: ACK with `OTA_OK`.
- `CMD_ERASE_APP`: erase sectors 5 through 10, then ACK or NACK.
- `CMD_WRITE_CHUNK`: read payload, write to `APP_START_ADDRESS + offset`,
  verify the bytes, then ACK or NACK.

Current protocol limitations:

- `crc32` is present in the frame but not validated.
- UART reads inside a frame are blocking and have no timeout.
- There is no explicit finalize, image metadata, rollback, or jump command.
- Image authenticity and version checks are not implemented.

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

`flash_erase_app_region()`:

- unlocks flash with STM32F4 key values;
- clears common flash status/error bits;
- erases sectors 5 through 10;
- locks flash;
- currently returns `FLASH_OK` without checking per-sector error status after
  each erase.

`flash_write_bytes()`:

- rejects zero-length writes;
- rejects addresses before `APP_START_ADDRESS`;
- rejects writes beyond `APP_END_ADDRESS`;
- unlocks flash and clears errors;
- programs one byte at a time with `PSIZE = x8`;
- reads each byte back immediately;
- locks flash before returning.

`flash_verify_bytes()` performs the same range checks and compares flash against
the supplied buffer byte by byte.

## 10. Application Jump

Files:

- `app/boot_app.h`
- `app/boot_app.c`

`boot_app_is_valid()` reads the initial MSP and reset handler from
`APP_START_ADDRESS`.

Validation rules:

- MSP must be in SRAM: `0x20000000` through `0x20020000`.
- reset handler must be in application flash: `0x08020000` through
  `0x080FFFFF`.

`boot_jump_to_app()`:

1. disables interrupts;
2. disables SysTick;
3. disables and clears pending NVIC IRQs;
4. sets `SCB->VTOR = APP_START_ADDRESS`;
5. sets MSP to the application's initial stack pointer;
6. calls the application reset handler.

This jump path is now connected from `main.c` after the initial OTA window.

## 11. PC-Side Test Scripts

The `test/` directory contains small `pyserial` scripts for direct PC-to-STM32
testing.

Current scripts:

- `test/test.py`: reads 100 bytes from `COM9` at 115200, useful for seeing the
  STM32 boot banners.
- `test/test_erase.py`: sends `CMD_ERASE_APP` on `COM9`, uses a 20 second serial
  timeout, scans for response magic `54 4F`, and prints the ACK/NACK status.
- `test/test_write_chunk.py`: sends erase, then writes a 16-byte payload at
  offset 0 and expects ACK/status OK.
- `test/send_hello.py`: still appears stale. It uses `CMD_HELLO = 0x55` and a
  14-byte header format, while the current firmware expects `CMD_HELLO = 0x01`
  and the 18-byte header.

The known 16-byte test payload is:

```text
11 22 33 44 55 66 77 88 99 AA BB CC DD EE 12 34
```

When read with OpenOCD as little-endian words from `0x08020000`, it should show:

```text
44332211 88776655 ccbbaa99 3412eedd
```

## 12. ESP32 OTA Sender

The nested ESP-IDF project is under:

```text
ESP32_OTA_Config/esp32_ota
```

Important files:

- `main/app_main.c`
- `main/ota_master.c`
- `main/ota_master.h`
- `main/ota_packet.h`
- `main/stm32_uart_transport.c`
- `main/stm32_uart_transport.h`
- `partitions.csv`
- `spiffs_image/stm32_app_v1_1_0.bin`
- `spiffs_image/stm32_app_v1_2_0.bin`

`main/app_main.c` currently:

1. initializes the STM32 UART transport;
2. mounts SPIFFS partition `storage` at `/spiffs`;
3. checks for `/spiffs/stm32_app_v1_2_0.bin`;
4. waits 1 second;
5. calls `ota_master_send_local_firmware()`;
6. logs `Step 7 OTA transfer complete` on success.

`main/ota_master.c` implements the file transfer:

- sends `HELLO` with retry;
- sends `ERASE_APP` with retry;
- reads the firmware file in chunks of up to `OTA_MAX_PAYLOAD` bytes;
- sends each chunk as `CMD_WRITE_CHUNK`;
- retries failed commands up to 3 times;
- logs progress percentages.

ESP32 UART settings in `main/stm32_uart_transport.c`:

```text
UART port       : UART_NUM_1
Baudrate        : 115200
TX pin          : GPIO17
RX pin          : GPIO18
RX/TX buffer    : 2048 bytes
ACK timeout     : 3000 ms
ERASE timeout   : 30000 ms
WRITE timeout   : 5000 ms
HELLO retries   : 3
Retry delay     : 100 ms
Flow control    : disabled
```

Wiring:

```text
ESP32 GPIO17 TX  ---> STM32 PA3 USART2_RX
ESP32 GPIO18 RX  <--- STM32 PA2 USART2_TX
ESP32 GND        ---- STM32 GND
```

The ESP32 response parser also scans for `54 4F`, so it can ignore STM32 text
such as `BOOTLOADER_UART_OK` before binary ACK/NACK frames.

The ESP32 partition table includes a SPIFFS `storage` partition of `0xF0000`
bytes, and `main/CMakeLists.txt` creates the SPIFFS image from
`../spiffs_image` with `FLASH_IN_PROJECT`.

Typical ESP32 commands:

```powershell
cd ESP32_OTA_Config\esp32_ota
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

## 13. End-to-End Test Flow

1. Build the STM32 bootloader:

   ```powershell
   cmake --build build
   ```

2. Flash the STM32 bootloader:

   ```powershell
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/stm32f407_bld.elf verify reset exit"
   ```

3. Wire ESP32 UART1 to STM32 USART2:

   ```text
   ESP32 GPIO17 TX -> STM32 PA3
   ESP32 GPIO18 RX <- STM32 PA2
   GND             -> GND
   ```

4. Build and flash the ESP32 project:

   ```powershell
   cd ESP32_OTA_Config\esp32_ota
   idf.py build
   idf.py -p COMx flash monitor
   ```

5. Confirm ESP32 logs show successful HELLO, ERASE_APP, WRITE_CHUNK transfers,
   progress, and `Step 7 OTA transfer complete`.

6. Verify STM32 flash contents when using the known test payload or a known app
   image:

   ```powershell
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init; reset halt; mdw 0x08020000 4; shutdown"
   ```

7. Reset STM32 with no OTA byte arriving during the boot window. If the image at
   `0x08020000` has a valid MSP and reset handler, the bootloader should print
   `BOOTLOADER_JUMP_APP` and jump to it.

## 14. Current Mismatches and Notes

- `.vscode/tasks.json`, `.vscode/launch.json`, and part of `README.md` still
  reference `stm32f407_blink` artifacts. The current CMake target is
  `stm32f407_bld`.
- `test/send_hello.py` does not match the current OTA command value or header
  size.
- `flash_erase_app_region()` does not verify erase errors per sector.
- `APP_END_ADDRESS` permits writes into part of sector 11, but the erase routine
  only erases sectors 5 through 10.
- OTA CRC, image metadata, signature/authenticity, and finalize/jump commands
  are not implemented yet.
- STM32 frame body reads are blocking; only the initial boot window uses
  `uart2_rx_available()` for a non-blocking decision.
- Several comments in `core/src/flash_manager.c` contain mojibake, but the code
  intent is still clear from the surrounding logic.

## 15. Short Summary

The current project is no longer only a simple blocking UART bootloader. It now
has a boot window, UART boot banners, magic-byte OTA resynchronization, app
validation, and a connected jump-to-application path. The nested ESP32-S3 project
has progressed from a UART smoke test to a SPIFFS-backed OTA sender that streams
a local STM32 firmware binary to the bootloader in 1024-byte chunks.
