#include <assert.h>
#include <kmalloc.h>
#include <mmu.h>
#include <stddef.h>
#include <string.h>

char *copy_and_allocate_string(const char *s) {
  size_t l = strlen(s) + 1;
  char *r = kmalloc(l);
  if (!r) {
    return NULL;
  }
  memcpy(r, s, l);
  return r;
}

char *copy_and_allocate_user_string(const char *s) {
  size_t len;
  if (!mmu_is_valid_user_c_string(s, &len)) {
    return NULL;
  }
  size_t real_len = strlen(s);
  assert(real_len == len);
  len = real_len;
  char *r = kmalloc(len + 1);
  if (!r) {
    return NULL;
  }
  memcpy(r, s, len + 1);
  return r;
}
