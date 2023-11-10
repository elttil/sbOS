#include <typedefs.h>

struct GENERAL_DEVICE {
  u32 base_mem_io;
  u8 interrupt_line;
};

struct PCI_DEVICE {
  u16 vendor;
  u16 device;
  u8 bus;
  u8 slot;
  union {
    struct GENERAL_DEVICE gen;
  };
};

u32 pci_config_read32(const struct PCI_DEVICE *device, u8 func,
                           u8 offset);
void pci_config_write32(const struct PCI_DEVICE *device, u8 func,
                        u8 offset, u32 data);

int pci_populate_device_struct(u16 vendor, u16 device,
                               struct PCI_DEVICE *pci_device);

u8 pci_get_interrupt_line(const struct PCI_DEVICE *device);
