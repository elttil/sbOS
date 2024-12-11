#include <ctype.h>
#include <kmalloc.h>
#include <lib/sv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct sv sv_init(const char *s, size_t length) {
  return (struct sv){.s = s, .length = length};
}

char *SV_TO_C(struct sv s) {
  char *c_string = kmalloc(s.length + 1);
  memcpy(c_string, s.s, s.length);
  c_string[s.length] = '\0';
  return c_string;
}

size_t sv_to_cstring_buffer(struct sv s, char *buffer, size_t length) {
  if (0 == length || length - 1 < s.length) {
    return s.length;
  }
  memcpy(buffer, s.s, s.length);
  buffer[s.length] = '\0';
  return s.length;
}

struct sv sv_next(struct sv s) {
  if (0 == s.length) {
    return s;
  }
  s.length--;
  s.s++;
  return s;
}

struct sv sv_skip_chars(const struct sv input, const char *chars) {
  struct sv r = input;
  for (; r.length > 0;) {
    int found = 0;
    const char *p = chars;
    for (; *p; p++) {
      if (*p == r.s[0]) {
        r.s++;
        r.length--;
        found = 1;
        break;
      }
    }
    if (!found) {
      break;
    }
  }
  return r;
}

uint64_t sv_parse_unsigned_number(struct sv input, struct sv *rest,
                                  int *got_num) {
  if (got_num) {
    *got_num = 0;
  }
  uint64_t r = 0;
  size_t i = 0;
  for (; i < input.length; i++) {
    if (!isdigit(input.s[i])) {
      break;
    }
    r *= 10;
    r += input.s[i] - '0';
  }
  if (i > 0) {
    if (got_num) {
      *got_num = 1;
    }
  }
  input.length -= i;
  input.s += i;
  if (rest) {
    *rest = input;
  }
  return r;
}

struct sv sv_split_function(const struct sv input, struct sv *rest,
                            int (*function)(int)) {
  struct sv r = {
      .s = input.s,
  };
  for (size_t i = 0; i < input.length; i++) {
    if (function(input.s[i])) {
      r.length = i;
      if (rest) {
        rest->s += i;
        rest->length -= i;
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

struct sv sv_split_space(const struct sv input, struct sv *rest) {
  return sv_split_function(input, rest, isspace);
}

struct sv sv_end_split_delim(const struct sv input, struct sv *rest,
                             char delim) {
  for (size_t i = input.length - 1; i > 0; i--) {
    if (delim == input.s[i]) {
      struct sv r = {
          .s = (input.s + i),
          .length = input.length - i,
      };
      if (rest) {
        rest->s = input.s;
        rest->length = i;
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

int sv_partial_eq(struct sv a, struct sv b) {
  if (a.length < b.length) {
    return 0;
  }
  for (size_t i = 0; i < b.length; i++) {
    if (a.s[i] != b.s[i]) {
      return 0;
    }
  }
  return 1;
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

struct sv sv_take(struct sv s, struct sv *rest, size_t n) {
  if (s.length < n) {
    if (rest) {
      rest->length = 0;
    }
    return s;
  }
  s.length = n;
  if (rest) {
    rest->length -= n;
    rest->s += n;
  }
  return s;
}

struct sv sv_take_end(struct sv s, struct sv *rest, size_t n) {
  if (s.length < n) {
    if (rest) {
      rest->length = 0;
    }
    return s;
  }
  if (rest) {
    rest->length = s.length - n;
  }
  s.s += (s.length - n);
  s.length = n;
  return s;
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
  char *new_string = kmalloc(s.length);
  memcpy(new_string, s.s, s.length);
  new_sv.s = new_string;
  return new_sv;
}

int sv_try_eat(struct sv input, struct sv *rest, struct sv b) {
  if (input.length < b.length) {
    return 0;
  }
  for (size_t i = 0; i < b.length; i++) {
    if (input.s[i] != b.s[i]) {
      return 0;
    }
  }
  input.s += b.length;
  input.length -= b.length;
  if (rest) {
    *rest = input;
  }
  return 1;
}

char *sv_copy_to_c(struct sv s, char *out, size_t buffer_length) {
  int copy_len = min(s.length + 1, buffer_length);
  if (0 == copy_len) {
    return NULL;
  }
  if (!out) {
    out = kmalloc(copy_len);
  }
  memcpy(out, s.s, copy_len - 1);
  out[copy_len - 1] = '\0';
  return out;
}

struct sv sv_clone_from_c(const char *s) {
  return sv_clone(C_TO_SV(s));
}
