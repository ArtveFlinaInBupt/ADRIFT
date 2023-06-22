#include "time.h"

#include "util/type.h"

#include <stdio.h>
#include <sys/time.h>

void get_current_time_format(char *buf) {
  struct timeval tv;
  struct tm *tm;
  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
  strftime(buf, 16, "%H:%M:%S", tm);
}
