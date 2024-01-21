#pragma once

#ifndef ADRIFT_UTIL_TIME_H
#  define ADRIFT_UTIL_TIME_H

#  include "util/type.h"

#  include <sys/time.h>

static inline void get_current_time_format(char *buf) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  const struct tm *tm = localtime(&tv.tv_sec);
  strftime(buf, 16, "%H:%M:%S", tm);
}

#endif // ADRIFT_UTIL_TIME_H
