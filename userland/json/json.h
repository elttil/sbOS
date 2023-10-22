#ifndef JSON_H
#define JSON_H
#include "hashmap/hashmap.h"
#include <stddef.h>
#include <stdint.h>

typedef enum JSON_TYPE {
  NONE,
  STRING,
  NUMBER,
  OBJECT,
  ARRAY, // FIXME
  BOOL,
  JSON_NULL,
} JSON_TYPE;

typedef struct JSON_ENTRY {
  char *name;
  JSON_TYPE type;
  void *value;
} JSON_ENTRY;

typedef struct JSON_OBJECT {
  uint64_t size;
  JSON_ENTRY *entries;
  HashMap *hash_indexes;
} JSON_OBJECT;

typedef struct JSON_CTX {
  JSON_ENTRY global_object;
} JSON_CTX;

void JSON_Init(JSON_CTX *ctx);
void JSON_Parse(JSON_CTX *ctx, const char *json);
void JSON_extract_fd(JSON_CTX *ctx, int whitespace, int fd);
uint64_t JSON_extract_length(JSON_CTX *ctx, int whitespace);
JSON_ENTRY *JSON_at(JSON_ENTRY *entry, uint64_t i);
JSON_ENTRY *JSON_search_name(JSON_ENTRY *entry, char *name);
#endif
