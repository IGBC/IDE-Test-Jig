#include "ata_driver.h"

#include "ide_controller.h"
#include "print.h"

#include "stm32f1xx_hal.h"

// ATA status codes, get by reading status register

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// ATA error codes, get by reading error register when ATA_SR_ERR is set

#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// ATA Commands, To be filled in from CF spec.
#define ATA_COMMAND_IDENTIFY         0xEC
#define ATA_COMMAND_READ_SECTOR      0x21

#define ATA_COMMAND_IDENTIFY_PACKET  0xA1


// ATA Register Definitions

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT   0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_STATUS     0x07
#define ATA_REG_COMMAND    0x07
// Registers 8 - 15 are not supported by the hardware

// #define ATA_REG_SECCOUNT1  0x08
// #define ATA_REG_LBA3       0x09
// #define ATA_REG_LBA4       0x0A
// #define ATA_REG_LBA5       0x0B
// #define ATA_REG_CONTROL    0x0C
// #define ATA_REG_ALTSTATUS  0x0C
// #define ATA_REG_DEVADDRESS 0x0D

// The hardware is locked into master mode, so can't address the slave device. 
// The hardware also only supports one channel. 

// ATA Magic Numbers

#define ATA_MAGIC_SELMASTER 0xA0
#define ATA_MAGIC_EN_LBA    0x40

typedef struct __attribute__((__packed__)) {
    uint16_t signature;
    uint16_t def_cylinders;
    uint16_t __RESERVED_1;
    uint16_t def_heads;
    uint16_t __RESERVED_2[2];
    uint16_t def_sectors;
    uint16_t num_sectors_w[2];
    uint16_t __RESERVED_3;
    char serial_no[20];
    uint16_t __RESERVED_4[2];
    uint16_t num_ecc_bytes;
    char firmware_rev[8];
    char model_no[40];
    uint16_t max_sectors_op;
    uint16_t __RESERVED_5;
    uint16_t capabilities;
    uint16_t __RESERVED_6;
    uint16_t timing_mode;
    uint16_t __RESERVED_7;
    uint16_t validity;
    uint16_t curr_cyclinders;
    uint16_t curr_heads;
    uint16_t curr_sectors;
    uint16_t curr_lba_sectors_w[2];
    uint16_t multiple_sectors_setting;
    uint16_t total_lba_sectors_w[2];
    uint16_t __RESERVED_8[2];
    uint16_t advanced_pio_modes;
    uint16_t __RESERVED_9[2];
    uint16_t min_pio_cycle_n_iordy;
    uint16_t min_pio_cycle_w_iordy;
    char __RESERVED_10[24];
    uint16_t features_supported_w[3];
    uint16_t features_enabled_w[3];
    // There is more after this but secure erase is all that's up there, doesn't interest us.
    char __RESERVED_11[334];
} DriveIdentity;


void ata_read_buffer(uint16_t *buffer, int size) {
    for (int i = 0; i < size; i += 1) {
        uint16_t data = IDE_read(ATA_REG_DATA);
        HAL_Delay(1);
        buffer[i] = data; // store low byte
        //buffer[i+1] = data >> (0xFF); // store high byte
    }
}

char ide_buffer[512] = {0};

