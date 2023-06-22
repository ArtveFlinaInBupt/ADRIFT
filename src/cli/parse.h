#pragma once

#ifndef ADRIFT_CLI_PARSE_H
#  define ADRIFT_CLI_PARSE_H

#include "util/type.h"

typedef struct {
  const char *dns_server;
  const char *hosts_file;
  const char *log_file;
  i32 debug_level;
  i8 help;
  i8 version;
} Arguments;

void print_usage(char *name);

void print_version(void);

Arguments default_arguments(void);

Arguments parse_arguments(int argc, char **argv);

void dump_arguments(Arguments *args);

#endif // ADRIFT_CLI_PARSE_H
