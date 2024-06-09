#include <kmalloc.h>
#include <lib/buffered_write.h>
#include <string.h>

int buffered_init(struct buffered *ctx, u16 size) {
  ctx->data = kmalloc(size);
  if (NULL == ctx->data) {
    return 0;
  }
  ctx->buffer_size = size;
  ctx->buffer_usage = 0;
  return 1;
}

int buffered_write(struct buffered *ctx, u8 *data, u16 length) {
  if (length + ctx->buffer_usage > ctx->buffer_size) {
    return 0;
  }
  u8 *data_ptr = ctx->data;
  data_ptr += ctx->buffer_usage;

  memcpy(data_ptr, data, length);
  ctx->buffer_usage += length;
  return 1;
}

void buffered_clear(struct buffered *ctx) {
  ctx->buffer_usage = 0;
}

void buffered_free(struct buffered *ctx) {
  kfree(ctx->data);
}
