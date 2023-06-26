#pragma once

#ifndef ADRIFT_DS_LIST_H
#  define ADRIFT_DS_LIST_H

#  include "protocol/protocol.h"
#  include "util/type.h"

#  include <stdlib.h>
#  include <time.h>

typedef struct ListNode {
  DnsResourceRecord *data;
  time_t record_time;
  time_t ttl;
  RrType type;
  struct ListNode *prev;
  struct ListNode *next;
} ListNode;

/// @brief Constructor for a list node.
/// @param data The data to store in the node.
/// @return The list node.
static inline ListNode *list_node_ctor(void *data) {
  ListNode *node = malloc(sizeof(ListNode));
  node->data = data;
  node->record_time = 0;
  node->ttl = 0;
  node->type = 0;
  node->prev = NULL;
  node->next = NULL;
  return node;
}

/// @brief Constructor for a list node with a record time and ttl.
/// @param data The data to store in the node.
/// @param record_time The record time.
/// @param ttl The time to live.
/// @return The list node.
static inline ListNode *list_node_ctor_with_info(
    void *data, RrType rr_type, time_t record_time, time_t ttl
) {
  ListNode *node = list_node_ctor(data);
  node->type = rr_type;
  node->record_time = record_time;
  node->ttl = ttl;
  return node;
}

/// @brief Destructor for a list node.
/// @param node The list node.
static inline void list_node_dtor(ListNode **node) {
  if (*node == NULL)
    return;
  if ((*node)->record_time != 0)
    free((*node)->data);
  free(*node);
}

/// @brief Linked list data structure.
/// @note  The list is implemented as a doubly linked list (with a head node).
typedef struct List {
  ListNode *head;
  ListNode *tail;
  size_t size;
} List;

/// @brief Constructor for a list.
/// @return The list.
static inline List *list_ctor(void) {
  List *list = malloc(sizeof(List));
  list->head = list_node_ctor(NULL);
  list->tail = list->head;
  list->size = 0;
  return list;
}

/// @brief Destructor for a list.
/// @param list The list.
static inline void list_dtor(List **list) {
  if (*list == NULL)
    return;
  for (ListNode *node = (*list)->head, *next; node != NULL; node = next) {
    next = node->next;
    list_node_dtor(&node);
  }
  free(*list);
  *list = NULL;
}

/// @brief Pushes an element to the beginning of the list.
/// @param list The list.
/// @param data The data to push.
static inline void list_push_back(List *list, ListNode *node) {
//  ListNode *node = list_node_ctor(data);
  list->tail->next = node;
  node->prev = list->tail;
  list->tail = node;
  ++list->size;
}

/// @brief Erases a specific element from the list.
/// @param list The list.
/// @param node The node to erase.
static inline void list_erase(List *list, ListNode *node) {
  if (node == NULL)
    return;
  if (node->prev != NULL)
    node->prev->next = node->next;
  if (node->next != NULL)
    node->next->prev = node->prev;
  if (node == list->head)
    list->head = node->next;
  if (node == list->tail)
    list->tail = node->prev;
  list_node_dtor(&node);
  --list->size;
}

/// @brief Updates the linked list, removing expired elements.
/// @param list The list.
static inline void list_update(List *list) {
  time_t now = time(NULL);
  for (ListNode *node = list->head, *next; node != NULL; node = next) {
    next = node->next;
    if (node->ttl != 0 && now - (node->record_time + node->ttl) > 10)
      list_erase(list, node);
  }
}

/// @brief Checks if the list is empty.
/// @param list The list.
static inline int is_list_empty(List *list) {
  return list->size == 0;
}

/// @brief Returns the size of the list.
/// @param list The list.
static inline size_t list_size(List *list) {
  return list->size;
}

#endif // ADRIFT_DS_LIST_H
