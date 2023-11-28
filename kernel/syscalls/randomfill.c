#include <random.h>
#include <typedefs.h>

// This syscall will never fail. At worst a page fault will occur but if
// the syscall returns the buffer will have been filled with random
// data.
void syscall_randomfill(void *buffer, u32 size) {
  get_random(buffer, size);
}
