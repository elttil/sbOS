#include <typedefs.h>
#include <stddef.h>

void ac97_init(void);
int ac97_add_pcm(u8* buffer, size_t len);
int ac97_can_write(void);
