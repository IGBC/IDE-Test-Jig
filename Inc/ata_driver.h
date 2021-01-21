#include <stdint.h>
void ata_init(void);
uint16_t ata_read_disk(uint16_t address, uint8_t *data, int count);
uint16_t ata_write_disk(uint16_t address, uint8_t *data, int count);
