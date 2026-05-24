#include "boot_app.h"
#include "stm32f407xx.h"

typedef void (*app_reset_handler_t)(void);

int boot_app_is_valid(void)
{
    uint32_t app_msp   = *(volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_reset = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);

    if ((app_msp < SRAM_START) || (app_msp > SRAM_END))
    {
        return 0;
    }

    if ((app_reset < APP_START_ADDRESS) || (app_reset > 0x080FFFFFUL))
    {
        return 0;
    }

    return 1;
}

void boot_jump_to_app(void)
{
    uint32_t app_msp   = *(volatile uint32_t *)APP_START_ADDRESS;
    uint32_t app_reset = *(volatile uint32_t *)(APP_START_ADDRESS + 4U);

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for (uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = APP_START_ADDRESS;

    __set_MSP(app_msp);

    app_reset_handler_t app_entry = (app_reset_handler_t)app_reset;
    app_entry();

    while (1)
    {
    }
}