#include "log.h"

#include "util/constant.h"
#include "util/time.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static i32 debug_level = DEFAULT_DEBUG_LEVEL;

static FILE *log_file_ptr = NULL;

#define output_log(format, ...)                   \
  do {                                            \
    fprintf(stderr, format, __VA_ARGS__);         \
    if (log_file_ptr != NULL) {                   \
      fprintf(log_file_ptr, format, __VA_ARGS__); \
      fflush(log_file_ptr);                       \
    }                                             \
  } while (0)

void set_debug_level(i32 level) {
  debug_level = level;
}

void set_log_file(const char *file) {
  if (file == NULL)
    return;

  log_file_ptr = fopen(file, "w.");
  if (log_file_ptr == NULL) {
    print_log(FAILURE, "Failed to open log file: %s", file);
    exit(EXIT_FAILURE);
  }
}

void destroy_log_file(void) {
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
void debug(int level, const char * restrict format, ...) {
  if (level > debug_level)
    return;

  char buf[512 | 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 512, format, args);
  va_end(args);

  char time_buf[8 | 1];
  get_current_time_format(time_buf);

  output_log(GREY "[%s] [DEBUG] %s" RESET "\n", time_buf, buf);
}

/// @brief Print log message.
/// @param success Success or failure.
/// @param format Format string.
/// @param ... Arguments.
/// @example print_log(SUCCESS, "Hello, %s", "world");
/// @example print_log(FAILURE, "Goodbye, %s", "world");
void print_log(Success success, const char * restrict format, ...) {
  char buf[512 | 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 512, format, args);
  va_end(args);

  char time_buf[8 | 1];
  get_current_time_format(time_buf);

  switch (success) {
    case FAILURE:
      output_log(RED "[%s] [FAILURE] %s" RESET "\n", time_buf, buf);
      break;
    case SUCCESS:
      output_log(GREEN "[%s] [SUCCESS] %s" RESET "\n", time_buf, buf);
      break;
  }
}
