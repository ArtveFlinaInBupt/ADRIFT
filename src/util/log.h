#pragma once

#ifndef ADRIFT_UTIL_LOG_H
#  define ADRIFT_UTIL_LOG_H

#  include "util/constant.h"
#  include "util/time.h"
#  include "util/type.h"

#  include <stdarg.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

extern i32 debug_level;

extern FILE *log_file_ptr;

typedef enum {
  FAILURE = 0,
  SUCCESS = 1,
} Success;

void set_debug_level(i32 level);

/// @brief Print log message.
/// @param success Success or failure.
/// @param format Format string.
/// @param ... Arguments.
/// @example print_log(SUCCESS, "Hello, %s", "world");
/// @example print_log(FAILURE, "Goodbye, %s", "world");
void print_log(Success success, const char *restrict format, ...);

void set_log_file(const char *filename);

static inline void destroy_log_file(void) {
  if (log_file_ptr != NULL)
    fclose(log_file_ptr);
}

/// @brief Print debug message.
/// @param level Debug level.
/// @param format Format string.
/// @param ... Arguments.
/// @note If the debug level is greater than the current debug level, the
/// message will not be printed.
/// @example debug(1, "Hello, %s", "world");
void debug(int level, const char *restrict format, ...);

#endif // ADRIFT_UTIL_LOG_H
