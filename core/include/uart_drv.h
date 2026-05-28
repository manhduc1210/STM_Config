#ifndef UART_DRV_H
#define UART_DRV_H

#include <stdint.h>

void uart2_init(void);
void uart2_write_byte(uint8_t data);
uint8_t uart2_read_byte_blocking(void);
void uart2_write(const uint8_t *data, uint32_t len);
void uart2_read_blocking(uint8_t *data, uint32_t len);

#endif