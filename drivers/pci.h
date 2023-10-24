#include <stdint.h>

struct PCI_DEVICE {
  uint16_t vendor;
  uint16_t device;
  uint8_t bus;
  uint8_t slot;
};

uint32_t pci_config_read32(const struct PCI_DEVICE *device, uint8_t func,
                           uint8_t offset);

int pci_populate_device_struct(uint16_t vendor, uint16_t device,
                               struct PCI_DEVICE *pci_device);
