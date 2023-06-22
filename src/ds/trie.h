#pragma once

#ifndef ADRIFT_DS_TRIE_H
#  define ADRIFT_DS_TRIE_H

#  include "util/type.h"

#  include <stdlib.h>

typedef struct TrieNode {
  void *data; // TODO: change type
  struct TrieNode *child[129];
} TrieNode;

static inline TrieNode *trie_node_ctor(const void *data) {
  TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
  node->data = (void *)data; // TODO: copy
  return node;
}

static inline void trie_node_dtor(TrieNode **node) {
  if (*node == NULL)
    return;
  for (i32 i = 0; i < 129; i++)
    trie_node_dtor(&(*node)->child[i]);
  free(*node);
}

typedef struct Trie {
  TrieNode *root;
} Trie;

static inline Trie *trie_ctor(void) {
  Trie *trie = (Trie *)calloc(1, sizeof(Trie));
  trie->root = trie_node_ctor(NULL);
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
static inline void trie_insert(Trie *trie, const char *key, const void *data) {
  TrieNode *node = trie->root;
  while (*key != '\0') {
    i32 cur_idx = (i32)*key;
    if (node->child[cur_idx] == NULL)
      node->child[cur_idx] = trie_node_ctor(NULL);
    node = node->child[cur_idx];
    key++;
  }
  node->data = (void *)data; // TODO: copy
}

/// @brief Find key in trie.
/// @return Data if found, NULL otherwise.
static inline void *trie_find(Trie *trie, const char *key) {
  TrieNode *node = trie->root;
  while (*key != '\0') {
    i32 cur_idx = (i32)*key;
    if (node->child[cur_idx] == NULL)
      return NULL;
    node = node->child[cur_idx];
    key++;
  }
  return node->data;
}

/// @brief Delete key from trie.
/// @return Data if found, NULL otherwise.
static inline void *trie_delete(Trie *trie, const char *key) {
  TrieNode *node = trie->root;
  while (*key != '\0') {
    i32 cur_idx = (i32)*key;
    if (node->child[cur_idx] == NULL)
      return NULL;
    node = node->child[cur_idx];
    key++;
  }
  void *data = node->data;
  node->data = NULL;
  return data;
}

#endif // ADRIFT_DS_TRIE_H
