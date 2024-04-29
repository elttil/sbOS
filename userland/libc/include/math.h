#define max(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

#if 100*__GNUC__+__GNUC_MINOR__ >= 303
#define NAN       __builtin_nanf("")
#define INFINITY  __builtin_inff()
#else
#define NAN       (0.0f/0.0f)
#define INFINITY  1e40f
#endif

#define HUGE_VALF INFINITY
#define HUGE_VAL  ((double)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

double ldexp(double x, int exp);
