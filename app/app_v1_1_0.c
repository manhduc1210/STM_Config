#include <stdint.h>

#include "stm32f407xx.h"

#define APP_START_ADDRESS 0x08020000UL
#define LED_GREEN_PIN     12U

__attribute__((used))
const char app_version_string[] = "STM32_APP_VERSION=1.1.0";

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

    GPIOD->MODER &= ~(3U << (LED_GREEN_PIN * 2U));
    GPIOD->MODER |=  (1U << (LED_GREEN_PIN * 2U));
    GPIOD->OTYPER &= ~(1U << LED_GREEN_PIN);
    GPIOD->OSPEEDR |= (2U << (LED_GREEN_PIN * 2U));
    GPIOD->PUPDR &= ~(3U << (LED_GREEN_PIN * 2U));
}

int main(void)
{
    SCB->VTOR = APP_START_ADDRESS;
    gpio_init();

    while (1)
    {
        GPIOD->ODR ^= (1U << LED_GREEN_PIN);
        delay(1200000);
    }
}
