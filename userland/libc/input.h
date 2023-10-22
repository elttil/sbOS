#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>

struct input_event {
  uint16_t key;
  uint8_t status;
};
#endif
