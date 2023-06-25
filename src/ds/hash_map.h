#pragma once

#ifndef ADRIFT_HASH_MAP_H
#  define ADRIFT_HASH_MAP_H

#  include "ds/hash.h"
#  include "ds/list.h"
#  include "util/constant.h"

#  include <stdlib.h>
#  include <string.h>

#  define HASH_MAP_DEFAULT_CAPACITY 16
#  define HASH_MAP_LOAD_FACTOR      0.75
#  define HASH_MAP_GROWTH_FACTOR    2

typedef struct HashMapEntry {
  u64 hash;
  u8 key[IPV6_ADDR_BUF_SIZE];
  List *value;
  struct HashMapEntry *next;
} HashMapEntry;

static inline HashMapEntry *hash_map_entry_ctor(const u8 *key) {
  HashMapEntry *entry = (HashMapEntry *)calloc(1, sizeof(HashMapEntry));
  entry->hash = hash(key, stream_length(key));
  stream_copy(entry->key, key);
  entry->value = list_ctor();
  return entry;
}

static inline void hash_map_entry_dtor(HashMapEntry **entry) {
  if (*entry == NULL)
    return;
  hash_map_entry_dtor(&(*entry)->next);
  free((*entry)->value);
  free(*entry);
  *entry = NULL;
}

typedef struct HashMap {
  HashMapEntry **table;
  size_t size;
  size_t capacity;
} HashMap;

static inline HashMap *hash_map_ctor(void) {
  HashMap *hash_map = (HashMap *)calloc(1, sizeof(HashMap));
  hash_map->capacity = HASH_MAP_DEFAULT_CAPACITY;
  hash_map->table
      = (HashMapEntry **)calloc(hash_map->capacity, sizeof(HashMapEntry *));
  return hash_map;
}

static inline void hash_map_dtor(HashMap **hash_map) {
  if (*hash_map == NULL)
    return;
  for (size_t i = 0; i < (*hash_map)->capacity; ++i)
    hash_map_entry_dtor(&(*hash_map)->table[i]);
  free(*hash_map);
  *hash_map = NULL;
}

static inline void hash_map_rehash(HashMap *hash_map, size_t capacity) {
  HashMapEntry **new_table
      = (HashMapEntry **)calloc(capacity, sizeof(HashMapEntry *));
  for (size_t i = 0; i < hash_map->capacity; ++i) {
    HashMapEntry *entry = hash_map->table[i];
    while (entry != NULL) {
      HashMapEntry *next = entry->next;
      size_t idx = entry->hash % capacity;
      entry->next = new_table[idx];
      new_table[idx] = entry;
      entry = next;
    }
  }
  free(hash_map->table);
  hash_map->table = new_table;
  hash_map->capacity = capacity;
}

static inline void
hash_map_insert(HashMap *hash_map, const u8 *key, void *value) {
  u64 hash_ = hash(key, stream_length(key));
  size_t idx = hash_ % hash_map->capacity;
  HashMapEntry *entry = hash_map->table[idx];
  while (entry != NULL) {
    if (entry->hash == hash_ && stream_compare(entry->key, key) == 0) {
      list_push_back(entry->value, value);
      return;
    }
    entry = entry->next;
  }
  entry = hash_map_entry_ctor(key);
  list_push_back(entry->value, value);
  entry->next = hash_map->table[idx];
  hash_map->table[idx] = entry;
  hash_map->size++;

  if ((f64)hash_map->size > (f64)hash_map->capacity * HASH_MAP_LOAD_FACTOR)
    hash_map_rehash(hash_map, hash_map->capacity * HASH_MAP_GROWTH_FACTOR);
}

static inline List *hash_map_find(HashMap *hash_map, const u8 *key) {
  u64 hash_ = hash(key, stream_length(key));
  size_t idx = hash_ % hash_map->capacity;
  HashMapEntry *entry = hash_map->table[idx];
  while (entry != NULL) {
    if (entry->hash == hash_ && stream_compare(entry->key, key) == 0)
      return entry->value;
    entry = entry->next;
  }
  return NULL;
}

// static inline void hash_map_remove(HashMap *hash_map, const char *key) {
//   u64 hash_ = hash(key, strlen(key));
//   size_t idx = hash_ % hash_map->capacity;
//   HashMapEntry *entry = hash_map->table[idx];
//   HashMapEntry *prev = NULL;
//   while (entry != NULL) {
//     if (entry->hash == hash_ && strcmp(entry->key, key) == 0) {
//       if (prev == NULL)
//         hash_map->table[idx] = entry->next;
//       else
//         prev->next = entry->next;
//       hash_map_entry_dtor(&entry);
//       hash_map->size--;
//       return;
//     }
//     prev = entry;
//     entry = entry->next;
//   }
// }

// static inline void hash_map_clear(HashMap *hash_map) {
//   for (size_t i = 0; i < hash_map->capacity; ++i)
//     hash_map_entry_dtor(&hash_map->table[i]);
//   hash_map->size = 0;
// }

// static inline void hash_map_reserve(HashMap *hash_map, size_t capacity) {
//   if (capacity <= hash_map->capacity)
//     return;
//   size_t new_capacity = hash_map->capacity;
//   while (new_capacity < capacity)
//     new_capacity *= HASH_MAP_GROWTH_FACTOR;
//   hash_map_rehash(hash_map, new_capacity);
// }

#endif // ADRIFT_HASH_MAP_H
