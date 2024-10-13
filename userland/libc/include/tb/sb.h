#ifndef SB_H
#define SB_H
#include "sv.h"
#include <stddef.h>
#include <stdint.h>

struct sb {
  char *string;
  size_t length;
  size_t capacity;
  uint8_t prebuffer;
};

struct sv;

void sb_init(struct sb *ctx);
int sb_init_capacity(struct sb *ctx, size_t starting_capacity);
void sb_init_buffer(struct sb *ctx, char *buffer, size_t size);
void sb_free(struct sb *ctx);
void sb_reset(struct sb *ctx);
int sb_isempty(const struct sb *ctx);
int sb_append_char(struct sb *ctx, char c);
int sb_delete_right(struct sb *ctx, size_t n);
int sb_append(struct sb *ctx, const char *s);
int sb_append_buffer(struct sb *ctx, const char *buffer,
                      size_t length);
int sb_append_sv(struct sb *ctx, struct sv sv);
int sb_prepend_sv(struct sb *ctx, struct sv sv);
int sb_prepend_buffer(struct sb *ctx, const char *buffer, size_t length);
#endif
