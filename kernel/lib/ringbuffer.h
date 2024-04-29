#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <typedefs.h>

struct ringbuffer {
  u8 *buffer;
  u32 buffer_size;
  u32 read_ptr;
  u32 write_ptr;
};

int ringbuffer_init(struct ringbuffer *rb, u32 buffer_size);
u32 ringbuffer_write(struct ringbuffer *rb, const u8 *buffer, u32 len);
u32 ringbuffer_read(struct ringbuffer *rb, u8 *buffer, u32 len);
int ringbuffer_isempty(const struct ringbuffer *rb);
void ringbuffer_free(struct ringbuffer *rb);
u32 ringbuffer_used(const struct ringbuffer *rb);
u32 ringbuffer_capacity(const struct ringbuffer *rb);
u32 ringbuffer_unused(const struct ringbuffer *rb);
#ifdef KERNEL_TEST
void ringbuffer_test(void);
#endif // KERNEL_TEST
#endif // RINGBUFFER_H
