#include "uart_drv.h"
#include "stm32f407xx.h"

void uart2_init(void)
{
    // GPIOA clock enable
    RCC->AHB1ENR |= (1U << 0);

    // USART2 clock enable
    RCC->APB1ENR |= (1U << 17);

    // PA2, PA3 alternate function mode
    GPIOA->MODER &= ~((3U << (2U * 2U)) |
                      (3U << (3U * 2U)));

    GPIOA->MODER |=  ((2U << (2U * 2U)) |
                      (2U << (3U * 2U)));

    // AF7 for USART2
    GPIOA->AFR[0] &= ~((0xFU << (2U * 4U)) |
                       (0xFU << (3U * 4U)));

    GPIOA->AFR[0] |=  ((7U << (2U * 4U)) |
                       (7U << (3U * 4U)));

    // 115200 baud if APB1 clock = 16 MHz
    USART2->BRR = 0x008B;

    USART2->CR1 = 0;
    USART2->CR1 |= (1U << 3);   // TE
    USART2->CR1 |= (1U << 2);   // RE
    USART2->CR1 |= (1U << 13);  // UE
}

void uart2_write_byte(uint8_t data)
{
    while (!(USART2->SR & (1U << 7))) {}
    USART2->DR = data;
}

uint8_t uart2_read_byte_blocking(void)
{
    while (!(USART2->SR & (1U << 5))) {}
    return (uint8_t)USART2->DR;
}

void uart2_write(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        uart2_write_byte(data[i]);
    }
}

void uart2_read_blocking(uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        data[i] = uart2_read_byte_blocking();
    }
}