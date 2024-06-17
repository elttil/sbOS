#include <assert.h>
#include <drivers/pci.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <math.h>
#include <mmu.h>
#include <stdio.h>

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define HBA_PxIS_TFES (1 << 30)

// https://wiki.osdev.org/ATA_Command_Matrix
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35

volatile struct HBA_MEM *hba;

const u16 num_prdt = 8;

typedef enum {
  FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
  FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
  FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
  FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
  FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
  FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
  FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
  FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
} FIS_TYPE;

struct HBA_PRDT_ENTRY {
  u32 dba;  // Data base address
  u32 dbau; // Data base address upper 32 bits
  u32 rsv0; // Reserved

  // DW3
  u32 dbc : 22; // Byte count, 4M max
  u32 rsv1 : 9; // Reserved
  u32 i : 1;    // Interrupt on completion
};

struct HBA_CMD_TBL {
  // 0x00
  u8 cfis[64]; // Command FIS

  // 0x40
  u8 acmd[16]; // ATAPI command, 12 or 16 bytes

  // 0x50
  u8 rsv[48]; // Reserved

  // 0x80
  struct HBA_PRDT_ENTRY
      prdt_entry[1]; // Physical region descriptor table entries, 0 ~ 65535
};

// Host to device
struct FIS_REG_H2D {
  // DWORD 0
  u8 fis_type; // FIS_TYPE_REG_H2D

  u8 pmport : 4; // Port multiplier
  u8 rsv0 : 3;   // Reserved
  u8 c : 1;      // 1: Command, 0: Control

  u8 command;  // Command register
  u8 featurel; // Feature register, 7:0

  // DWORD 1
  u8 lba0;   // LBA low register, 7:0
  u8 lba1;   // LBA mid register, 15:8
  u8 lba2;   // LBA high register, 23:16
  u8 device; // Device register

  // DWORD 2
  u8 lba3;     // LBA register, 31:24
  u8 lba4;     // LBA register, 39:32
  u8 lba5;     // LBA register, 47:40
  u8 featureh; // Feature register, 15:8

  // DWORD 3
  u8 countl;  // Count register, 7:0
  u8 counth;  // Count register, 15:8
  u8 icc;     // Isochronous command completion
  u8 control; // Control register

  // DWORD 4
  u8 rsv1[4]; // Reserved
};

struct HBA_PORT {
  u32 clb;       // 0x00, command list base address, 1K-byte aligned
  u32 clbu;      // 0x04, command list base address upper 32 bits
  u32 fb;        // 0x08, FIS base address, 256-byte aligned
  u32 fbu;       // 0x0C, FIS base address upper 32 bits
  u32 is;        // 0x10, interrupt status
  u32 ie;        // 0x14, interrupt enable
  u32 cmd;       // 0x18, command and status
  u32 rsv0;      // 0x1C, Reserved
  u32 tfd;       // 0x20, task file data
  u32 sig;       // 0x24, signature
  u32 ssts;      // 0x28, SATA status (SCR0:SStatus)
  u32 sctl;      // 0x2C, SATA control (SCR2:SControl)
  u32 serr;      // 0x30, SATA error (SCR1:SError)
  u32 sact;      // 0x34, SATA active (SCR3:SActive)
  u32 ci;        // 0x38, command issue
  u32 sntf;      // 0x3C, SATA notification (SCR4:SNotification)
  u32 fbs;       // 0x40, FIS-based switch control
  u32 rsv1[11];  // 0x44 ~ 0x6F, Reserved
  u32 vendor[4]; // 0x70 ~ 0x7F, vendor specific
};

struct HBA_MEM {
  // 0x00 - 0x2B, Generic Host Control
  u32 cap;     // 0x00, Host capability
  u32 ghc;     // 0x04, Global host control
  u32 is;      // 0x08, Interrupt status
  u32 pi;      // 0x0C, Port implemented
  u32 vs;      // 0x10, Version
  u32 ccc_ctl; // 0x14, Command completion coalescing control
  u32 ccc_pts; // 0x18, Command completion coalescing ports
  u32 em_loc;  // 0x1C, Enclosure management location
  u32 em_ctl;  // 0x20, Enclosure management control
  u32 cap2;    // 0x24, Host capabilities extended
  u32 bohc;    // 0x28, BIOS/OS handoff control and status

  // 0x2C - 0x9F, Reserved
  u8 rsv[0xA0 - 0x2C];

  // 0xA0 - 0xFF, Vendor specific registers
  u8 vendor[0x100 - 0xA0];

  // 0x100 - 0x10FF, Port control registers
  struct HBA_PORT ports[1]; // 1 ~ 32
};

struct HBA_CMD_HEADER {
  // DW0
  u8 cfl : 5; // Command FIS length in DWORDS, 2 ~ 16
  u8 a : 1;   // ATAPI
  u8 w : 1;   // Write, 1: H2D, 0: D2H
  u8 p : 1;   // Prefetchable

