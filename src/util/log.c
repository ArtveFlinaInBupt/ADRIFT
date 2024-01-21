#include "log.h"

i32 debug_level = DEFAULT_DEBUG_LEVEL;

FILE *log_file_ptr = NULL;

#define output_log(format, ...)                   \
  do {                                            \
    fprintf(stderr, format, __VA_ARGS__);         \
    if (log_file_ptr != NULL) {                   \
      fprintf(log_file_ptr, format, __VA_ARGS__); \
      fflush(log_file_ptr);                       \
    }                                             \
  } while (0)

void set_debug_level(const i32 level) {
  debug_level = level;
}

void print_log(const Success success, const char *restrict format, ...) {
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

void set_log_file(const char *filename) {
  if (filename == NULL)
    return;

  log_file_ptr = fopen(filename, "w.");
  if (log_file_ptr == NULL) {
    print_log(FAILURE, "Failed to open log file: %s", filename);
    exit(EXIT_FAILURE);
  }
}

void debug(const int level, const char *restrict format, ...) {
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
