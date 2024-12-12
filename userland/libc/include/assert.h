#ifndef ASSERT_H
#define ASSERT_H

#define assert(expr)                                                           \
  {                                                                            \
    if (!(expr))                                                               \
      aFailed(__FILE__, __LINE__);                                             \
  }
void aFailed(char *f, int l);
#endif