  u8 r : 1;    // Reset
  u8 b : 1;    // BIST
  u8 c : 1;    // Clear busy upon R_OK
  u8 rsv0 : 1; // Reserved
  u8 pmp : 4;  // Port multiplier port

  u16 prdtl; // Physical region descriptor table length in entries

  // DW1
  volatile u32 prdbc; // Physical region descriptor byte count transferred

  // DW2, 3
  u32 ctba;  // Command table descriptor base address
  u32 ctbau; // Command table descriptor base address upper 32 bits

  // DW4 - 7
  u32 rsv1[4]; // Reserved
};

#define SATA_SIG_ATA 0x00000101   // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101    // Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

u32 check_type(volatile struct HBA_PORT *port) {
  u32 ssts = port->ssts;

  u8 ipm = (ssts >> 8) & 0x0F;
  u8 det = ssts & 0x0F;

  if (det != HBA_PORT_DET_PRESENT) { // Check drive status
    return AHCI_DEV_NULL;
  }
  if (ipm != HBA_PORT_IPM_ACTIVE) {
    return AHCI_DEV_NULL;
  }

  switch (port->sig) {
  case SATA_SIG_ATAPI:
    return AHCI_DEV_SATAPI;
  case SATA_SIG_SEMB:
    return AHCI_DEV_SEMB;
  case SATA_SIG_PM:
    return AHCI_DEV_PM;
  default:
    return AHCI_DEV_SATA;
  }
}

void ahci_start_command_execution(volatile struct HBA_PORT *port) {
  // Wait for it to stop running.
  // TODO: Figure out if this is really required.
  for (; port->cmd & (1 << 14);)
    ;

  // Start recieving FIS into PxFB
  port->cmd |= ((u32)4 << 0);
  // Start processing of commands
  port->cmd |= ((u32)1 << 0);
}

void ahci_stop_command_execution(volatile struct HBA_PORT *port) {
  // Disable processing of commands
  port->cmd &= ~((u32)1 << 0);
  // Disable recieving FIS into PxFB
  port->cmd &= ~((u32)1 << 4);

  // Check the CR and FR registers to make sure they are no longer running.
  for (; (port->cmd & (1 << 14)) || (port->cmd & (1 << 15));)
    ;
}

// clb_address: size has to be 1024 and byte aligned to 1024
// fb_address: size has to be 256 and byte aligned to 256
// command_table_array: size has to be 256*32
// They are both physical addresses
void ahci_set_base(volatile struct HBA_PORT *port, u32 virt_clb_address,
                   u32 virt_fb_address, u32 virt_command_table_array) {
  u32 clb_address = (u32)virtual_to_physical((void *)virt_clb_address, NULL);
  u32 fb_address = (u32)virtual_to_physical((void *)virt_fb_address, NULL);
  u32 command_table_array =
      (u32)virtual_to_physical((void *)virt_command_table_array, NULL);

  ahci_stop_command_execution(port);

  // Command List Base Address (CLB): Indicates the 32-bit base physical address
  // for the command list for this port. This base is used when fetching
  // commands to execute. This address must be 1K-byte aligned as indicated by
  // bits 09:00 being read only.
  assert(0 == (clb_address & (0x3FF)));
  assert(0 == (fb_address & (0xFF)));
  port->clb = clb_address & (~(u32)(0x3FF));
  port->clbu = 0;

  port->fb = fb_address;
  port->fbu = 0;

  // Command table offset: 40K + 8K*portno
  // Command table size = 256*32 = 8K per port
  struct HBA_CMD_HEADER *cmdheader =
      (struct HBA_CMD_HEADER *)(virt_clb_address);
  for (u8 i = 0; i < 32; i++) {
    cmdheader[i].prdtl = num_prdt; // 8 prdt entries per command table
                                   // 256 bytes per command table, 64+16+48+16*8
    cmdheader[i].ctba = command_table_array + i * 256;
    cmdheader[i].ctbau = 0;
  }

  ahci_start_command_execution(port);
}

void ahci_sata_setup(volatile struct HBA_PORT *port) {
  // clb_address: size has to be 1024 and byte aligned to 1024
  // fb_address: size has to be 256 and byte aligned to 256
  // command_table_array: size has to be 256*32
  u32 clb_address = (u32)ksbrk(1024);
  u32 fb_address = (u32)ksbrk(256);
  u32 command_table_array = (u32)ksbrk(256 * 32);
  create_physical_to_virtual_mapping(
      virtual_to_physical((void *)clb_address, NULL), (void *)clb_address,
      1024);
  create_physical_to_virtual_mapping(
      virtual_to_physical((void *)fb_address, NULL), (void *)fb_address, 256);
  create_physical_to_virtual_mapping(
      virtual_to_physical((void *)command_table_array, NULL),
      (void *)command_table_array, 256 * 32);

  // TODO: Should it be the responsiblity of the caller to make sure these are
  // clear?
  memset((void *)clb_address, 0, 1024);
  memset((void *)fb_address, 0, 256);
  memset((void *)command_table_array, 0, 256 * 32);

  ahci_set_base(port, clb_address, fb_address, command_table_array);
}

