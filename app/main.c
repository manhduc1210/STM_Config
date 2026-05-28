#include <stdint.h>
#include "stm32f407xx.h"
#include "uart_drv.h"
#include "ota_protocol.h"
#include "flash_manager.h"

#define LED_ORANGE_PIN 13U

static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile ("nop");
    }
}

static void gpio_init(void)
{
    RCC->AHB1ENR |= (1U << 3);
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(3U << (LED_ORANGE_PIN * 2U));
    GPIOD->MODER |=  (1U << (LED_ORANGE_PIN * 2U));
}

static int flash_test_run(void)
{
    uint8_t test_data[16] =
    {
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC,
        0xDD, 0xEE, 0x12, 0x34
    };

    flash_status_t ret;

    ret = flash_erase_app_region();
    if (ret != FLASH_OK)
    {
        return -1;
    }

    ret = flash_write_bytes(APP_START_ADDRESS, test_data, sizeof(test_data));
    if (ret != FLASH_OK)
    {
        return -2;
    }

    ret = flash_verify_bytes(APP_START_ADDRESS, test_data, sizeof(test_data));
    if (ret != FLASH_OK)
    {
        return -3;
    }

    ret = flash_write_bytes(0x08000000UL, test_data, sizeof(test_data));
    if (ret != FLASH_ERR_INVALID_ADDRESS)
    {
        return -4;
    }

    return 0;
}

int main(void)
{
    gpio_init();
    uart2_init();

    int ret = flash_test_run();

    if (ret == 0)
    {
        // PASS: LED cam blink chậm
        while (1)
        {
            GPIOD->ODR ^= (1U << LED_ORANGE_PIN);
            uart2_write((const uint8_t *)"Hello\r\n", 7);
            delay(1000000);
        }
    }
    else
    {
        // FAIL: LED cam blink nhanh
        while (1)
        {
            GPIOD->ODR ^= (1U << LED_ORANGE_PIN);
            delay(150000);
        }
    }
}