#include "cache.h"

#include "ds/hash_map.h"
#include "ds/trie.h"
#include "util/log.h"
#include "util/type.h"
#include "rwlock/rwlock.h"

RwLock rwlock;

Trie *trie[3] = {NULL, NULL, NULL};
HashMap *hash_map[3] = {NULL, NULL, NULL};

void cache_ctor(CacheType cache_type) {
  trie[cache_type] = trie_ctor();
  hash_map[cache_type] = hash_map_ctor();

  rwlock_ctor(&rwlock);
}

void cache_dtor(CacheType cache_type) {
  rwlock_dtor(&rwlock);

  hash_map_dtor(&hash_map[cache_type]);
  trie_dtor(&trie[cache_type]);
}

void cache_insert(CacheType cache_type, const u8 *key, ListNode *node) {
  debug(1, "Cache insert: %s", (char *)(key));

  rwlock_write_lock(&rwlock);
  if (stream_length(key) <= 16) {
    trie_insert(trie[cache_type], key, node);
  } else {
    hash_map_insert(hash_map[cache_type], key, node);
  }
  rwlock_write_unlock(&rwlock);
}

List *cache_find(CacheType cache_type, const u8 *key) {
  debug(1, "Cache find: %s", (char *)(key));

  rwlock_read_lock(&rwlock);
  List *list = NULL;
  if (stream_length(key) <= 16) {
    list = trie_find(trie[cache_type], key);
  } else {
    list = hash_map_find(hash_map[cache_type], key);
  }
  rwlock_read_unlock(&rwlock);
  return list;
}
