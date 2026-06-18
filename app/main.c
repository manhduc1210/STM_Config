#include <stdint.h>
#include "stm32f407xx.h"
#include "uart_drv.h"
#include "ota_protocol.h"
#include "boot_app.h"

#define LED_ORANGE_PIN 13U
#define BOOTLOADER_OTA_WINDOW_LOOPS 8000000UL

static void gpio_init(void)
{
    RCC->AHB1ENR |= (1U << 3);
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(3U << (LED_ORANGE_PIN * 2U));
    GPIOD->MODER |=  (1U << (LED_ORANGE_PIN * 2U));
}

static void bootloader_run_ota_loop(void)
{
    const uint8_t ota_msg[] = "BOOTLOADER_OTA_MODE\r\n";
    uart2_write(ota_msg, sizeof(ota_msg) - 1);

    while (1)
    {
        ota_process_once();
    }
}

int main(void)
{
    gpio_init();
    uart2_init();
    const uint8_t msg[] = "BOOTLOADER_UART_OK\r\n";
    uart2_write(msg, sizeof(msg) - 1);

    for (uint32_t i = 0; i < BOOTLOADER_OTA_WINDOW_LOOPS; i++)
    {
        if (uart2_rx_available())
        {
            bootloader_run_ota_loop();
        }
    }

    if (boot_app_is_valid())
    {
        const uint8_t jump_msg[] = "BOOTLOADER_JUMP_APP\r\n";
        uart2_write(jump_msg, sizeof(jump_msg) - 1);
        boot_jump_to_app();
    }

    bootloader_run_ota_loop();
}
