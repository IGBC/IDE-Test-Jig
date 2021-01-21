#include "ata_driver.h"

#include <string.h> //using for memset.
#include <stdbool.h>

#define HEADER_MAGIC_NUM 0x494445415544494f // "IDEAUDIO" in ascii, its 8 bytes so is a uint64_t
#define TAPE_PARTITION_TYPE 0x23 // originally windows mobile boot, but I think its safe to reuse.
#define TAPE_NAME_LEN 64


typedef struct __attribute__((__packed__)) {
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t start_lba_sector;
    uint32_t lba_sector_count;
} Partition_Entry;


typedef struct __attribute__((__packed__)) {
    uint8_t bootcode[446];
    Partition_Entry partition[4];
    uint16_t boot_signature;
} Mbr;


typedef struct __attribute__((__packed__)) {
    uint64_t magic_number;   // identifier to determine this is indeed a tape partition
    uint8_t major_ver;       // major version of this header
    uint8_t minor_ver;       // minor version of this header
    uint8_t channel_count;   // number of channels on this tape
    uint8_t word_len;        // length of a word on this tape
    uint32_t tape_start;     // relative offset to first tape block
    uint32_t tape_len;       // number of strides in this tape 
                             // (convert back to lbas by multiplying by stride)
    uint32_t sample_rate;    // sample rate of tape
    char name [TAPE_NAME_LEN]; // Name of this disk
    // current size 84 bytes
    // newer versions of this header will grow down.
    // the maximum size of this header is 512 bytes, or one block.
} Header;


typedef struct {
    uint32_t start_lba;
    uint32_t lba_count;
    uint8_t type;
} LbaPartition;


typedef struct {
    // AtaController *ata_drive;
    bool disk_valid;                // is this disk formatted correctly
    unsigned int header_offset_lba; // start of partition, in LBAs.
    unsigned int tape_offset_lba;   // start of the linear data, in LBAs
    unsigned char header_ver[2];    // data version of this disk [0] major, [1] minor
    unsigned int n_channels;        // number of channels on this disk
    unsigned int bit_depth;         // number of bytes per sample
    unsigned int stride;            // offset between blocks, of a channel (=n_channels * bit_depth)
    char disk_name[TAPE_NAME_LEN];    // Label on this disk
} VirtualTape;

// This is a sratch buffer for storing temporary data retrived from disk.
// Its contents nor any pointers from within it are to be passed outside of
// the boundry of a function. This is treated like a super local variable.
// Every time a function call or return is made the contents of this variable are
// to be considered invalid. That is because this variable is used by all header 
// manipulation functions to store data about to be read/writen from/to disk
// this will save a LOT of stack space, as every function that needs to access the disk
// would otherwise need a 512 byte local variable, these add up quickly.
uint8_t scratch[512] = {0};

VirtualTape tape = {0};


void onDriveAttach(uint32_t disk_len_lba, void *ctx);
void onDriveDetach(void *ctx);

void readMbr(LbaPartition[4]);


void initDisk() {
    LbaPartition partition[4];
    readMbr(partition);
    for (int i = 0; i<4; i++) {
        if (partition[i].type == TAPE_PARTITION_TYPE) {
            ata_read_disk(partition[i].start_lba, scratch, 1);
            Header *header = (Header*)scratch;
	    if (header->magic_number == HEADER_MAGIC_NUM) {
                tape.header_offset_lba = partition[i].start_lba;
		tape.tape_offset_lba = tape.header_offset_lba + header->tape_start;
		tape.disk_valid = true;
		tape.n_channels = header->channel_count;
		tape.bit_depth = header->word_len;
		tape.stride = tape.n_channels * tape.bit_depth;
		strncpy(tape.disk_name, header->name, TAPE_NAME_LEN);
		return;
            }
	}
    }
}


void readMbr(LbaPartition* partitions) {
    //LbaPartition partitions[4];
    ata_read_disk(0, scratch, 1);
    Mbr *mbr = (Mbr*)scratch;
    for (int i = 0; i<4; i++) {
        partitions[i].start_lba = mbr->partition[i].start_lba_sector;
        partitions[i].lba_count = mbr->partition[i].lba_sector_count;
        partitions[i].type = mbr->partition[i].type;
    }
}

void formatDisk(uint32_t disk_len, uint8_t n_channels, uint32_t rate) {
    // write mbr
    memset(scratch, 0, sizeof(scratch));
    Mbr *mbr = (Mbr*)scratch;
    // todo figure out how to correctly ignore CHS adressing.
    // mbr->partition[0].start_sector = 1;
    mbr->partition[0].start_lba_sector = 1;
    mbr->partition[0].lba_sector_count = disk_len - 1;
    ata_write_disk(0, scratch, 1);
    // write header
    memset(scratch, 0, sizeof(scratch));
    uint32_t len = disk_len - 1024; // this is the preallocated space for the ToC and any patches.
    uint8_t stride = n_channels * 2;
    uint32_t tape_len = len / stride;
    Header *header = (Header*)scratch;
    header->magic_number = HEADER_MAGIC_NUM;
    header->major_ver = 0;
    header->minor_ver = 0;
    header->channel_count = n_channels;
    header->word_len = 2;
    header->tape_start = 1023;
    header->tape_len = tape_len;
    header->sample_rate = rate;
    strncpy(header->name, "Untitiled Disk", TAPE_NAME_LEN);
    ata_write_disk(1, scratch, 1);
}

