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

void debug(int level, const char *format, ...) {
  if (level > debug_level)
    return;

  char buf[512 | 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 512, format, args);
  va_end(args);

  char time_buf[16 | 1];
  get_current_time_format(time_buf); // HH:MM:SS

  output_log(GREY "[%s] [DEBUG] %s" RESET "\n", time_buf, buf);
}

void print_log(Success success, const char *format, ...) {
  char buf[512 | 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 512, format, args);
  va_end(args);

  char time_buf[16 | 1];
  get_current_time_format(time_buf); // HH:MM:SS

  switch (success) {
    case FAILURE:
      output_log(RED "[%s] [FAILURE] %s" RESET "\n", time_buf, buf);
      break;
    case SUCCESS:
      output_log(GREEN "[%s] [SUCCESS] %s" RESET "\n", time_buf, buf);
      break;
  }
}

// void output_log(const char *buf) {
//   fprintf(stderr, "%s", buf);
//   if (log_file_ptr != NULL)
//     fprintf(log_file_ptr, "%s", buf);
// }
