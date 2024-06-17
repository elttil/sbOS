#include <assert.h>
#include <cpu/io.h>
#include <drivers/ata.h>
#include <drivers/pit.h>

#define PRIMARY_BUS_BASEPORT 0x1F0
#define SECONDAY_BUS_BASEPORT 0x170

#define PRIMARY_BUS_IRQ 14
#define SECONDAY_BUS_IRQ 15

#define STATUS_PORT 7
#define COMMAND_PORT 7
#define DRIVE_SELECT 6
#define LBAhi 5
#define LBAmid 4
#define LBAlo 3
#define SECTOR_COUNT 2
#define DATA_PORT 0

#define IDENTIFY 0xEC
#define READ_SECTORS 0x20
#define WRITE_SECTORS 0x30
#define CACHE_FLUSH 0xE7

#define STATUS_BSY ((1 << 7))
#define STATUS_DF ((1 << 5))
#define STATUS_DRQ ((1 << 3))
#define STATUS_ERR ((1 << 0))

u32 io_base;

unsigned char read_buffer[SECTOR_SIZE];

void select_drive(u8 master_slave) {
  outb(io_base + DRIVE_SELECT, (master_slave) ? 0xA0 : 0xB0);
}

int identify(int master_slave) {
  select_drive(master_slave);
  outb(io_base + SECTOR_COUNT, 0);
  outb(io_base + LBAlo, 0);
  outb(io_base + LBAmid, 0);
  outb(io_base + LBAhi, 0);
  outb(io_base + COMMAND_PORT, IDENTIFY);
  if (0 == inb(io_base + STATUS_PORT)) {
    return 0; // Drive does not exist
  }

  for (; 0 != (inb(io_base + STATUS_PORT) & STATUS_BSY);)
    ;

  // Because of some ATAPI drives that do not
  // follow spec, at this point we need to check
  // the LBAmid and LBAhi ports to see if they are
  // non-zero. If so, the drive is not ATA, and we
  // should stop polling.
  if (0 != inb(io_base + LBAmid) || 0 != inb(io_base + LBAhi)) {
    klog(LOG_ERROR, "Drive is not ATA.");
    return -1;
  }

  for (u16 status;;) {
    status = inb(io_base + STATUS_PORT);

    if (1 == (status & STATUS_ERR)) {
      klog(LOG_ERROR, "Drive ERR set.");
      return -2;
    }

    if ((status & STATUS_DRQ)) {
      break;
    }
  }

  // The data is ready to read from the Data
  // port (0x1F0). Read 256 16-bit values,
  // and store them.
  // TODO: This returns some intreasting information.
  // https://wiki.osdev.org/ATA_PIO_Mode#Interesting_information_returned_by_IDENTIFY
  u16 array[256];
  rep_insw(1 * SECTOR_SIZE / 16, io_base + DATA_PORT, array);
  return 1;
}

int poll_status(void) {
  for (u16 status;;) {
    // Read the Regular Status port until...
    // We read this 15 times to give some
    // time for the drive to catch up.
    for (int n = 0; n < 15; n++) {
      status = inb(io_base + STATUS_PORT);
    }

    // ERR or
    // DF sets
    if ((status & STATUS_ERR) || (status & STATUS_DF)) {
      klog(LOG_ERROR, "Drive error set.");
      return 0;
    }

    // BSY clears
    // DRQ sets
    if (0 == (status & STATUS_BSY) && 0 != (status & STATUS_DRQ)) {
      break;
    }
  }
  return 1;
}

