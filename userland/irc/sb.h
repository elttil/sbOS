#ifndef SB_H
#define SB_H
#include "sv.h"
#include <stddef.h>

struct sb {
  char *string;
  size_t length;
  size_t capacity;
};

struct sv;

void sb_init(struct sb *ctx);
void sb_free(struct sb *ctx);
void sb_reset(struct sb *ctx);
int sb_isempty(const struct sb *ctx);
void sb_append_char(struct sb *ctx, char c);
int sb_delete_right(struct sb *ctx, int n);
void sb_append(struct sb *ctx, const char *s);
void sb_append_sv(struct sb *ctx, struct sv sv);
#endif
