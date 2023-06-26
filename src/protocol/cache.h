/*
 * # 缓存 cache 机制
 *
 * 将上游服务器返回的有效 response 中 IPv4, CNAME 和 IPv6 三类 RR 存入缓存。 TODO: CNAME
 *
 * dns-relay.txt (hosts) 中的表项也存放在 cache 中，用特殊值标识，防止被删除。
 *
 * 每个 RR 的 TTL 在上游服务器返回的值和 180s 中取最小值。
 * 采用 [向外界返回的 TTL - 10 秒] 作为数据在服务器中存活的 TTL。
 * 查询缓存时会根据在服务器中存活的 TTL 刷新缓存，过期则销毁。
 *
 * 为防止多线程数据竞争，缓存访问加读写锁。
 * 读写锁是利用 mutex, condition variable 和 atomic variable 自行实现的写优先读写锁。
 *
 * 缓存外部接口采用面向对象的封装设计思路。
 *
 * 缓存内部数据结构采用 Trie 树（字典树）和 HashMap（散列表）两种，不作赘述。
 */

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

void cache_insert(CacheType cache_type, const u8 *key, ListNode *value);

List *cache_find(CacheType cache_type, const u8 *key);

#endif // ADRIFT_PROTOCOL_CACHE_H
