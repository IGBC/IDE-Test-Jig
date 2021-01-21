#include "print.h"

#include <stdio.h>
#include <stdarg.h>
#include "stm32f1xx_hal.h"

static UART_HandleTypeDef print_uart;

static uint8_t print_buffer[(PRINT_BUFFER_SIZE) + 1];

void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf((char*)print_buffer, sizeof(print_buffer), format, args );
    va_end(args);
    if (n > sizeof(print_buffer) - 1) { n = sizeof(print_buffer - 1); };
    HAL_UART_Transmit(&print_uart, print_buffer, n, 10000);
}

void hexdump(uint8_t *data, unsigned int len) {
    print("%p\r\n", data);
    for (unsigned int i = 0; i < (len - 7); i+=8) {
        print("| %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x | %c%c%c%c%c%c%c%c\r\n",
        data[i+0], data[i+1], data[i+2], data[i+3], data[i+4], data[i+5], data[i+6], data[i+7],
        ((data[i+0] > 31) && (data[i+0] < 256)) ? data[i+0] : ' ',
        ((data[i+1] > 31) && (data[i+1] < 256)) ? data[i+1] : ' ',
        ((data[i+2] > 31) && (data[i+2] < 256)) ? data[i+2] : ' ',
        ((data[i+3] > 31) && (data[i+3] < 256)) ? data[i+3] : ' ',
        ((data[i+4] > 31) && (data[i+4] < 256)) ? data[i+4] : ' ',
        ((data[i+5] > 31) && (data[i+5] < 256)) ? data[i+5] : ' ',
        ((data[i+6] > 31) && (data[i+6] < 256)) ? data[i+6] : ' ',
        ((data[i+7] > 31) && (data[i+7] < 256)) ? data[i+7] : ' ');
    }
    
}

void print_fixed_str(char * str, unsigned int len) {
    HAL_UART_Transmit(&print_uart, (uint8_t*)str, len, 10000);
    HAL_UART_Transmit(&print_uart, (uint8_t*)"\r\n", 2, 10000);
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