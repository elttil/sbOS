//
// Copyright (C) 2022 by Anton Kling <anton@kling.gg>
//
// SPDX-License-Identifier: BSD-2-Clause
//
#include "hashmap/hashmap.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  size_t size;
  JSON_ENTRY *entries;
  HashMap *hash_indexes;
} JSON_OBJECT;

typedef struct JSON_CTX {
  JSON_ENTRY global_object;
} JSON_CTX;

void JSON_Init(JSON_CTX *ctx) {
  ctx->global_object.name = NULL;
  ctx->global_object.type = OBJECT;
  ctx->global_object.value = malloc(sizeof(JSON_OBJECT));
  JSON_OBJECT *obj = ctx->global_object.value;
  obj->size = 0;
  obj->entries = NULL;
}

const char *skip_whitespace(const char *str) {
  for (; *str; str++)
    switch (*str) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      break;
    default:
      goto _exit;
    }
_exit:
  return str;
}

char to_lower(char c) {
  if (c <= 'Z')
    return c | 32;
  return c;
}

int low_strequ(const char *s1, const char *s2, size_t l) {
  for (; 0 < l; l--, s2++, s1++) {
    if (to_lower(*s2) != to_lower(*s1))
      return 0;
  }
  return 1;
}

const char *find_name(const char *json, char *name_buffer, size_t buffer_len,
                      size_t *name_length, int *rc) {
  *rc = 0;
  json = skip_whitespace(json);
  if ('"' != *json)
    return json;

  // This *can* be the name
  json++;
  const char *tmp = json;
  for (; *tmp && '"' != *tmp; tmp++)
    ;
  if (!(*tmp && ':' == *(tmp + 1))) {
    // Invalid name
    // FIXME: Do something better than this.
    assert(0);
  }
  // The name was found.
  *rc = 1;
  size_t str_len;
  if (tmp == json)
    str_len = 0;
  else
    str_len = (tmp - json - 1);
  if (str_len + 1 > buffer_len) {
    // JSON string is too long for the buffer
    // FIXME: Do something better than this.
    assert(0);
  }
  memcpy(name_buffer, json, str_len + 1);
  name_buffer[str_len + 1] = '\0';
  *name_length = str_len;
  return tmp + 2;
}

const char *find_entry(const char *json, char *name_buffer, size_t buffer_len,
                       size_t *name_length, JSON_TYPE *entry_type, int *rc) {
  *rc = 0;
  json = skip_whitespace(json);
  json = find_name(json, name_buffer, 4096, name_length, rc);
  json = skip_whitespace(json);
  switch (*json) {
  case '{':
    *entry_type = OBJECT;
    break;
  case '"':
    *entry_type = STRING;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    *entry_type = NUMBER;
    break;
  case '[':
    *entry_type = ARRAY;
    break;
  case 'T':
  case 'F':
  case 't':
  case 'f':
    *entry_type = BOOL;
    break;
  case 'N':
  case 'n':
    *entry_type = JSON_NULL;
    break;
  case '}':
    *rc = 2;
    break;
  default:
    *entry_type = NONE;
    break;
  }
  return json;
}

const char *extract_int(const char *json, uint32_t **value, int *rc) {
  *value = malloc(sizeof(uint32_t));
  *rc = 0;
  json = skip_whitespace(json);
  // FIXME: Do some checking to determine that this is a int.
  **value = 0;
  for (; *json && ',' != *json && '}' != *json;) {
    **value *= 10;
    **value += (uint32_t)(*json - '0');
    json++;
    json = skip_whitespace(json);
  }
  if ('\0' != *json)
    *rc = 1;
  return json;
}

const char *extract_string(const char *json, char **value, int *rc) {
  *value = malloc(4096);
  *rc = 0;
  int escape = 0;
  json++;
  char *tmp = *value;
  for (; *json; json++) {
    if ('"' == *json && !escape)
      break;
    escape = 0;

    if ('\\' == *json && !escape) {
      escape = 1;
      continue;
    }
    *tmp = *json;
    tmp++;
  }
  *tmp = '\0';
  if ('\0' != *json)
    *rc = 1;
  return json;
}

const char *extract_bool(const char *json, uint8_t **value, int *rc) {
  *value = malloc(sizeof(uint8_t));
  *rc = 0;
  if (low_strequ(json, "true", 3)) {
    **value = 1;
    json += 4;
    *rc = 1;
    return json;
  } else if (low_strequ(json, "false", 4)) {
    **value = 0;
    json += 5;
    *rc = 1;
    return json;
  }
  return json;
}

const char *extract_null(const char *json, uint8_t **value, int *rc) {
  *value = malloc(sizeof(uint8_t));
  *rc = 0;

  if (low_strequ(json, "null", 3)) {
    **value = 0;
    json += 4;
    *rc = 1;
    return json;
  }
  return json;
}

void hashmap_free_value(char *key, void *value) {
  (void)key;
  free(value);
}

