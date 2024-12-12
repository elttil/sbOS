#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

int ctoi(char c) {
  return c - '0';
}

double strtod(const char *restrict nptr, char **restrict endptr) {
  double r = 0;
  // An initial, possibly empty, sequence of white-space characters (as
  // specified by isspace())
  for (; isspace(*nptr); nptr++)
    ;

  // A subject sequence interpreted as a floating-point constant or representing
  // infinity or NaN

  {
    // The expected form of the subject sequence is an optional '+' or '-' sign
    int sign = 0;
    int exp_sign = 0;
    if ('+' == *nptr) {
      sign = 0;
      nptr++;
    } else if ('-' == *nptr) {
      sign = 1;
      nptr++;
    }

    // A non-empty sequence of decimal digits optionally containing a radix
    // character
    double exp = 0;
    for (; isdigit(*nptr); nptr++) {
      r *= 10;
      r += ctoi(*nptr);
    }
    if ('.' == *nptr) {
      double div = 10;
      for (; isdigit(*nptr); nptr++) {
        r += ctoi(*nptr) / div;
        div *= 10;
      }
    }
    r *= (sign) ? (-1) : (1);

    // then an optional exponent part consisting of the character 'e' or
    // the character 'E'
    if ('e' == tolower(*nptr)) {
      // optionally followed by a '+' or '-' character
      if ('+' == *nptr) {
        exp_sign = 0;
        nptr++;
      } else if ('-' == *nptr) {
        exp_sign = 1;
        nptr++;
      }
      // and then followed by one or more decimal digits
      for (; isdigit(*nptr); nptr++) {
        exp *= 10;
        exp += ctoi(*nptr);
      }
      exp *= (exp_sign) ? (-1) : (1);
    }
    assert(0 == exp); // TODO
  }

  // A final string of one or more unrecognized characters, including the
  // terminating NUL character of the input string
  ;
  return r;
}
