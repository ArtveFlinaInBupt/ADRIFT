#include "cli/parse.h"
#include "util/log.h"

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

  set_debug_level(args.debug_level);
  set_log_file(args.log_file);

  dump_arguments(&args);

  destroy_log_file();
}
