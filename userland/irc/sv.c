#include "sv.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *SV_TO_C(struct sv s) {
  char *c_string = malloc(s.length + 1);
  memcpy(c_string, s.s, s.length);
  c_string[s.length] = '\0';
  return c_string;
}

struct sv sv_split_space(const struct sv input, struct sv *rest) {
  struct sv r = {
      .s = input.s,
  };
  for (size_t i = 0; i < input.length; i++) {
    if (isspace(input.s[i])) {
      r.length = i;
      if (rest) {
        rest->s += i + 1;
        rest->length -= (i + 1);
      }
      return r;
    }
  }

  if (rest) {
    rest->s = NULL;
    rest->length = 0;
  }
  return input;
}

struct sv sv_split_delim(const struct sv input, struct sv *rest, char delim) {
  struct sv r = {
      .s = input.s,
  };
  for (size_t i = 0; i < input.length; i++) {
    if (delim == input.s[i]) {
      r.length = i;
      if (rest) {
        rest->s += i + 1;
        rest->length -= (i + 1);
      }
      return r;
    }
  }

  if (rest) {
    rest->s = NULL;
    rest->length = 0;
  }
  return input;
}

int sv_isempty(struct sv s) {
  return (0 == s.length);
}

char sv_peek(struct sv s) {
  if (0 == s.length) {
    return '\0';
  }
  return s.s[0];
}

int sv_eq(struct sv a, struct sv b) {
  if (a.length != b.length) {
    return 0;
  }
  for (size_t i = 0; i < a.length; i++) {
    if (a.s[i] != b.s[i]) {
      return 0;
    }
  }
  return 1;
}

struct sv sv_trim_left(struct sv s, size_t n) {
  if (s.length < n) {
    s.s += s.length;
    s.length = 0;
    return s;
  }
  s.s += n;
  s.length -= n;
  return s;
}

struct sv sv_clone(struct sv s) {
  struct sv new_sv;
  new_sv.length = s.length;
  char *new_string = malloc(s.length);
  memcpy(new_string, s.s, s.length);
  new_sv.s = new_string;
  return new_sv;
}

struct sv sv_clone_from_c(const char *s) {
  return sv_clone(C_TO_SV(s));
}
