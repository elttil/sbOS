#include <typedefs.h>

struct PCI_BaseAddressRegister {
  u32 address;
  u32 size;
  // TODO: Add a "type".
};

struct GENERAL_DEVICE {
  u32 base_mem_io;
  u8 interrupt_line;
};

struct PCI_DEVICE {
  u16 vendor;
  u16 device;
  u8 bus;
  u8 slot;
  u8 header_type;
  union {
    struct GENERAL_DEVICE gen;
  };
};

u8 pci_get_bar(const struct PCI_DEVICE *device, u8 bar_index,
               struct PCI_BaseAddressRegister *bar);
u32 pci_config_read32(const struct PCI_DEVICE *device, u8 func, u8 offset);
void pci_config_write32(const struct PCI_DEVICE *device, u8 func, u8 offset,
                        u32 data);

int pci_populate_device_struct(u16 vendor, u16 device,
                               struct PCI_DEVICE *pci_device);
u8 pci_devices_by_id(u8 class_id, u8 subclass_id,
                     struct PCI_DEVICE *pci_device);

u8 pci_get_interrupt_line(const struct PCI_DEVICE *device);
