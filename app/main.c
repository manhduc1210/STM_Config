#include <stdint.h>
#include "stm32f407xx.h"

#define LED_GREEN_PIN   12U   // PD12 - LD4
#define LED_ORANGE_PIN  13U   // PD13 - LD3

static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile ("nop");
    }
}

static void gpio_init(void)
{
    // Enable GPIOD clock: RCC_AHB1ENR bit 3 = GPIODEN
    RCC->AHB1ENR |= (1U << 3);

    // Dummy read to ensure clock is enabled before GPIO access
    (void)RCC->AHB1ENR;

    // PD12, PD13 output mode: MODER = 01
    GPIOD->MODER &= ~((3U << (LED_GREEN_PIN * 2U)) |
                      (3U << (LED_ORANGE_PIN * 2U)));

    GPIOD->MODER |=  ((1U << (LED_GREEN_PIN * 2U)) |
                      (1U << (LED_ORANGE_PIN * 2U)));

    // Push-pull
    GPIOD->OTYPER &= ~((1U << LED_GREEN_PIN) |
                       (1U << LED_ORANGE_PIN));

    // Medium/high speed
    GPIOD->OSPEEDR |= ((2U << (LED_GREEN_PIN * 2U)) |
                       (2U << (LED_ORANGE_PIN * 2U)));

    // No pull-up, no pull-down
    GPIOD->PUPDR &= ~((3U << (LED_GREEN_PIN * 2U)) |
                      (3U << (LED_ORANGE_PIN * 2U)));
}

int main(void)
{
    gpio_init();

    while (1)
    {
        // Set PD12, reset PD13
        // GPIOD->BSRR = (1U << LED_GREEN_PIN) |
        //               (1U << (LED_ORANGE_PIN + 16U));

        // delay(800000);

        // Reset PD12, set PD13
        GPIOD->BSRR = (1U << (LED_GREEN_PIN + 16U)) |
                      (1U << LED_ORANGE_PIN);

        delay(800000);
    }
}