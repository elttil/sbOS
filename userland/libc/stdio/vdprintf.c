#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct vd_cookie {
  int fd;
  char *buffer;
  uint8_t buf_len;
  uint8_t buf_used;
  int sent_bytes;
};

size_t min(size_t a, size_t b) { return (a < b) ? a : b; }

size_t vd_write(FILE *f, const unsigned char *s, size_t l) {
  struct vd_cookie *c = f->cookie;

  int clear_buffer = 0;
  size_t b_copy = min(l, c->buf_len - (c->buf_used));
  for (int i = 0; i < b_copy; i++) {
    c->buffer[c->buf_used + i] = s[i];
    if (s[i] == '\n')
      clear_buffer = 1;
  }
  c->buf_used += b_copy;

  if (clear_buffer) {
    int rc = write(c->fd, c->buffer, c->buf_used);
    c->buf_used = 0;
    if (-1 == rc) {
      return (size_t)-1;
    }
    c->sent_bytes += rc;
  }
  return l;
}

int vdprintf(int fd, const char *format, va_list ap) {
  FILE f = {
      .write = write_fd,
      .fd = fd,
  };
  return vfprintf(&f, format, ap);
  //    return -1;
  /*
char buffer[32];
struct vd_cookie c = {.fd = fd,
                  .buffer = buffer,
                  .buf_len = 32,
                  .buf_used = 0,
                  .sent_bytes = 0};
FILE f = {
.write = vd_write,
.cookie = &c,
};

// If an output error was encountered, these functions shall return a
// negative value and set errno to indicate the error.
if (-1 == vfprintf(&f, format, ap))
return -1;

// Upon successful completion, the dprintf(),
// fprintf(), and printf() functions shall return the number of bytes
// transmitted.

if(0 == c.buf_used)
return c.sent_bytes;

// First the current buffer needs to be cleared
int rc = write(fd, buffer, c.buf_used);
if (-1 == rc) {
return -1;
}
c.sent_bytes += rc;
return c.sent_bytes;*/
}
