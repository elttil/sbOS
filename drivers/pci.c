#include <cpu/io.h>
#include <drivers/pci.h>
#include <stdio.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

uint32_t pci_config_read32(const struct PCI_DEVICE *device, uint8_t func,
                           uint8_t offset) {
  uint32_t address;
  uint32_t lbus = (uint32_t)device->bus;
  uint32_t lslot = (uint32_t)device->slot;
  uint32_t lfunc = (uint32_t)func;

  // Create configuration address as per Figure 1
  address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                       (offset & 0xFC) | ((uint32_t)0x80000000));

  // Write out the address
  outl(CONFIG_ADDRESS, address);
  return inl(CONFIG_DATA);
}

int pci_populate_device_struct(uint16_t vendor, uint16_t device,
                               struct PCI_DEVICE *pci_device) {
  pci_device->vendor = vendor;
  pci_device->device = device;

  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 256; slot++) {
      struct PCI_DEVICE tmp;
      tmp.bus = bus;
      tmp.slot = slot;
      uint32_t device_vendor = pci_config_read32(&tmp, 0, 0);
      if (vendor != (device_vendor & 0xFFFF))
        continue;
      if (device != (device_vendor >> 16))
        continue;
      pci_device->bus = bus;
      pci_device->slot = slot;
      return 1;
    }
  }
  return 0;
}
