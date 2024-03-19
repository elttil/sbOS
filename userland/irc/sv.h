#ifndef SV_H
#define SV_H
#include "sb.h"
#include <stddef.h>

#define SB_TO_SV(_sb)                                                          \
  (struct sv) {                                                                \
    .s = (_sb).string, .length = (_sb).length                                  \
  }

#define C_TO_SV(_c_string)                                                     \
  ((struct sv){.length = strlen(_c_string), .s = (_c_string)})

struct sv {
  const char *s;
  size_t length;
};

char *SV_TO_C(struct sv s);
struct sv sv_split_delim(const struct sv input, struct sv *rest, char delim);
struct sv sv_split_space(const struct sv input, struct sv *rest);
int sv_isempty(struct sv s);
char sv_peek(struct sv s);
int sv_eq(struct sv a, struct sv b);
struct sv sv_trim_left(struct sv s, size_t n);
struct sv sv_clone(struct sv s);
struct sv sv_clone_from_c(const char *s);
#endif
