#include <assert.h>
#include <cpu/io.h>
#include <drivers/pci.h>
#include <kmalloc.h>
#include <stdio.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

// Gets the bar address and size the populates the struct("bar") given.
// Return value:
// 1 Success
// 0 Failure
u8 pci_get_bar(const struct PCI_DEVICE *device, u8 bar_index,
               struct PCI_BaseAddressRegister *bar) {
  if (bar_index > 5)
    return 0;
  u8 offset = 0x10 + bar_index * sizeof(u32);
  u32 physical_bar = pci_config_read32(device, 0, offset);
  u32 original_bar = physical_bar;
  physical_bar &= 0xFFFFFFF0;
  // Now we do the konami code of PCI devices to figure out the size of what
  // the BAR is pointing to.

  // Comments taken from https://wiki.osdev.org/PCI#Address_and_size_of_the_BAR

  // write a value of all 1's to the register,
  pci_config_write32(device, 0, 0x24, 0xFFFFFFFF);
  // then read it back.
  u32 bar_result = pci_config_read32(device, 0, 0x24);

  // The amount of memory can then be determined by masking the information
  // bits,
  bar_result &=
      ~(0xF); // Apparently the "information bits" are the last 4 bits according
              // to this answer: https://stackoverflow.com/a/39618552

  // performing a bitwise NOT ('~' in C),
  bar_result = ~bar_result;

  // and incrementing the value by 1.
  bar_result++;

  // Restore the old result
  pci_config_write32(device, 0, 0x24, original_bar);

  bar->address = physical_bar;
  bar->size = bar_result;
  return 1;
}

void pci_config_write32(const struct PCI_DEVICE *device, u8 func, u8 offset,
                        u32 data) {
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

u32 pci_config_read32(const struct PCI_DEVICE *device, u8 func, u8 offset) {
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

// Returns number of devices found.
u8 pci_devices_by_id(u8 class_id, u8 subclass_id,
                     struct PCI_DEVICE *pci_device) {
  for (u8 bus = 0; bus < 255; bus++) {
    for (u8 slot = 0; slot < 255; slot++) {
      pci_device->bus = bus;
      pci_device->slot = slot;
      u16 class_info = pci_config_read32(pci_device, 0, 0x8) >> 16;
      u16 h_classcode = (class_info & 0xFF00) >> 8;
      u16 h_subclass = (class_info & 0x00FF);
      if (h_classcode != class_id)
        continue;
      if (h_subclass != subclass_id)
        continue;

      u32 device_vendor = pci_config_read32(pci_device, 0, 0);
      pci_device->vendor = (device_vendor & 0xFFFF);
      pci_device->device = (device_vendor >> 16);

      u32 BIST_headertype_latencytimer_cachesize =
          pci_config_read32(pci_device, 0, 0xC);
      pci_device->header_type = BIST_headertype_latencytimer_cachesize >> 16;
      pci_device->header_type &= 0xFF;
      return 1;
    }
  }
  return 0;
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
