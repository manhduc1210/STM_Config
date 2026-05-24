#include <stdint.h>
#include "stm32f407xx.h"
#include "boot_app.h"

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

int main(void)
{
    gpio_init();

    delay(2000000);

    if (boot_app_is_valid())
    {
        boot_jump_to_app();
    }

    while (1)
    {
        GPIOD->BSRR = (1U << LED_ORANGE_PIN);
        delay(400000);
        GPIOD->BSRR = (1U << (LED_ORANGE_PIN + 16U));
        delay(400000);
    }
}