#include <libc_test.h>
#include <math.h>

double fmin(double x, double y) {
  if (x > y) {
    return y;
  }
  return x;
}

LIBC_TEST(fmin, {
  TEST_ASSERT(0 == fmin(1, 0));
  TEST_ASSERT(0 == fmin(0, 1));
})
