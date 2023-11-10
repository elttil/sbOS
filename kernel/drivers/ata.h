#include "../cpu/idt.h"
#include <stddef.h>

#define SECTOR_SIZE 512

void ata_init(void);

void read_drive_chs(u8 head_index, u8 sector_count,
                    u8 sector_index, u8 cylinder_low_value,
                    u8 cylinder_high_value, void *address);
void read_drive_lba(u32 lba, u8 sector_count, void *address);
void read_lba(u32 lba, void *address, size_t size, size_t offset);
void write_lba(u32 lba, volatile void *address, size_t size,
               size_t offset);
void ata_write_lba28(u32 lba, u32 sector_count,
                     volatile const u8 *buffer);