JSON_ENTRY *JSON_at(JSON_ENTRY *entry, size_t i);

void JSON_Parse(JSON_CTX *ctx, const char *json) {
  int rc;
  char name_buffer[4096];
  size_t name_length;
  JSON_TYPE entry_type;
  JSON_OBJECT *object_stack[64];
  memset(object_stack, 0, sizeof(JSON_OBJECT *[64]));
  size_t parent_num = 0;
  object_stack[parent_num] = ctx->global_object.value;
  object_stack[parent_num]->size = 0;

  for (;; json++) {
    json = find_entry(json, name_buffer, 4096, &name_length, &entry_type, &rc);
    if ('\0' == *json)
      return;

    if (2 == rc) {
      if (0 == parent_num)
        return;
      parent_num--;
      json++;
      json = skip_whitespace(json);
      if (',' == *json)
        json++;
      continue;
    }

    if (NULL == object_stack[parent_num]->entries) {
      object_stack[parent_num]->entries = malloc(sizeof(JSON_ENTRY[30]));
      object_stack[parent_num]->size = 0;
      object_stack[parent_num]->hash_indexes = hashmap_create(30);
    }

    size_t entry_num = object_stack[parent_num]->size;
    object_stack[parent_num]->size++;

    if (NULL == object_stack[parent_num]->entries) {
      void *rc = object_stack[parent_num]->entries =
          malloc(sizeof(JSON_ENTRY[30]));
      object_stack[parent_num]->entries = rc;
      object_stack[parent_num]->size = 0;
      object_stack[parent_num]->hash_indexes = hashmap_create(30);
    }
    JSON_ENTRY *entry = &object_stack[parent_num]->entries[entry_num];
    if (1 == rc) {
      name_length++;
      entry->name = malloc(name_length + 2);
      memcpy(entry->name, name_buffer, name_length);
      entry->name[name_length] = '\0';
      size_t *b = malloc(sizeof(size_t));
      *b = entry_num;
      hashmap_add_entry(object_stack[parent_num]->hash_indexes, entry->name, b,
                        hashmap_free_value, 1);
    } else {
      entry->name = NULL;
    }
    entry->type = entry_type;
    if (OBJECT == entry_type) {
      entry->value = malloc(sizeof(JSON_OBJECT));
      parent_num++;
      object_stack[parent_num] = entry->value;
      object_stack[parent_num]->entries = NULL;
      continue;
    } else if (NUMBER == entry_type) {
      json = extract_int(json, (uint32_t **)&entry->value, &rc);
    } else if (STRING == entry_type) {
      json = extract_string(json, (char **)&entry->value, &rc);
    } else if (BOOL == entry_type) {
      json = extract_bool(json, (uint8_t **)&entry->value, &rc);
    } else if (JSON_NULL == entry_type) {
      json = extract_null(json, (uint8_t **)&entry->value, &rc);
    }
    json++;
  }
}

JSON_ENTRY *JSON_at(JSON_ENTRY *entry, size_t i) {
  if (OBJECT != entry->type)
    return NULL;
  return &((JSON_OBJECT *)(entry->value))->entries[i];
}

JSON_ENTRY *JSON_search_name(JSON_ENTRY *entry, char *name) {
  if (OBJECT != entry->type)
    return NULL;
  size_t *i =
      hashmap_get_entry(((JSON_OBJECT *)(entry->value))->hash_indexes, name);
  if (!i)
    return NULL;
  return JSON_at(entry, *i);
}

void write_entry_content(int fd, size_t indent, JSON_ENTRY *entry) {
#define PLACE_TABS                                                             \
  {                                                                            \
    for (size_t i = 0; i < indent; i++)                                        \
      dprintf(fd, "\t");                                                       \
  }
  PLACE_TABS
  if (entry->name)
    dprintf(fd, "\"%s\": ", entry->name);
  if (OBJECT == entry->type) {
    JSON_OBJECT *object = entry->value;
    dprintf(fd, "{\n");
    for (size_t i = 0; i < object->size; i++)
      write_entry_content(fd, indent + 1, &object->entries[i]);
    PLACE_TABS
    dprintf(fd, "},\n");
  } else if (NUMBER == entry->type)
    dprintf(fd, "%d,\n", *(uint32_t *)entry->value);
  else if (STRING == entry->type)
    dprintf(fd, "\"%s\",\n", (char *)entry->value);
  else if (BOOL == entry->type)
    dprintf(fd, "%s,\n", ((*(uint8_t *)entry->value) ? "true" : "false"));
  else if (JSON_NULL == entry->type)
    dprintf(fd, "null,\n");
}

void JSON_extract_fd(JSON_CTX *ctx, int fd) {
  JSON_ENTRY *entry = &ctx->global_object;
  write_entry_content(fd, 0, &((JSON_OBJECT *)entry->value)->entries[0]);
}