// Returns the command slot.
// Sets err if no free slot was found.
u8 get_free_command_slot(volatile struct HBA_PORT *port, u8 *err) {
  u32 slots = (port->sact | port->ci);
  for (u8 i = 0; i < 32; i++) {
    if (!((slots >> i) & 1)) {
      *err = 0;
      return i;
    }
  }
  *err = 1;
  return 0;
}

// is_write: Determins whether a read or write command will be used.
u8 ahci_perform_command(volatile struct HBA_PORT *port, u32 startl, u32 starth,
                        u32 count, u16 *buffer, u8 is_write) {
  // TODO: The number of PRDT tables are hardcoded at a seemingly
  // very low number. It can be up to 65,535. Should it maybe be
  // changed?
  assert(count <= num_prdt);
  port->is = -1; // Clear pending interrupts
  u8 err;
  u32 command_slot = get_free_command_slot(port, &err);
  if (err) {
    klog(LOG_WARN, "AHCI No command slot found");
    return 0;
  }
  struct HBA_CMD_HEADER *cmdheader =
      (struct HBA_CMD_HEADER *)physical_to_virtual((void *)port->clb);
  cmdheader += command_slot;
  cmdheader->w = is_write;
  cmdheader->cfl = sizeof(struct FIS_REG_H2D) / sizeof(u32);
  cmdheader->prdtl = (u16)((count - 1) / 16 + 1); // Number of PRDT

  // Write to the prdtl
  struct HBA_CMD_TBL *cmdtbl =
      (struct HBA_CMD_TBL *)(physical_to_virtual((void *)cmdheader->ctba));

  memset((void *)cmdtbl, 0,
         sizeof(struct HBA_CMD_TBL) +
             (cmdheader->prdtl - 1) * sizeof(struct HBA_PRDT_ENTRY));

  // 8K bytes (16 sectors) per PRDT
  u16 i = 0;
  for (; i < cmdheader->prdtl - 1; i++) {
    cmdtbl->prdt_entry[i].dba = (u32)virtual_to_physical(buffer, NULL);
    cmdtbl->prdt_entry[i].dbc =
        8 * 1024 - 1; // 8K bytes (this value should always be set to 1 less
                      // than the actual value)
    cmdtbl->prdt_entry[i].i = 1;
    buffer += 4 * 1024; // 4K words
    count -= 16;        // 16 sectors
  }
  // FIXME: Edge case if the count does not fit. This should not be here it is
  // ugly. Find a more general case.
  cmdtbl->prdt_entry[i].dba = (u32)virtual_to_physical(buffer, NULL);
  cmdtbl->prdt_entry[i].dbc = count * 512 - 1;
  cmdtbl->prdt_entry[i].i = 1;

  struct FIS_REG_H2D *cmdfis = (struct FIS_REG_H2D *)(&cmdtbl->cfis);

  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;
  cmdfis->command = (is_write) ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;

  cmdfis->lba0 = (u8)startl;
  cmdfis->lba1 = (u8)(startl >> 8);
  cmdfis->lba2 = (u8)(startl >> 16);
  cmdfis->device = 1 << 6; // LBA mode

  cmdfis->lba3 = (u8)(startl >> 24);
  cmdfis->lba4 = (u8)starth;
  cmdfis->lba5 = (u8)(starth >> 8);

  cmdfis->countl = count & 0xFF;
  cmdfis->counth = (count >> 8) & 0xFF;

  // The below loop waits until the port is no longer busy before issuing a new
  // command
  u32 spin = 0;
  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 10000) {
    spin++;
  }
  if (spin == 10000) {
    klog(LOG_ERROR, "AHCI port is hung");
    return 0;
  }

  port->ci = 1 << command_slot; // Issue command

  // Wait for completion
  for (;;) {
    // In some longer duration reads, it may be helpful to spin on the DPS bit
    // in the PxIS port field as well (1 << 5)
    if ((port->ci & (1 << command_slot)) == 0) {
      break;
    }
    if (port->is & HBA_PxIS_TFES) {
      klog(LOG_ERROR, "AHCI command failed");
      return 0;
    }
  }

  // Check again
  if (port->is & HBA_PxIS_TFES) {
    klog(LOG_ERROR, "AHCI command failed");
    return 0;
  }

  return 1;
}

