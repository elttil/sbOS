#include <assert.h>
#include <kmalloc.h>
#include <lib/ringbuffer.h>
#include <math.h>
#include <string.h>

int ringbuffer_init(struct ringbuffer *rb, u32 buffer_size) {
  rb->buffer_size = 0;
  rb->read_ptr = 0;
  rb->write_ptr = 0;
  rb->buffer = kmalloc(buffer_size);
  if (!rb->buffer) {
    return 0;
  }
  rb->buffer_size = buffer_size;
  return 1;
}

u32 ringbuffer_write(struct ringbuffer *rb, const u8 *buffer, u32 len) {
  const u32 orig_len = len;
  for (; len > 0;) {
    u32 buffer_left;
    if (rb->write_ptr >= rb->read_ptr) {
      buffer_left = rb->buffer_size - rb->write_ptr;
      if (0 == rb->read_ptr && buffer_left > 0) {
        buffer_left--;
      }
    } else {
      buffer_left = (rb->read_ptr - rb->write_ptr - 1);
    }

    if (0 == buffer_left) {
      break;
    }

    u32 write_len = min(len, buffer_left);
    write_len = min(write_len, rb->buffer_size - rb->write_ptr);
    memcpy(rb->buffer + rb->write_ptr, buffer, write_len);
    buffer += write_len;
    len -= write_len;
    rb->write_ptr = (rb->write_ptr + write_len) % rb->buffer_size;
  }
  return orig_len - len;
}

u32 ringbuffer_read(struct ringbuffer *rb, u8 *buffer, u32 len) {
  const u32 orig_len = len;
  for (; len > 0;) {
    if (rb->read_ptr == rb->write_ptr) {
      break;
    }
    u32 read_len;
    if (rb->read_ptr >= rb->write_ptr) {
      read_len = rb->buffer_size - rb->read_ptr;
    } else if (rb->read_ptr < rb->write_ptr) {
      read_len = rb->write_ptr - rb->read_ptr;
    }
    read_len = min(len, read_len);

    memcpy(buffer, rb->buffer + rb->read_ptr, read_len);
    len -= read_len;
    buffer += read_len;
    rb->read_ptr = (rb->read_ptr + read_len) % rb->buffer_size;
  }
  return orig_len - len;
}

int ringbuffer_isempty(const struct ringbuffer *rb) {
  return (rb->write_ptr == rb->read_ptr);
}

void ringbuffer_free(struct ringbuffer *rb) {
  kfree(rb->buffer);
  rb->buffer = NULL;
  rb->buffer_size = 0;
  rb->write_ptr = 0;
  rb->read_ptr = 0;
}

#ifdef KERNEL_TEST
void ringbuffer_test(void) {
  char buffer[4096];
  struct ringbuffer rb;
  assert(ringbuffer_init(&rb, 6));
  assert(2 == ringbuffer_write(&rb, "ab", 2));
  assert(2 == ringbuffer_write(&rb, "cd", 2));
  assert(1 == ringbuffer_write(&rb, "ef", 2));
  assert(0 == ringbuffer_write(&rb, "gh", 2));
  assert(2 == ringbuffer_read(&rb, buffer, 2));
  assert(0 == memcmp(buffer, "ab", 2));
  assert(2 == ringbuffer_read(&rb, buffer, 2));
  assert(0 == memcmp(buffer, "cd", 2));
  assert(1 == ringbuffer_read(&rb, buffer, 2));
  assert(0 == memcmp(buffer, "e", 1));
  assert(0 == ringbuffer_read(&rb, buffer, 2));
  ringbuffer_free(&rb);
}
#endif // KERNEL_TEST
