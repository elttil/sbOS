#include "pit.h"

#define PIT_IO_CHANNEL_0 0x40
#define PIT_IO_MODE_COMMAND 0x43

u64 clock_num_ms_ticks = 0;
u32 pit_counter = 0;
u32 switch_counter = 0;
u16 hertz;

u64 pit_num_ms(void) {
  return clock_num_ms_ticks;
}

u16 read_pit_count(void) {
  u16 count = 0;

  outb(PIT_IO_MODE_COMMAND, 0x0 /*0b00000000*/);

  count = inb(PIT_IO_CHANNEL_0);
  count |= inb(PIT_IO_CHANNEL_0) << 8;

  return count;
}

void set_pit_count(u16 _hertz) {
  hertz = _hertz;
  u16 divisor = 1193180 / hertz;

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

extern int is_switching_tasks;
void int_clock(reg_t *regs) {
  clock_num_ms_ticks += 5;
  switch_counter++;
  if (switch_counter >= hertz) {
    EOI(0x20);
    switch_counter = 0;
    if(is_switching_tasks) {
      return;
    }
    switch_task();
  } else {
    EOI(0x20);
  }
}

void pit_install(void) {
  install_handler(int_clock, INT_32_INTERRUPT_GATE(0x3), 0x20);
}
