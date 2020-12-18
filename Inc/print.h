#ifndef PRINT_H
#define PRINT_H

#define PRINT_BUFFER_SIZE 80
#define PRINT_UART USART3

void print_uart_init();

void print(const char *format, ...);

#endif