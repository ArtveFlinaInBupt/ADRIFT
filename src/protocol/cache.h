#pragma once

#ifndef ADRIFT_PROTOCOL_CACHE_H
#  define ADRIFT_PROTOCOL_CACHE_H

#  include "ds/hash_map.h"
#  include "ds/trie.h"

typedef enum CacheType {
  CACHE_TYPE_IPV4 = 0,
  CACHE_TYPE_IPV6 = 1,
} CacheType;

extern Trie *trie[2];
extern HashMap *hash_map[2];

void cache_ctor(CacheType cache_type);

void cache_dtor(CacheType cache_type);

void cache_insert(CacheType cache_type, const u8 *key, void *value);

const List *cache_find(CacheType cache_type, const u8 *key);

#endif // ADRIFT_PROTOCOL_CACHE_H
