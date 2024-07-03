#include <arch/i386/tsc.h>
#include <drivers/cmos.h>

void timer_init(void);
void timer_get(struct timespec *tp);
u64 timer_get_uptime(void);
u64 timer_get_ms(void);
int timer_add_clock(void);
