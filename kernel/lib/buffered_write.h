#include <typedefs.h>

struct buffered {
  u8 *data;
  u16 buffer_usage;
  u16 buffer_size;
};

int buffered_init(struct buffered *ctx, u16 size);
int buffered_write(struct buffered *ctx, u8 *data, u16 length);
void buffered_clear(struct buffered *ctx);
void buffered_free(struct buffered *ctx);
