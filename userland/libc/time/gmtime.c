#include <time.h>

struct tm gmtime_r = {
    .tm_sec = 0,
    .tm_min = 0,
    .tm_hour = 0,
    .tm_mday = 0,
    .tm_mon = 0,
    .tm_year = 0,
    .tm_wday = 0,
    .tm_yday = 0,
    .tm_isdst = 0,
    .__tm_gmtoff = 0,
    .__tm_zone = 0,
};

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/gmtime.html
struct tm *gmtime(const time_t *timer) {
	(void)timer;
  // TODO: Implement
  return &gmtime_r;
}
