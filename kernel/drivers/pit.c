#include "pit.h"

#define PIT_IO_CHANNEL_0 0x40
#define PIT_IO_MODE_COMMAND 0x43

uint64_t clock_num_ms_ticks = 0;
uint64_t pit_counter = 0;
uint16_t hertz;

uint64_t pit_num_ms(void) { return clock_num_ms_ticks; }

uint16_t read_pit_count(void) {
  uint16_t count = 0;

  outb(PIT_IO_MODE_COMMAND, 0x0 /*0b00000000*/);

  count = inb(PIT_IO_CHANNEL_0);
  count |= inb(PIT_IO_CHANNEL_0) << 8;

  return count;
}

void set_pit_count(uint16_t hertz) {
  uint16_t divisor = 1193180 / hertz;

  /*
   * 0b00110110
   *   ^^
   * channel - 0
   *     ^^
   * r/w mode - LSB then MSB
   *       ^^^
   * mode - 3 Square Wave Mode
   *          ^
   * BCD - no
   */
  outb(PIT_IO_MODE_COMMAND, 0x36 /*0b00110110*/);
  outb(PIT_IO_CHANNEL_0, divisor & 0xFF);
  outb(PIT_IO_CHANNEL_0, (divisor & 0xFF00) >> 8);
}

__attribute__((interrupt)) void
int_clock(__attribute__((unused)) struct interrupt_frame *frame) {
  outb(0x20, 0x20);
  pit_counter++;
  if (pit_counter >= hertz / 1000) {
    pit_counter = 0;
    clock_num_ms_ticks++;
  }
  switch_task();
}

void pit_install(void) {
  install_handler(int_clock, INT_32_INTERRUPT_GATE(0x0), 0x20);
}
