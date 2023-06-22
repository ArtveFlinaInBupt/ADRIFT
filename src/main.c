#include "cli/parse.h"
#include "util/constant.h"
#include "util/ipv4addr.h"
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

  // char ipv4_addr_buf[IPV4_ADDR_BUF_SIZE];
  // ipv4_addr_to_str(0x01020304, ipv4_addr_buf);
  // print_log(SUCCESS, "ipv4_addr_to_str(0x01020304) = %s", ipv4_addr_buf);
  char ipv4_addr_str[] = "127.0.0.1";
  Ipv4Addr ipv4_addr = ipv4_addr_from_str(ipv4_addr_str);
  u8 octet = ipv4_addr_to_octet(ipv4_addr, 1);
  char ipv4_addr_buf[IPV4_ADDR_BUF_SIZE];
  ipv4_addr_to_str(ipv4_addr, ipv4_addr_buf);
  print_log(
      SUCCESS, "ipv4_addr_from_str(\"%s\") = 0x%08x", ipv4_addr_str, ipv4_addr
  );
  print_log(
      SUCCESS, "ipv4_addr_to_octet(0x%08x, 1) = 0x%02x", ipv4_addr, octet
  );
  print_log(SUCCESS, "ipv4_addr_to_str(0x%08x) = %s", ipv4_addr, ipv4_addr_buf);

  destroy_log_file();
}
