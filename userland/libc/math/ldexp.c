#include <float.h>
#include <math.h>

double ldexp(double x, int exp) {
  double sign = (x < 0.0) ? (-1) : (1);

  if (exp > 29) {
    return sign * HUGE_VAL;
  }
  double two_exp = 2 << exp;

  if (x > DBL_MAX / two_exp) {
    return sign * HUGE_VAL;
  }
  return x * two_exp;
}
