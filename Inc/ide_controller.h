#include "stm32f1xx.h"
#include "stdint.h"
#include "stdbool.h"

#define IDE_DATA_PORT GPIOB
#define IDE_DATA_WIDTH 16
#define IDE_ADDRESS_PORT GPIOA
#define IDE_ADDRESS_OFFSET 5
#define IDE_WRITE_STROBE_PORT GPIOA
#define IDE_WRITE_STROBE_PIN 12
#define IDE_READ_STROBE_PORT GPIOA
#define IDE_READ_STROBE_PIN 11

void IDE_init();

uint16_t IDE_read(uint8_t reg);

void IDE_write(uint8_t reg, uint16_t value);