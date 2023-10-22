#include "../cpu/idt.h"
#include <stddef.h>

#define SECTOR_SIZE 512

void ata_init(void);

void read_drive_chs(uint8_t head_index, uint8_t sector_count,
                    uint8_t sector_index, uint8_t cylinder_low_value,
                    uint8_t cylinder_high_value, void *address);
void read_drive_lba(uint32_t lba, uint8_t sector_count, void *address);
void read_lba(uint32_t lba, void *address, size_t size, size_t offset);
void write_lba(uint32_t lba, volatile void *address, size_t size,
               size_t offset);
void ata_write_lba28(uint32_t lba, uint32_t sector_count,
                     volatile const uint8_t *buffer);