void ata_init() {
    bool detected;
    start:
    // Todo: bail if card not detected (RN not supported in HW) 
    detected = false;
    do {
        HAL_Delay(1000); // wait 1s for the card to boot up and or be plugged in;
        
        print("ATA Selecting Master\r\n");
        IDE_write(ATA_REG_COMMAND, ATA_MAGIC_SELMASTER);
        HAL_Delay(100); // wait 10ms for master select to work. 

        // issue identify command
        print("ATA Issuing Identify\r\n");
        IDE_write(ATA_REG_COMMAND, ATA_COMMAND_IDENTIFY);
        HAL_Delay(10);
        if (IDE_read(ATA_REG_STATUS) == 0) {
            print("ATA Driver believes no device present, waiting.\r\n");
        } else {
            detected = true;
        }
    } while (!detected);
    
    // spin while card is busy
    while (true) {
        int status = IDE_read(ATA_REG_STATUS);
        // Spitting out the UART implicitly has a delay.
        if (status & ATA_SR_DRQ) {
             print("ATA Responds Ready (%02x)\r\n", status);
             break;
        } else { 
            if (status & ATA_SR_BSY) {
                print("ATA Responds Busy (%02x)\r\n", status);
            } else {
                print("ATA NOT Ready (%02x)\r\n", status);
                print("ATA Registers: 1: %02x, 2: %02x, 3: %02x, 4: %02x, 5: %02x, 6: %02x,\r\n", IDE_read(1), IDE_read(2), IDE_read(3), IDE_read(4), IDE_read(5), IDE_read(6));
                goto start;
            }
        }
    }

    uint16_t status = IDE_read(ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
        uint16_t error = IDE_read(ATA_SR_ERR);
        print("ATA Reads Error: 0x%02x\r\n", (uint8_t)error);
        // BAIL
        return;
    }
    print("ATA STATUS now: 0x%02x\r\n", (uint8_t)status);

    //IDE_read(0); // DEGUB!
    HAL_Delay(1);

    ata_read_buffer((uint16_t *)ide_buffer, 256);
    

    // // spin while card is busy
    // while ((IDE_read(ATA_REG_STATUS) & ATA_SR_BSY) != 0) {
    //     print("ATA Responds Busy\r\n");
    //     // Spitting out the UART implicitly has a delay.
    // }

    // status = IDE_read(ATA_REG_STATUS);
    // if (status & ATA_SR_ERR) {
    //     uint16_t error = IDE_read(ATA_SR_ERR);
    //     print("ATA Reads Error: 0x%02x\r\n", (uint8_t)error);
    //     // BAIL
    //     return;
    // }
    
    print("ATA: Identity > ");
    hexdump(ide_buffer, sizeof(ide_buffer));

    DriveIdentity *ident = (DriveIdentity *)ide_buffer;
    print("signature: %04x\r\n", ident->signature);
    
    print("Drive Model: ");
    print_fixed_str(ident->model_no, 20);
    print("Drive_Serial: ");
    print_fixed_str(ident->serial_no, 20);
    print("Drive Firmware Rev: ");
    print_fixed_str(ident->firmware_rev, 8);
    
    uint32_t def_sectors = ((uint32_t)ident->num_sectors_w[1] << 16) | ((uint32_t)ident->num_sectors_w[0]);
    print("Def  C:%u H:%u S:%u -- LBAs: %u\r\n", ident->def_cylinders, ident->def_heads, ident->def_sectors, def_sectors);

    uint32_t curr_sectors = ((uint32_t)ident->curr_lba_sectors_w[1] << 16) | ((uint32_t)ident->curr_lba_sectors_w[0]);
    print("Curr C:%u H:%u S:%u -- LBAs: %u\r\n", ident->curr_cyclinders, ident->curr_heads, ident->curr_sectors, curr_sectors);

    print("Capabilities: %04x\r\n", ident->capabilities);

}

void ata_read_disk(uint16_t address, uint8_t *data, int count) {
    // spin while card is busy
    HAL_Delay(1);
    while (true) {
        int status = IDE_read(ATA_REG_STATUS);
        // Spitting out the UART implicitly has a delay.
        if (status & ATA_SR_DRQ) {
             print("ATA Responds Ready (%02x)\r\n", status);
             break;
        } else { 
            if (status & ATA_SR_BSY) {
                print("ATA Responds Busy (%02x)\r\n", status);
            } else {
                print("ATA NOT Ready (%02x)\r\n", status);
                print("ATA Registers: 1: %02x, 2: %02x, 3: %02x, 4: %02x, 5: %02x, 6: %02x,\r\n", IDE_read(1), IDE_read(2), IDE_read(3), IDE_read(4), IDE_read(5), IDE_read(6));
                break;
            }
        }
    }

    request:

    print("ATA Requesting Read\r\n");

    IDE_write(ATA_REG_SECCOUNT, 1);
    HAL_Delay(1);
    IDE_write(ATA_REG_LBA0, address & 0xFF);
    HAL_Delay(1);
    IDE_write(ATA_REG_LBA1, address >> 8);
    HAL_Delay(1);
    IDE_write(ATA_REG_LBA2, 0);
    HAL_Delay(1);
    IDE_write(ATA_REG_HDDEVSEL, 0xE0);
    HAL_Delay(1);
    IDE_write(ATA_REG_COMMAND, ATA_COMMAND_READ_SECTOR);
    HAL_Delay(100);

    while (true) {
        int status = IDE_read(ATA_REG_STATUS);
        // Spitting out the UART implicitly has a delay.
        if (status & ATA_SR_DRQ) {
             print("ATA Responds Ready (%02x)\r\n", status);
             break;
        } else { 
            if (status & ATA_SR_BSY) {
                print("ATA Responds Busy (%02x)\r\n", status);
            } else {
                print("ATA NOT Ready (%02x)\r\n", status);
                print("ATA Registers: 1: %02x, 2: %02x, 3: %02x, 4: %02x, 5: %02x, 6: %02x,\r\n", IDE_read(1), IDE_read(2), IDE_read(3), IDE_read(4), IDE_read(5), IDE_read(6));
                goto request;
            }
        }
    }

    print("ATA Reading block\r\n");

    ata_read_buffer((uint16_t *)ide_buffer, 256);

    print("ATA: Sector %i > ", address);
    hexdump(ide_buffer, sizeof(ide_buffer));
}
    