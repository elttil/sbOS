#include <arch/i386/tsc.h>

u64 cpu_mhz = 0;
u64 tsc_get_hz(void);

void tsc_init(void) {
  cpu_mhz = tsc_get_hz() / 10000;
}

u64 tsc_get_mhz() {
  return cpu_mhz;
}

u64 tsc_calculate_ms(u64 tsc) {
  return tsc / (cpu_mhz * 1000);
}
