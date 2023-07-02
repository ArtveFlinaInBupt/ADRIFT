#include "cache.h"

#include "ds/hash_map.h"
#include "ds/trie.h"
#include "rwlock/rwlock.h"
#include "util/log.h"
#include "util/type.h"

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
  for (const u8 *key_ = key; *key_ != '\0'; ++key_)
    if (*key - 33 < 0 || *key - 33 > 94) {
      debug(2, "Cache insert: [invalid key]", (char *)(key));
      return;
    }
  debug(1, "Cache insert: %s", (char *)(key));

  rwlock_write_lock(&rwlock);
  if (stream_length(key) <= 16) {
    trie_insert(trie[cache_type], key, node);
  } else {
    hash_map_insert(hash_map[cache_type], key, node);
  }

//  if (rand() % 500 < 1) { // NOLINT(cert-msc50-cpp)
//    trie_shrink(trie[cache_type]);
//    hash_map_shrink(hash_map[cache_type]);
//  }
  rwlock_write_unlock(&rwlock);
}

List *cache_find(CacheType cache_type, const u8 *key) {
  for (const u8 *key_ = key; *key_ != '\0'; ++key_)
    if (*key - 33 < 0 || *key - 33 > 94) {
      debug(2, "Cache find: [invalid key]", (char *)(key));
      return NULL;
    }
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
