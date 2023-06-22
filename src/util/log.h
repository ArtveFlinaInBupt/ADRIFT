#pragma once

#ifndef ADRIFT_UTIL_LOG_H
#  define ADRIFT_UTIL_LOG_H

#  include "util/type.h"

typedef enum {
  FAILURE = 0,
  SUCCESS = 1,
} Success;

void set_debug_level(i32 level);

void set_log_file(const char *file);

void destroy_log_file(void);

void debug(int level, const char *format, ...);

void print_log(Success success, const char *format, ...);

#endif // ADRIFT_UTIL_LOG_H
