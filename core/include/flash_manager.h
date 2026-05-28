#ifndef FLASH_MANAGER_H
#define FLASH_MANAGER_H

#include <stdint.h>

#define APP_START_ADDRESS   0x08020000UL
#define APP_END_ADDRESS     0x080EFFFFUL
#define APP_SLOT_SIZE       (APP_END_ADDRESS - APP_START_ADDRESS + 1UL)

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERR_INVALID_ADDRESS,
    FLASH_ERR_INVALID_LENGTH,
    FLASH_ERR_ERASE,
    FLASH_ERR_WRITE,
    FLASH_ERR_VERIFY
} flash_status_t;

flash_status_t flash_erase_app_region(void);
flash_status_t flash_write_bytes(uint32_t address, const uint8_t *data, uint32_t len);
flash_status_t flash_verify_bytes(uint32_t address, const uint8_t *data, uint32_t len);

#endif