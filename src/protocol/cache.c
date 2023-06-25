#include "cache.h"

#include "ds/hash_map.h"
#include "ds/trie.h"
#include "util/type.h"

Trie *trie[2] = {NULL, NULL};
HashMap *hash_map[2] = {NULL, NULL};

void cache_ctor(CacheType cache_type) {
  trie[cache_type] = trie_ctor();
  hash_map[cache_type] = hash_map_ctor();
}

void cache_dtor(CacheType cache_type) {
  trie_dtor(&trie[cache_type]);
  hash_map_dtor(&hash_map[cache_type]);
}

void cache_insert(CacheType cache_type, const u8 *key, void *value) {
  if (stream_length(key) <= 16) {
    trie_insert(trie[cache_type], key, value);
  } else {
    hash_map_insert(hash_map[cache_type], key, value);
  }
}

const List *cache_find(CacheType cache_type, const u8 *key) {
  if (stream_length(key) <= 16) {
    return trie_find(trie[cache_type], key);
  } else {
    return hash_map_find(hash_map[cache_type], key);
  }
}
