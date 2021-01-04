#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>

#define PRINT_BUFFER_SIZE 80
#define PRINT_UART USART3

void print_uart_init();

void print(const char *format, ...);
void print_fixed_str(char * str, unsigned int len);

void hexdump(uint8_t *data, unsigned int len);

#endif