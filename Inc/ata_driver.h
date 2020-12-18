#include <stdint.h>
void ata_init(void);
void read_disk(uint16_t address, uint8_t *data, int count);