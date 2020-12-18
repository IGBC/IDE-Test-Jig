#include "print.h"

#include <stdio.h>
#include <stdarg.h>
#include "stm32f1xx_hal.h"

static UART_HandleTypeDef print_uart;

static uint8_t print_buffer[(PRINT_BUFFER_SIZE) + 1];

print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf((char*)print_buffer, sizeof(print_buffer), format, args );
    va_end(args);
    if (n > sizeof(print_buffer) - 1) { n = sizeof(print_buffer - 1); };
    HAL_UART_Transmit(&print_uart, print_buffer, n, 10000);
}

void print_uart_init() {
  print_uart.Instance = PRINT_UART;
  print_uart.Init.BaudRate = 115200;
  print_uart.Init.WordLength = UART_WORDLENGTH_8B;
  print_uart.Init.StopBits = UART_STOPBITS_1;
  print_uart.Init.Parity = UART_PARITY_NONE;
  print_uart.Init.Mode = UART_MODE_TX;
  print_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  print_uart.Init.OverSampling = UART_OVERSAMPLING_16;
  assert_param(HAL_UART_Init(&print_uart) == HAL_OK);
}