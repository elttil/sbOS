//
// Copyright (C) 2022 by Anton Kling <anton@kling.gg>
//
// SPDX-License-Identifier: BSD-2-Clause
//
#ifndef HASHMAP_H
#define HASHMAP_H
#include <stddef.h>
#include <typedefs.h>

typedef struct LinkedList {
  const char *key;
  int key_allocated;
  void *value;
  void (*upon_deletion)(const char *, void *);
  struct LinkedList *next;
} LinkedList;

typedef struct HashMap {
  LinkedList **entries;
  size_t size;
  size_t num_entries;
  u32 (*hash_function)(const u8 *data, size_t len);
} HashMap;

HashMap *hashmap_create(size_t size);
void hashmap_free(HashMap *m);

// hashmap_add_entry()
// -------------------
// This function adds a entry to the hashmap. The "key" passed to the
// hashmap will be allocated by default unless (do_not_allocate_key) is
// set to 1.
int hashmap_add_entry(HashMap *m, const char *key, void *value,
                      void (*upon_deletion)(const char *, void *),
                      int do_not_allocate_key);
void *hashmap_get_entry(HashMap *m, const char *key);
int hashmap_delete_entry(HashMap *m, const char *key);
#endif
