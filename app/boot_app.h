#ifndef BOOT_APP_H
#define BOOT_APP_H

#include <stdint.h>

#define APP_START_ADDRESS  0x08020000UL
#define SRAM_START         0x20000000UL
#define SRAM_SIZE          (128UL * 1024UL)
#define SRAM_END           (SRAM_START + SRAM_SIZE)

int boot_app_is_valid(void);
void boot_jump_to_app(void);

#endif