u8 ahci_raw_write(volatile struct HBA_PORT *port, u32 startl, u32 starth,
                  u32 count, u16 *inbuffer) {
  return ahci_perform_command(port, startl, starth, count, inbuffer, 1);
}

u8 ahci_raw_read(volatile struct HBA_PORT *port, u32 startl, u32 starth,
                 u32 count, u16 *outbuffer) {
  return ahci_perform_command(port, startl, starth, count, outbuffer, 0);
}

int ahci_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  vfs_inode_t *inode = fd->inode;
  int port = inode->inode_num;
  assert(port == 0);
  u32 lba = offset / 512;
  offset %= 512;
  const int rc = len;

  u32 sector_count = len / 512;
  if (len % 512 != 0) {
    sector_count++;
  }

  if (offset > 0) {
    u8 tmp_buffer[512];
    ahci_raw_read(&hba->ports[port], lba, 0, 1, (u16 *)tmp_buffer);

    int left = 512 - offset;
    int write = min(left, len);

    memcpy(tmp_buffer + offset, buffer, write);
    ahci_raw_write(&hba->ports[port], lba, 0, 1, (u16 *)tmp_buffer);

    offset = 0;
    len -= write;
    sector_count--;
    lba++;
  }

  for (; sector_count >= num_prdt; lba++) {
    ahci_raw_write(&hba->ports[port], lba, 0, num_prdt, (u16 *)buffer);
    buffer += num_prdt * 512;
    len -= num_prdt * 512;
    sector_count -= num_prdt;
  }

  if (sector_count > 0 && len > 0) {
    u8 tmp_buffer[512 * num_prdt];
    ahci_raw_read(&hba->ports[port], lba, 0, sector_count, (u16 *)tmp_buffer);
    memcpy(tmp_buffer + offset, buffer, len);
    ahci_raw_write(&hba->ports[port], lba, 0, sector_count, (u16 *)tmp_buffer);
  }
  return rc;
}

int ahci_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  vfs_inode_t *inode = fd->inode;
  int port = inode->inode_num;
  assert(port == 0);
  u32 lba = offset / 512;
  offset %= 512;
  int rc = len;

  u32 sector_count = len / 512;
  if (len % 512 != 0) {
    sector_count++;
  }
  u8 tmp_buffer[512 * num_prdt];
  for (; sector_count >= num_prdt; lba++) {
    ahci_raw_read(&hba->ports[port], lba, 0, num_prdt, (u16 *)tmp_buffer);
    memcpy(buffer, tmp_buffer + offset, 512 * num_prdt);
    offset = 0;
    buffer += num_prdt * 512;
    len -= num_prdt * 512;
    sector_count -= num_prdt;
  }

  if (sector_count > 0) {
    ahci_raw_read(&hba->ports[port], lba, 0, sector_count, (u16 *)tmp_buffer);
    memcpy(buffer, tmp_buffer + offset, len);
  }
  return rc;
}

int ahci_has_data(vfs_inode_t *inode) {
  (void)inode;
  return 1;
}

void add_devfs_drive_file(u8 port) {
  static u8 num_drives_added = 0;
  char *path = "/sda";
  path[strlen(path)] += num_drives_added;
  num_drives_added++;
  vfs_inode_t *inode =
      devfs_add_file(path, ahci_read, ahci_write, NULL /*get_vm_object*/,
                     ahci_has_data, NULL /*can_write*/, FS_TYPE_BLOCK_DEVICE);
  inode->inode_num = port;
}

int ahci_init(void) {
  struct PCI_DEVICE device;
  if (!pci_devices_by_id(0x01, 0x06, &device)) {
    return 0;
  }
  kprintf("vendor: %x\n", device.vendor);
  kprintf("device: %x\n", device.device);
  kprintf("header_type: %x\n", device.header_type);

  struct PCI_BaseAddressRegister bar;
  pci_get_bar(&device, 5, &bar);

  u8 *HBA_base = mmu_map_frames((void *)bar.address, bar.size);
  if (!HBA_base) {
    return 0;
  }
  hba = (volatile struct HBA_MEM *)(HBA_base);
  for (u8 i = 0; i < 32; i++) {
    if (!((hba->pi >> i) & 1)) {
      continue;
    }
    u32 type = check_type(&hba->ports[i]);
    switch (type) {
    case AHCI_DEV_SATA:
      ahci_sata_setup(&hba->ports[i]);
      add_devfs_drive_file(i);
      kprintf("SATA drive found at port %d\n", i);
      break;
    case AHCI_DEV_SATAPI:
      kprintf("SATAPI drive found at port %d\n", i);
      break;
    case AHCI_DEV_SEMB:
      kprintf("SEMB drive found at port %d\n", i);
      break;
    case AHCI_DEV_PM:
      kprintf("PM drive found at port %d\n", i);
      break;
    default:
      kprintf("No drive found at port %d\n", i);
      break;
    }
  }
  return 1;
}
