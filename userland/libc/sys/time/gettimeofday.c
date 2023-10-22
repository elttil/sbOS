#include <sys/time.h>
#include <time.h>

static int return_tod =0;
int gettimeofday(struct timeval *tp, void *tzp) {return return_tod++;}
