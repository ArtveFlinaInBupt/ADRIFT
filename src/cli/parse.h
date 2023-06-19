#pragma once

#ifndef ADRIFT_CLI_PARSE_H
#  define ADRIFT_CLI_PARSE_H

#include "util/type.h"

typedef struct {
  const char *dns_server;
  const char *config_file;
  i32 debug_level;
  i8 help;
  i8 version;
} Arguments;

void print_usage(char *name);

void print_version();

Arguments default_arguments();

Arguments parse_arguments(int argc, char **argv);

void dump_arguments(Arguments *args);

#endif // ADRIFT_CLI_PARSE_H
