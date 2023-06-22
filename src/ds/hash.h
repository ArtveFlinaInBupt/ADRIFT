#pragma once

#ifndef ADRIFT_DS_HASH_H
#  define ADRIFT_DS_HASH_H

#  include "util/constant.h"
#  include "util/type.h"

static inline u64 hash(const void *data, size_t size) {
  u64 hash = FNV_OFFSET_BASIS;
  const u8 *ptr = (const u8 *)data;
  for (size_t i = 0; i < size; ++i) {
    hash ^= (u64)ptr[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

#endif // ADRIFT_DS_HASH_H
