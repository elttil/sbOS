#include <log.h>
#include <stdio.h>

#define assert(expr)                                                           \
  {                                                                            \
    if (!(expr))                                                               \
      aFailed(__FILE__, __LINE__);                                             \
  }

#define ASSERT_BUT_FIXME_PROPOGATE(expr)                                       \
  {                                                                            \
    if (!(expr))                                                               \
      kprintf("Performing assert that should have been a propogated error.");  \
    assert(expr);                                                              \
  }

void aFailed(char *f, int l);
#define ASSERT_NOT_REACHED                                                     \
  { assert(0) }
