//
// Copyright (C) 2022 by Anton Kling <anton@kling.gg>
//
// SPDX-License-Identifier: BSD-2-Clause
//
/*
 * HashMap
 * -------
 * This hashmap works by creating a array of linked lists and associating
 * the "key" with a specific entry in the array. This is done through a
 * hash. Once the linked list is found it goes through it until it finds
 * a entry that has the "key" provided.
 *
 * Changing the hashing function
 * -----------------------------
 * The default hashing function in this library is a custom made
 * one that can be found in hash.c. But it should be possible to use any
 * other hashing algorithm should you want to as long as it uses these
 * types:
 * uint32_t hash_function(uint8_t*, size_t)
 *
 * The hashing algorithm can be changed via changing the "hash_function"
 * pointer in the HashMap structure.
 *
 * Important to note that the hashing function shall not be changed
 * after having added entries to the hashmap as the key to linked list
 * pairing will change.
 */
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kmalloc.h>

#define CONSTANT 0x031b5515

uint32_t mix(uint32_t x) {
  x ^= CONSTANT;
  x ^= (x << 13);
  x ^= (x >> 7);
  x ^= (x << 17);
  return x;
}

uint32_t hash(const uint8_t *data, size_t len) {
  uint32_t hash = 0;
  for (; len;) {
    uint32_t value = 0;
    uint8_t read_len = (sizeof(uint32_t) < len) ? sizeof(uint32_t) : len;
    memcpy(&value, data, read_len);
    hash ^= mix(value);
    data += read_len;
    len -= read_len;
  }
  return hash;
}

char *copy_c_string(const char *str) {
  char *ret_string;
  size_t len = strlen(str);
  ret_string = kmalloc(len + 1);
  if (!ret_string)
    return NULL;
  memcpy(ret_string, str, len);
  ret_string[len] = '\0';
  return ret_string;
}

uint32_t limit_hash(HashMap *m, uint32_t hash) { return hash % m->size; }

void free_linkedlist_entry(LinkedList *entry) {
  if (entry->key_allocated) {
    // If the key is allocated by the hashmap library then it owns the
    // key and can safley discard the const qualifier and override its
    // contents
    kfree((char *)entry->key);
  }
  kfree(entry);
}

LinkedList *get_linkedlist_entry(LinkedList *list, const char *key,
                                 LinkedList **prev) {
  if (prev)
    *prev = NULL;
  for (; list; list = list->next) {
    const char *str1 = key;
    const char *str2 = list->key;
    for (; *str1 && *str2; str1++, str2++)
      if (*str1 != *str2)
        break;
    if (*str1 == *str2)
      return list;
    if (prev)
      prev = &list;
  }
  return NULL;
}

void *get_linkedlist_value(LinkedList *list, const char *key) {
  LinkedList *entry = get_linkedlist_entry(list, key, NULL);
  if (!entry)
    return NULL;
  return entry->value;
}

uint32_t find_index(HashMap *m, const char *key) {
  return limit_hash(m, m->hash_function((uint8_t *)key, strlen(key)));
}

int hashmap_add_entry(HashMap *m, const char *key, void *value,
                      void (*upon_deletion)(const char *, void *),
                      int do_not_allocate_key) {
  // Create the entry
  LinkedList *entry = kmalloc(sizeof(LinkedList));
  if (!entry)
    return 0;

  entry->key_allocated = !do_not_allocate_key;
  if (do_not_allocate_key) {
    entry->key = key;
  } else {
    if (!(entry->key = copy_c_string(key)))
      return 0;
  }
  entry->value = value;
  entry->next = NULL;
  entry->upon_deletion = upon_deletion;

  // Add the new entry to the list.
  uint32_t index = find_index(m, key);
  LinkedList **list_pointer = &m->entries[index];
  for (; *list_pointer;)
    list_pointer = &(*list_pointer)->next;

  *list_pointer = entry;
  m->num_entries++;
  return 1;
}

void *hashmap_get_entry(HashMap *m, const char *key) {
  uint32_t index = find_index(m, key);
  if (!m->entries[index])
    return NULL;
  return get_linkedlist_value(m->entries[index], key);
}

int hashmap_delete_entry(HashMap *m, const char *key) {
  LinkedList *list = m->entries[find_index(m, key)];
  if (!list)
    return 0;
  LinkedList **prev = NULL;
  LinkedList *entry = get_linkedlist_entry(list, key, prev);
  if (!entry)
    return 0;
  if (!prev)
    prev = &m->entries[find_index(m, key)];

  if (entry->upon_deletion)
    entry->upon_deletion(entry->key, entry->value);

  LinkedList *next = entry->next;
  free_linkedlist_entry(entry);
  if (*prev != entry)
    (*prev)->next = next;
  else
    *prev = NULL;

  // Redo the delete process incase there are multiple
  // entires that have the same key.
  hashmap_delete_entry(m, key);
  m->num_entries--;
  return 1;
}

void hashmap_free(HashMap *m) {
  for (int i = 0; i < m->size; i++) {
    if (!m->entries[i])
      continue;
    LinkedList *list = m->entries[i];
    for (; list;) {
      if (list->upon_deletion)
        list->upon_deletion(list->key, list->value);
      LinkedList *old = list;
      list = list->next;
      free_linkedlist_entry(old);
    }
  }
  kfree(m->entries);
  kfree(m);
}

HashMap *hashmap_create(size_t size) {
  HashMap *m = kmalloc(sizeof(HashMap));
  if (!m)
    return NULL;

  m->size = size;
  m->num_entries = 0;

  // Create a array of pointers to linkedlists but don't create them
  // yet.
  m->entries = kcalloc(size, sizeof(LinkedList **));
  if (!m->entries)
    return NULL;

  for (size_t i = 0; i < m->size; i++)
    m->entries[i] = NULL;

  m->hash_function = hash;
  return m;
}
