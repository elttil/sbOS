#include <assert.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <stdio.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

void pci_config_write32(const struct PCI_DEVICE *device, u8 func,
                        u8 offset, u32 data) {
  u32 address;
  u32 lbus = (u32)device->bus;
  u32 lslot = (u32)device->slot;
  u32 lfunc = (u32)func;

  // Create configuration address as per Figure 1
  address = (u32)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((u32)0x80000000));

  // Write out the address
  outl(CONFIG_ADDRESS, address);
  outl(CONFIG_DATA, data);
}

u32 pci_config_read32(const struct PCI_DEVICE *device, u8 func,
                           u8 offset) {
  u32 address;
  u32 lbus = (u32)device->bus;
  u32 lslot = (u32)device->slot;
  u32 lfunc = (u32)func;

  // Create configuration address as per Figure 1
  address = (u32)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((u32)0x80000000));

  // Write out the address
  outl(CONFIG_ADDRESS, address);
  return inl(CONFIG_DATA);
}

int pci_populate_device_struct(u16 vendor, u16 device,
                               struct PCI_DEVICE *pci_device) {
  pci_device->vendor = vendor;
  pci_device->device = device;

  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 256; slot++) {
      struct PCI_DEVICE tmp;
      tmp.bus = bus;
      tmp.slot = slot;
      u32 device_vendor = pci_config_read32(&tmp, 0, 0);
      if (vendor != (device_vendor & 0xFFFF))
        continue;
      if (device != (device_vendor >> 16))
        continue;
      pci_device->bus = bus;
      pci_device->slot = slot;
      u32 bar0 = pci_config_read32(pci_device, 0, 0x10);
      assert(bar0 & 0x1 && "Only support memory IO");
      pci_device->gen.base_mem_io = bar0 & (~0x3);
      return 1;
    }
  }
  return 0;
}

u8 pci_get_interrupt_line(const struct PCI_DEVICE *device) {
  return pci_config_read32(device, 0, 0x3C) & 0xFF;
}
