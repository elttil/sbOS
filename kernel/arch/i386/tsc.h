#include <typedefs.h>

void tsc_init(void);
u64 tsc_get(void);
u64 tsc_get_mhz(void);
u64 tsc_calculate_ms(u64 tsc);
