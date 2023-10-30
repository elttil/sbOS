#include <stdint.h>

struct GENERAL_DEVICE {
  uint32_t base_mem_io;
  uint8_t interrupt_line;
};

struct PCI_DEVICE {
  uint16_t vendor;
  uint16_t device;
  uint8_t bus;
  uint8_t slot;
  union {
    struct GENERAL_DEVICE gen;
  };
};

uint32_t pci_config_read32(const struct PCI_DEVICE *device, uint8_t func,
                           uint8_t offset);
void pci_config_write32(const struct PCI_DEVICE *device, uint8_t func,
                        uint8_t offset, uint32_t data);

int pci_populate_device_struct(uint16_t vendor, uint16_t device,
                               struct PCI_DEVICE *pci_device);

uint8_t pci_get_interrupt_line(const struct PCI_DEVICE *device);
