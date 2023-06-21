#include "parse.h"

#include "util/constant.h"

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
  printf("  " BOLD "-d" RESET " <LEVEL>\t\tSet debug level\n");
  printf("  " BOLD "-f" RESET " <FILENAME>\t\tSet config file\n");
  printf("  " BOLD "-h" RESET "\t\t\tPrint usage\n");
  printf("  " BOLD "-s" RESET " <IP>\t\tSet DNS server\n");
  printf("  " BOLD "-v" RESET "\t\t\tPrint version info\n");
}

void print_version() {
  printf(BOLD "%s %s" RESET "\n", ADRIFT_NAME, ADRIFT_VERSION);
}

Arguments default_arguments() {
  Arguments args;
  args.config_file = DEFAULT_CONFIG_FILE;
  args.debug_level = DEFAULT_DEBUG_LEVEL;
  args.dns_server = DEFAULT_DNS_SERVER;
  args.help = 0;
  args.version = 0;
  return args;
}

Arguments parse_arguments(int argc, char **argv) {
  // TODO: gen by copilot, neither completed nor tested, just for demo
  Arguments args = default_arguments();

  for (int opt; (opt = getopt(argc, argv, "d:f:hs:v")) != -1;) {
    switch (opt) {
      case 'd':
        args.debug_level = atoi(optarg);
        if (args.debug_level < 0 || args.debug_level >= 3) {
          fprintf(stderr, "Invalid debug level: %d\n", args.debug_level);
          exit(EXIT_FAILURE);
        }
        break;
      case 'f':
        args.config_file = optarg;
        break;
      case 'h':
        args.help = 1;
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
  fprintf(stderr, "Arguments:\n");
  fprintf(stderr, "  config_file: %s\n", args->config_file);
  fprintf(stderr, "  debug_level: %d\n", args->debug_level);
  fprintf(stderr, "  dns_server: %s\n", args->dns_server);
  fprintf(stderr, "  version: %d\n", args->version);
}
