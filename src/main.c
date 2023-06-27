#include "cli/parse.h"
#include "protocol/cache.h"
#include "protocol/handle.h"
#include "protocol/protocol.h"
#include "util/log.h"
#include "thread/threadpool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __APPLE__
#  include <dispatch/dispatch.h>
#else
#  include <semaphore.h>
#endif

#define SERVER_PORT 53

void exit_handler(int sig) {
  if (!sig || sig == SIGHUP || sig == SIGINT || sig == SIGTERM) {
    if (sig != 0)
      debug(0, "Exit with signal %d", sig);
    close(server_fd);
    destroy_log_file();
    cache_dtor(CACHE_TYPE_IPV4);
    cache_dtor(CACHE_TYPE_IPV6);
    cache_dtor(CACHE_TYPE_CNAME);
  }
}

int program_init(const Arguments *args) {
  signal(SIGHUP, exit_handler);
  signal(SIGINT, exit_handler);
  signal(SIGTERM, exit_handler);
  signal(SIGQUIT, exit_handler);

  set_debug_level(args->debug_level);
  if (args->log_file) {
    set_log_file(args->log_file);
    print_log(SUCCESS, "Log file: %s", args->log_file);
  }

  return 0;
}

int read_hosts_file(const char *filename) {
  // only ipv4, in the format of "ip domain"
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    print_log(FAILURE, "Failed to open hosts file: %s", filename);
    return -1;
  }

  char line[512 | 1];
  while (fgets(line, 512, fp)) {
    if (!line[0] || line[0] == '\n')
      continue;

    char *ip = strtok(line, " \t\r\n");
    char *domain = strtok(NULL, " \t\r\n");
    if (!ip || !domain) {
      print_log(FAILURE, "Invalid line in hosts file: %s", line);
      continue;
    }

    // if domain don't end with '.', then add it
    if (domain[strlen(domain) - 1] != '.') {
      char *tmp = (char *)malloc(strlen(domain) + 2);
      strcpy(tmp, domain);
      strcat(tmp, ".");
      domain = tmp;
    }
    struct in_addr addr;
    if (inet_pton(AF_INET, ip, &addr) != 1) {
      print_log(FAILURE, "Invalid ip address in hosts file: %s", ip);
      continue;
    }

    DnsResourceRecord *rr
        = (DnsResourceRecord *)malloc(sizeof(DnsResourceRecord));
    rr->name = (u8 *)malloc(strlen(domain) + 1);
    memcpy(rr->name, domain, strlen(domain) + 1);
    rr->type = 1;  // Ipv4
    rr->class = 1; // IN
    rr->ttl = 180;
    rr->rdlength = sizeof addr; // 4 octets
    rr->rdata = (u8 *)malloc(rr->rdlength);
    memcpy(rr->rdata, &addr, rr->rdlength);

    cache_insert(
        CACHE_TYPE_IPV4,
        (const u8 *)domain,
        list_node_ctor_with_info(rr, ANSWER, 0, 0)
    );
  }

  return 0;
}

int server_init(const Arguments *args) {
  // 初始化 cache
  cache_ctor(CACHE_TYPE_IPV4);
  cache_ctor(CACHE_TYPE_IPV6);
  cache_ctor(CACHE_TYPE_CNAME);

  // 处理配置文件 dns-relay.txt (hosts)
  if (args->hosts_file && read_hosts_file(args->hosts_file) < 0)
    return -1;

  // 配置上游 DNS 服务器地址
  if (args->dns_server)
    dns_addr.sin_addr.s_addr = inet_addr(args->dns_server);
  print_log(SUCCESS, "DNS server: %s", inet_ntoa(dns_addr.sin_addr));

  // 初始化 socket
  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd < 0) {
    print_log(FAILURE, "create socket failed!");
    return -1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))
      < 0) {
    print_log(FAILURE, "bind failed!");
    return -1;
  }

#ifdef __APPLE__
  for (int i = 0; i < MAP_LEN; i++)
    convert_table[i].sem = dispatch_semaphore_create(0);
#else
  for (int i = 0; i < MAP_LEN; i++)
    sem_init(&convert_table[i].sem, 0, 0);
#endif

  return 0;
}

int main(int argc, char **argv) {
  // 处理命令行参数
  Arguments args = parse_arguments(argc, argv);
  if (args.version) {
    print_version();
    return EXIT_SUCCESS;
  }
  if (args.help) {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  // 程序初始化
  program_init(&args);
  if (server_init(&args) < 0)
    return EXIT_FAILURE;

//   ThreadPool* pool = threadPoolCreate(10, 200, 200);
  event_loop();

  exit_handler(0);

  return 0;
}
