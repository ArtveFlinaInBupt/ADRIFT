#pragma once

#ifndef ADRIFT_UTIL_TYPE_H
#  define ADRIFT_UTIL_TYPE_H

#  include <stddef.h>
#  include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

static inline u32 min_u32(u32 a, u32 b) {
  return a < b ? a : b;
}

static inline size_t stream_length(const u8 *str) {
  size_t len = 0;
  while (*str++ != '\0')
    ++len;
  return len;
}

static inline void stream_copy(u8 *dst, const u8 *src) {
  while (*src != '\0')
    *dst++ = *src++;
}

static inline int stream_compare(u8 *dst, const u8 *src) {
  while (*dst != '\0' && *src != '\0') {
    if (*dst != *src)
      return *dst - *src;
    ++dst;
    ++src;
  }
  return *dst - *src;
}

#endif // ADRIFT_UTIL_TYPE_H
