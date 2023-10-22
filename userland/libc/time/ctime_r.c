#include <string.h>
#include <time.h>

// TODO: Implement this

// Time, formatting and parsing are some of the most annoying parts of
// programming. Lets just hope this function is not important
char *ctime_r(const time_t *clock, char *buf) {
  (void)clock;
  size_t l = strlen(buf);
  memset(buf, '0', l);
  return buf;
}
