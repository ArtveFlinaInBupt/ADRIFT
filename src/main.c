#include "cli/parse.h"

#include <stdio.h>

int main(int argc, char **argv) {
  Arguments args = parse_arguments(argc, argv);

  if (args.version) {
    print_version();
    return 0;
  }
  if (args.help) {
    print_usage(argv[0]);
    return 0;
  }

  dump_arguments(&args);
}
