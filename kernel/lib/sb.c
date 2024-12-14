#include <kmalloc.h>
#include <lib/sb.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 256

void sb_init(struct sb *ctx) {
  (void)sb_init_capacity(ctx, DEFAULT_CAPACITY);
}

int sb_init_capacity(struct sb *ctx, size_t starting_capacity) {
  ctx->to_ignore = 0;
  ctx->length = 0;
  ctx->prebuffer = 0;
  ctx->string = kmalloc(starting_capacity);
  if (NULL == ctx->string) {
    ctx->capacity = 0;
    return 0;
  }
  ctx->capacity = starting_capacity;
  return 1;
}

void sb_init_buffer(struct sb *ctx, char *buffer, size_t size) {
  ctx->string = buffer;
  ctx->capacity = size;
  ctx->length = 0;
  ctx->prebuffer = 1;
  ctx->to_ignore = 0;
}

void sb_free(struct sb *ctx) {
  if (ctx->prebuffer) {
    ctx->length = 0;
    return;
  }
  ctx->length = 0;
  ctx->capacity = 0;
  kfree(ctx->string);
  ctx->string = NULL;
}

void sb_set_ignore(struct sb *ctx, size_t n) {
  ctx->to_ignore = n;
}

void sb_reset(struct sb *ctx) {
  ctx->length = 0;
  ctx->to_ignore = 0;
}

int sb_isempty(const struct sb *ctx) {
  return (0 == ctx->length);
}

int sb_increase_buffer(struct sb *ctx, size_t min) {
  if (ctx->prebuffer) {
    return 0;
  }
  size_t new_capacity = ctx->capacity + max(32, min);
  char *new_allocation = krealloc(ctx->string, new_capacity);
  if (!new_allocation) {
    return 0;
  }

  ctx->string = new_allocation;
  ctx->capacity = new_capacity;
  return 1;
}

int sb_append_char(struct sb *ctx, char c) {
  if (ctx->to_ignore > 0) {
    ctx->to_ignore--;
    return 1;
  }
  if (1 > ctx->capacity - ctx->length) {
    if (!sb_increase_buffer(ctx, 1)) {
      return 0;
    }
  }
  memcpy(ctx->string + ctx->length, &c, 1);
  ctx->length++;
  return 1;
}

int sb_delete_right(struct sb *ctx, size_t n) {
  n = min(n, ctx->length);
  ctx->length -= n;
  return n;
}

int sb_append(struct sb *ctx, const char *s) {
  size_t l = strlen(s);
  if (ctx->to_ignore >= l) {
    ctx->to_ignore -= l;
    return 1;
  }
  l -= ctx->to_ignore;
  s += ctx->to_ignore;
  ctx->to_ignore = 0;
  if (l > ctx->capacity - ctx->length) {
    if (!sb_increase_buffer(ctx, l)) {
      return 0;
    }
  }
  memcpy(ctx->string + ctx->length, s, l);
  ctx->length += l;
  return 1;
}

int sb_prepend_sv(struct sb *ctx, struct sv sv) {
  return sb_prepend_buffer(ctx, sv.s, sv.length);
}

int sb_prepend_buffer(struct sb *ctx, const char *buffer, size_t length) {
  if (ctx->to_ignore >= length) {
    ctx->to_ignore -= length;
    return 1;
  }
  length -= ctx->to_ignore;
  buffer += ctx->to_ignore;
  ctx->to_ignore = 0;
  if (length > ctx->capacity - ctx->length) {
    if (!sb_increase_buffer(ctx, length)) {
      return 0;
    }
  }
  memmove(ctx->string + length, ctx->string, ctx->length);
  memcpy(ctx->string, buffer, length);
  ctx->length += length;
  return 1;
}

int sb_append_buffer(struct sb *ctx, const char *buffer, size_t length) {
  if (ctx->to_ignore >= length) {
    ctx->to_ignore -= length;
    return 1;
  }
  length -= ctx->to_ignore;
  buffer += ctx->to_ignore;
  ctx->to_ignore = 0;
  if (length > ctx->capacity - ctx->length) {
    if (!sb_increase_buffer(ctx, length)) {
      return 0;
    }
  }
  memcpy(ctx->string + ctx->length, buffer, length);
  ctx->length += length;
  return 1;
}

int sb_append_sv(struct sb *ctx, struct sv sv) {
  return sb_append_buffer(ctx, sv.s, sv.length);
}
