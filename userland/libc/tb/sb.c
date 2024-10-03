#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <tb/sb.h>

void sb_init(struct sb *ctx, size_t starting_capacity) {
  ctx->string = malloc(starting_capacity);
  ctx->length = 0;
  ctx->capacity = starting_capacity;
}

void sb_free(struct sb *ctx) {
  ctx->length = 0;
  ctx->capacity = 0;
  free(ctx->string);
  ctx->string = NULL;
}

void sb_reset(struct sb *ctx) {
  ctx->length = 0;
}

int sb_isempty(const struct sb *ctx) {
  return (0 == ctx->length);
}

void sb_append_char(struct sb *ctx, char c) {
  if (1 > ctx->capacity - ctx->length) {
    ctx->capacity += 32;
    ctx->string = realloc(ctx->string, ctx->capacity);
  }
  memcpy(ctx->string + ctx->length, &c, 1);
  ctx->length++;
}

int sb_delete_right(struct sb *ctx, size_t n) {
  n = min(n, ctx->length);
  ctx->length -= n;
  return n;
}

void sb_append(struct sb *ctx, const char *s) {
  size_t l = strlen(s);
  if (l > ctx->capacity - ctx->length) {
    ctx->capacity += l;
    ctx->string = realloc(ctx->string, ctx->capacity);
  }
  memcpy(ctx->string + ctx->length, s, l);
  ctx->length += l;
}

void sb_prepend_sv(struct sb *ctx, struct sv sv) {
  if (sv.length > ctx->capacity - ctx->length) {
    ctx->capacity += sv.length;
    ctx->string = realloc(ctx->string, ctx->capacity);
  }
  memmove(ctx->string + sv.length, ctx->string, ctx->length);
  memcpy(ctx->string, sv.s, sv.length);
  ctx->length += sv.length;
}

void sb_append_buffer(struct sb *ctx, const char *buffer, size_t length) {
  if (length > ctx->capacity - ctx->length) {
    ctx->capacity += length;
    ctx->string = realloc(ctx->string, ctx->capacity);
  }
  memcpy(ctx->string + ctx->length, buffer, length);
  ctx->length += length;
}

void sb_append_sv(struct sb *ctx, struct sv sv) {
  sb_append_buffer(ctx, sv.s, sv.length);
}
