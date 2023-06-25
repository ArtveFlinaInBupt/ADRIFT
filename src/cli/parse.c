#include "parse.h"

#include "util/constant.h"
#include "util/log.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__MACH__)
#  include <getopt.h>
#elif defined(__linux__)
#  include <getopt.h>
#else
#  include "external/getopt.h"
#endif

void print_usage(char *name) {
  printf(
      BOLD UNDERLINE "Usage:" RESET " " BOLD "%s" RESET " [OPTIONS]\n", name
  );
  printf("\n");
  printf(BOLD UNDERLINE "Options:\n" RESET);
  printf("  " BOLD "-d" RESET
         " <LEVEL>\t\tSet debug level [possible values: " UNDERLINE "0" RESET
         ", 1, 2]\n");
  printf("  " BOLD "-f" RESET " <FILENAME>\t\tSet hosts file\n");
  printf("  " BOLD "-h" RESET "\t\t\tPrint usage\n");
  printf("  " BOLD "-l" RESET " <FILENAME>\t\tSet log file\n");
  printf("  " BOLD "-s" RESET " <IP>\t\tSet DNS server\n");
  printf("  " BOLD "-v" RESET "\t\t\tPrint version info\n");
}

void print_version(void) {
  printf(BOLD "%s %s" RESET "\n", ADRIFT_NAME, ADRIFT_VERSION);
}

Arguments default_arguments(void) {
  Arguments args;
  args.hosts_file = NULL;
  args.debug_level = DEFAULT_DEBUG_LEVEL;
  args.dns_server = NULL;
  args.log_file = NULL;
  args.help = 0;
  args.version = 0;
  return args;
}

Arguments parse_arguments(int argc, char **argv) {
  Arguments args = default_arguments();

  for (int opt; (opt = getopt(argc, argv, "d:f:hl:.s:v")) != -1;) {
    switch (opt) {
      case 'd':
        args.debug_level = (i32)strtol(optarg, NULL, 10);
        if (args.debug_level < 0 || args.debug_level >= 3) {
          fprintf(stderr, "Invalid debug level: %d\n", args.debug_level);
          exit(EXIT_FAILURE);
        }
        break;
      case 'f':
        args.hosts_file = optarg;
        break;
      case 'h':
        args.help = 1;
        break;
      case 'l':
        args.log_file = optarg;
        break;
      case 's':
        args.dns_server = optarg;
        break;
      case 'v':
        args.version = 1;
        break;
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  return args;
}

void dump_arguments(Arguments *args) {
  debug(1, "Arguments:");
  debug(1, "  hosts_file: %s", args->hosts_file);
  debug(1, "  debug_level: %d", args->debug_level);
  debug(1, "  dns_server: %s", args->dns_server);
  if (args->log_file == NULL)
    debug(1, "  log_file: (null)");
  else
    debug(1, "  log_file: %s", args->log_file);
  debug(1, "  help: %d", args->help);
  debug(1, "  version: %d", args->version);
}
