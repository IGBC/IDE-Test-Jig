#include "ide_controller.h"

#include "stopwatch.h"

#include "stm32f1xx_hal.h"

#define IDE_SPEED GPIO_SPEED_LOW
#define IDE_PORT_READ 0x88888888
#define IDE_PORT_WRITE 0x11111111
#define IDE_ODR_READ 0x0000

#define NS_ADDR_SETUP 70
#define NS_DATA_SETUP 165
#define NS_WRITE_SETUP 60
#define NS_WRITE_HOLD 30
#define NS_READ_HOLD 20

void IDE_init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Configure Read Strobe
    HAL_GPIO_WritePin(IDE_READ_STROBE_PORT, (0x01 << IDE_READ_STROBE_PIN), GPIO_PIN_SET);
    GPIO_InitStruct.Pin = (0x01 << IDE_READ_STROBE_PIN);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = IDE_SPEED;
    HAL_GPIO_Init(IDE_READ_STROBE_PORT, &GPIO_InitStruct);
    
    // Configure Write Strobe
    HAL_GPIO_WritePin(IDE_WRITE_STROBE_PORT, (0x01 << IDE_WRITE_STROBE_PIN), GPIO_PIN_SET);
    GPIO_InitStruct.Pin = (0x01 << IDE_WRITE_STROBE_PIN);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = IDE_SPEED;
    HAL_GPIO_Init(IDE_WRITE_STROBE_PORT, &GPIO_InitStruct);

    // Configure Address Pins
    HAL_GPIO_WritePin(IDE_ADDRESS_PORT, (0x07 << IDE_ADDRESS_OFFSET), GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = (0x07 << IDE_ADDRESS_OFFSET);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = IDE_SPEED;
    HAL_GPIO_Init(IDE_ADDRESS_PORT, &GPIO_InitStruct);

    // Configure Data Bus
    // Reset Data Port into read mode (We will be using it bidirectionally as the Data bus.);
    IDE_DATA_PORT->ODR = IDE_ODR_READ;
    IDE_DATA_PORT->CRL = IDE_PORT_READ;
    IDE_DATA_PORT->CRH = IDE_PORT_READ;

    // this peripheral uses the stopwatch timer, so we need to start it
    STOPWATCH_RESET();
}

inline void IDE_set_addr(uint8_t addr) {
    // We're gonna drive the Port nanually, cos the HAL uses if statements, and I don't like that.
    addr &= 0x7;
    //generate a mask of reset pins by inverting the set pins
    uint8_t naddr = (~addr) & 0x7;
    // create a 32 bit register of pins to set and pins to reset (naddr)
    uint32_t bsrr = (addr << IDE_ADDRESS_OFFSET) | (naddr << (IDE_ADDRESS_OFFSET + 16));
    IDE_ADDRESS_PORT->BSRR = bsrr; //blit it to the port
}

/**
 * Reads the IDE Port
 * This is very blocking
 */
uint16_t IDE_read(uint8_t reg) {
    IDE_set_addr(reg);
    //Address setup time;
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_DATA_SETUP));
    //set port to read
    IDE_DATA_PORT->ODR = IDE_ODR_READ;
    IDE_DATA_PORT->CRL = IDE_PORT_READ;
    IDE_DATA_PORT->CRH = IDE_PORT_READ;
    // get data
    // Set strobe pin
    IDE_READ_STROBE_PORT->BSRR = (0x01 << (IDE_READ_STROBE_PIN + 16));
    //data setup time
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_DATA_SETUP));
    // read input data into destination
    uint16_t result = IDE_DATA_PORT->IDR;
    //clear strobe pin
    IDE_READ_STROBE_PORT->BSRR = (0x01 << (IDE_READ_STROBE_PIN));
    // recovery time
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_READ_HOLD));
    HAL_Delay(1); // wait for the bus to shut up;
    return result;
}


/** 
 * this will write/read from/to a register on the IDE bus
 * Timings are assuming mode0 and are stolen from http://blog.retroleum.co.uk/electronics-articles/an-8-bit-ide-interface/
 */
void IDE_write(uint8_t reg, uint16_t value) {
    IDE_set_addr(reg);
    //Address setup time;
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_DATA_SETUP));
// strobe the write pin.
    IDE_WRITE_STROBE_PORT->BSRR = (0x01 << (IDE_WRITE_STROBE_PIN + 16));
    // write setup time
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_DATA_SETUP-NS_WRITE_SETUP));

    // Push Value onto port
    IDE_DATA_PORT->ODR = value;
    // set port to write
    IDE_DATA_PORT->CRL = IDE_PORT_WRITE;
    IDE_DATA_PORT->CRH = IDE_PORT_WRITE;
    
    // write setup time
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_WRITE_SETUP));

    //reset strobe
    IDE_WRITE_STROBE_PORT->BSRR = (0x01 << (IDE_WRITE_STROBE_PIN));
    // write hold time
    STOPWATCH_DELAY(STOPWATCH_NS_TO_TICKS(NS_WRITE_HOLD));
    //set port back to read
    IDE_DATA_PORT->ODR = IDE_ODR_READ;
    IDE_DATA_PORT->CRL = IDE_PORT_READ;
    IDE_DATA_PORT->CRH = IDE_PORT_READ;
    HAL_Delay(1); // wait for the bus to shut up;
}
