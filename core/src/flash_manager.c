#include "flash_manager.h"
#include "stm32f407xx.h"

#define FLASH_KEY1  0x45670123UL
#define FLASH_KEY2  0xCDEF89ABUL

// #define FLASH_CR_LOCK     (1U << 31)
// #define FLASH_CR_STRT     (1U << 16)
// #define FLASH_CR_SER      (1U << 1)
// #define FLASH_SR_BSY      (1U << 16)

#define FLASH_CR_PSIZE_X32  (2U << 8)

static void flash_wait_busy(void)
{
    while (FLASH->SR & FLASH_SR_BSY)
    {
    }
}

static void flash_clear_errors(void)
{
    // Clear common error flags on STM32F4
    FLASH->SR |= (1U << 0);  // EOP
    FLASH->SR |= (1U << 1);  // OPERR
    FLASH->SR |= (1U << 4);  // WRPERR
    FLASH->SR |= (1U << 5);  // PGAERR
    FLASH->SR |= (1U << 6);  // PGPERR
    FLASH->SR |= (1U << 7);  // PGSERR
}

static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static flash_status_t flash_check_range(uint32_t address, uint32_t len)
{
    if (len == 0)
    {
        return FLASH_ERR_INVALID_LENGTH;
    }

    if (address < APP_START_ADDRESS)
    {
        return FLASH_ERR_INVALID_ADDRESS;
    }

    if ((address + len - 1UL) > APP_END_ADDRESS)
    {
        return FLASH_ERR_INVALID_ADDRESS;
    }

    return FLASH_OK;
}

static void flash_erase_sector(uint8_t sector)
{
    flash_wait_busy();

    FLASH->CR &= ~(0xFU << 3);
    FLASH->CR |= FLASH_CR_SER;
    FLASH->CR |= ((uint32_t)sector << 3);
    FLASH->CR |= FLASH_CR_STRT;

    flash_wait_busy();

    FLASH->CR &= ~FLASH_CR_SER;
}

/*
 STM32F407 1MB sector map:
 Sector 0: 0x08000000 - 0x08003FFF   16KB
 Sector 1: 0x08004000 - 0x08007FFF   16KB
 Sector 2: 0x08008000 - 0x0800BFFF   16KB
 Sector 3: 0x0800C000 - 0x0800FFFF   16KB
 Sector 4: 0x08010000 - 0x0801FFFF   64KB
 Sector 5: 0x08020000 - 0x0803FFFF   128KB
 Sector 6: 0x08040000 - 0x0805FFFF   128KB
 Sector 7: 0x08060000 - 0x0807FFFF   128KB
 Sector 8: 0x08080000 - 0x0809FFFF   128KB
 Sector 9: 0x080A0000 - 0x080BFFFF   128KB
 Sector 10:0x080C0000 - 0x080DFFFF   128KB
 Sector 11:0x080E0000 - 0x080FFFFF   128KB
*/
flash_status_t flash_erase_app_region(void)
{
    flash_unlock();
    flash_clear_errors();

    // App bắt đầu từ 0x08020000 nên erase sector 5 -> 10.
    // Không erase sector 11 vì mình đang để metadata tại 0x080F0000.
    for (uint8_t sector = 5; sector <= 10; sector++)
    {
        flash_erase_sector(sector);
    }

    flash_lock();

    return FLASH_OK;
}

flash_status_t flash_write_bytes(uint32_t address, const uint8_t *data, uint32_t len)
{
    flash_status_t range_status = flash_check_range(address, len);
    if (range_status != FLASH_OK)
    {
        return range_status;
    }

    flash_unlock();
    flash_clear_errors();

    // Program theo byte cho dễ test trước.
    // Sau này tối ưu lên word/32-bit.
    FLASH->CR &= ~(3U << 8);  // PSIZE = x8

    for (uint32_t i = 0; i < len; i++)
    {
        flash_wait_busy();

        FLASH->CR |= (1U << 0);  // PG

        *(volatile uint8_t *)(address + i) = data[i];

        flash_wait_busy();

        FLASH->CR &= ~(1U << 0); // clear PG

        if (*(volatile uint8_t *)(address + i) != data[i])
        {
            flash_lock();
            return FLASH_ERR_WRITE;
        }
    }

    flash_lock();

    return FLASH_OK;
}

flash_status_t flash_verify_bytes(uint32_t address, const uint8_t *data, uint32_t len)
{
    flash_status_t range_status = flash_check_range(address, len);
    if (range_status != FLASH_OK)
    {
        return range_status;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        if (*(volatile uint8_t *)(address + i) != data[i])
        {
            return FLASH_ERR_VERIFY;
        }
    }

    return FLASH_OK;
}