// Instructions from: https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
void setup_drive_for_command(u32 lba, u32 sector_count) {
  // 1. Send 0xE0 for the "master" or 0xF0 for
  //    the "slave", ORed with the highest 4 bits
  //    of the LBA to port 0x1F6
  outb(io_base + DRIVE_SELECT, 0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

  // 2. Send a NULL byte to port 0x1F1, if you
  //    like (it is ignored and wastes
  //    lots of CPU time)

  // NOP

  // 3. Send the sectorcount to port 0x1F2
  outb(io_base + SECTOR_COUNT, sector_count);

  // 4. Send the low 8 bits of the LBA to port 0x1F3
  outb(io_base + LBAlo, (lba >> 0) & 0xFF);

  // 5. Send the next 8 bits of the LBA to port 0x1F4
  outb(io_base + LBAmid, (lba >> 8) & 0xFF);

  // 6. Send the next 8 bits of the LBA to port 0x1F5
  outb(io_base + LBAhi, (lba >> 16) & 0xFF);
}

void delayed_rep_outsw(size_t n, u16 port, volatile u8 *buffer) {
  for (volatile size_t i = 0; i < n; i++) {
    outsw(port, (u32)buffer);
    buffer += 2;
    //    outsw(port, buffer);
  }
}

void ata_write_lba28(u32 lba, u32 sector_count, volatile const u8 *buffer) {
  setup_drive_for_command(lba, sector_count);

  outb(io_base + COMMAND_PORT, WRITE_SECTORS);

  for (volatile u32 i = 0; i < sector_count; i++) {
    if (!poll_status()) {
      // FIXME: Fail properly
      for (;;)
        ;
    }

    delayed_rep_outsw(256, io_base + DATA_PORT,
                      (void *)((u32)buffer + i * 256));
  }

  // Cache flush
  outb(io_base + COMMAND_PORT, CACHE_FLUSH);

  // Wait for BSY to clear
  for (;;) {
    u16 status = inb(io_base + STATUS_PORT);
    if (!(status & STATUS_BSY)) {
      break;
    }
  }
}

// Instructions from: https://wiki.osdev.org/ATA_PIO_Mode#28_bit_PIO
void ata_read_lba28(u32 lba, u32 sector_count, volatile void *address) {
  // Step 1-6 is done in this function.
  setup_drive_for_command(lba, sector_count);

  // 7. Send the "READ SECTORS" command to port 0x17F
  outb(io_base + COMMAND_PORT, READ_SECTORS);

  // 8. Wait for an IRQ or poll.

  // This step can be found in the for loop

  // 9. Transfer 256 16-bit values, a u16 at a time,
  //    into your buffer from I/O port 0x1F0
  for (volatile u32 i = 0; i < sector_count; i++) {
    // 10. Then loop back to waiting for the next IRQ
    // or poll again for each successive sector.
    // 8. Wait for an IRQ or poll.
    if (!poll_status()) {
      // FIXME: Fail properly
      for (;;)
        ;
    }
    rep_insw(256, io_base + DATA_PORT, (void *)((u32)address + i * 256));
  }
}

void ata_init(void) {
  io_base = PRIMARY_BUS_BASEPORT;

  // Before sending any data to the IO ports,
  // read the Regular Status byte. The value
  // 0xFF is an illegal status value, and
  // indicates that the bus has no drives
  if (0xFF == inb(io_base + STATUS_PORT)) {
    klog(LOG_ERROR, "Bus has no drives");
  }

  // Issue IDENTIFY command
  select_drive(1);
}

void read_lba(u32 lba, void *address, size_t size, size_t offset) {
  uintptr_t ptr = (uintptr_t)address;
  lba += offset / SECTOR_SIZE;
  offset = offset % SECTOR_SIZE;
  size_t total_read = 0;
  for (int i = 0; size > 0; i++) {
    u32 read_len =
        (SECTOR_SIZE < (size + offset)) ? (SECTOR_SIZE - offset) : size;
    ata_read_lba28(lba + i, 1, read_buffer);
    memcpy((void *)ptr, read_buffer + offset, read_len);
    size -= read_len;
    total_read += read_len;
    ptr += read_len;
    offset = 0;
  }
}

void write_lba(u32 lba, volatile void *address, size_t size, size_t offset) {
  uintptr_t ptr = (uintptr_t)address;
  lba += offset / SECTOR_SIZE;
  offset = offset % SECTOR_SIZE;
  size_t total_write = 0;
  u8 sector_buffer[512];
  for (int i = 0; size > 0; i++) {
    ata_read_lba28(lba + i, 1, sector_buffer);
    u32 write_len =
        (SECTOR_SIZE < (size + offset)) ? (SECTOR_SIZE - offset) : size;

    memcpy(sector_buffer + offset, (void *)ptr, write_len);
    ata_write_lba28(lba + i, 1, sector_buffer);

    size -= write_len;
    total_write += write_len;
    ptr += write_len;
    offset = 0;
  }
}
