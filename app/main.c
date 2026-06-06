#include <stdint.h>
#include "stm32f407xx.h"
#include "uart_drv.h"
#include "ota_protocol.h"

#define LED_ORANGE_PIN 13U

// static void delay(volatile uint32_t count)
// {
//     while (count--)
//     {
//         __asm volatile ("nop");
//     }
// }

static void gpio_init(void)
{
    RCC->AHB1ENR |= (1U << 3);
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(3U << (LED_ORANGE_PIN * 2U));
    GPIOD->MODER |=  (1U << (LED_ORANGE_PIN * 2U));
}

int main(void)
{
    gpio_init();
    uart2_init();
    const uint8_t msg[] = "BOOTLOADER_UART_OK\r\n";
    uart2_write(msg, sizeof(msg) - 1);

    while (1)
    {
        ota_process_once();

        GPIOD->ODR ^= (1U << LED_ORANGE_PIN);
    }
}