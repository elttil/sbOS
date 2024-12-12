#include <stddef.h>
#include <typedefs.h>

void ac97_init(void);
int ac97_add_pcm(u8 *buffer, size_t len);
int ac97_can_write(void);
void ac97_set_volume(int volume);
int ac97_get_volume(void);
