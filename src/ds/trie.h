#pragma once

#ifndef ADRIFT_DS_TRIE_H
#  define ADRIFT_DS_TRIE_H

#  include "ds/list.h"
#  include "util/log.h"
#  include "util/type.h"

#  include <stdlib.h>

typedef struct TrieNode {
  List *data;
  struct TrieNode *child[95];
} TrieNode;

static inline TrieNode *trie_node_ctor() {
  TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
  return node;
}

static inline void trie_node_dtor(TrieNode **node) {
  if (*node == NULL)
    return;
  list_dtor(&(*node)->data);
  for (i32 i = 0; i < 95; i++)
    trie_node_dtor(&(*node)->child[i]);
  free(*node);
  *node = NULL;
}

static inline size_t trie_node_shrink(TrieNode *node) {
  size_t count = 0;

  if (node->data != NULL)
    count += list_update(node->data);

  for (i32 i = 0; count == 0 && i < 95; i++)
    if (node->child[i] != NULL)
      count += trie_node_shrink(node->child[i]);

  if (count == 0)
    trie_node_dtor(&node);

  return count;
}

typedef struct Trie {
  TrieNode *root;
} Trie;

static inline Trie *trie_ctor(void) {
  Trie *trie = (Trie *)calloc(1, sizeof(Trie));
  trie->root = trie_node_ctor();
  return trie;
}

static inline void trie_dtor(Trie **trie) {
  if (*trie == NULL)
    return;
  trie_node_dtor(&(*trie)->root);
  free(*trie);
}

/// @brief Insert or modify key into trie.
/// @details If key already exists, modify data.
static inline void trie_insert(Trie *trie, const u8 *key, ListNode *data) {
  TrieNode *node = trie->root;

  const u8 *end = key;
  while (*end != '\0')
    ++end;
  --end;
  for (; end >= key; --end) {
    i32 cur_idx = *end - 33;
    if (node->child[cur_idx] == NULL)
      node->child[cur_idx] = trie_node_ctor();
    node = node->child[cur_idx];
  }

  if (node->data == NULL)
    node->data = list_ctor();
  list_push_back(node->data, data);
}

/// @brief Find key in trie.
/// @return Data if found, NULL otherwise.
static inline List *trie_find(Trie *trie, const u8 *key) {
  TrieNode *node = trie->root;

  const u8 *end = key;
  while (*end != '\0')
    ++end;
  --end;
  for (; end >= key; --end) {
    i32 cur_idx = *end - 33;
    if (node->child[cur_idx] == NULL)
      return NULL;
    node = node->child[cur_idx];
  }

  return node->data;
}

static inline void trie_shrink(Trie *trie) {
  trie_node_shrink(trie->root);
}

/// @brief Delete key from trie.
/// @return Data if found, NULL otherwise.
static inline void trie_delete(Trie *trie, const u8 *key) {
  TrieNode *node = trie->root;

  const u8 *end = key;
  while (*end != '\0')
    ++end;
  --end;
  for (; end >= key; --end) {
    i32 cur_idx = *end - 33;
    if (node->child[cur_idx] == NULL)
      return;
    node = node->child[cur_idx];
  }

  list_dtor(&node->data);
  node->data = NULL;
}

#endif // ADRIFT_DS_TRIE_H
