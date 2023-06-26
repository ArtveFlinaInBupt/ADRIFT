#include "cli/parse.h"
#include "protocol/cache.h"
#include "protocol/protocol.h"
#include "util/constant.h"
#include "util/ipv4addr.h"
#include "util/log.h"

#include <arpa/inet.h>
#include <dispatch/dispatch.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for close

#define BUFF_LEN    1'024
#define SERVER_PORT 53
#define MAP_LEN     (0xffff + 2) // 0x10001

// 获取一个udp请求,如果是来自客户端的请求,则进行解析,得到请求的网址
// 如果网址在cache里面,就将结果返回
// 如果网站不在,就向上级服务器请求,维护一个转发表
// 如果udp来自上级服务器,就查表将结果返回给客户端

int server_fd;
struct sockaddr_in server_addr;

typedef struct Convert {
  uint16_t buf_req_id;
  struct sockaddr_in buf_sock;
#ifdef __APPLE__
  dispatch_semaphore_t sem;
#else
  sem_t sem;
#endif
  uint8_t valid;
} Convert;

Convert convert_table[MAP_LEN];
// TODO: 改掉临时的转发表,加锁.
int buf1_len = 1;

typedef struct TimerContext {
  int buf_id;
} TimerContext;

void timer_handler(void *context) {
  TimerContext *timerContext = (TimerContext *)context;
  print_log(FAILURE, "超时: %d", timerContext->buf_id);
#ifdef __APPLE__
  dispatch_semaphore_signal(convert_table[timerContext->buf_id].sem);
#else
  sem_post(&convert_table[req_id].sem);
#endif
}

struct sockaddr_in dns_addr
    = {.sin_family = AF_INET,
       .sin_port = htons(53),
       .sin_addr.s_addr = 0x2c09030a};

typedef struct ThreadArg {
  uint8_t buf[BUFF_LEN];
  struct sockaddr_in client_addr;
  ssize_t recv_len;
} ThreadArg;

int handle_read_cache(
    ThreadArg *arg, DnsHeader *header, DnsQuestion *question
) {
  /// 查找 cache，删除过期 cache，向下游发包
  List *list = NULL;
  if (question->qtype == 1) {
    list = cache_find(CACHE_TYPE_IPV4, question->qname);
  } else if (question->qtype == 28) {
    list = cache_find(CACHE_TYPE_IPV6, question->qname);
  }

  if (list != NULL) {
    list_update(list); // 刷新缓存，删除过期项

    DnsHeader header_downstream = get_default_header();
    header_downstream.id = header->id;
    header_downstream.qr = 1;
    header_downstream.qdcount = 1;
    for (ListNode *p = list->head; p != NULL; p = p->next) {
      switch (p->type) {
        case ANSWER:
          ++header_downstream.ancount;
          break;
        case AUTHORITY:
          ++header_downstream.nscount;
          break;
        case ADDITIONAL:
          ++header_downstream.arcount;
          break;
      }
    }

    // 特判 0.0.0.0（从 dns-relay.txt / hosts 中读取）项并直接返回错误
    if (header_downstream.ancount != 0 || header_downstream.nscount != 0
        || header_downstream.arcount != 0) {
      for (ListNode *p = list->head; p != NULL; p = p->next) {
        if (p->data == NULL)
          continue;

        if (p->data->rdlength == 4
            && memcmp(p->data->rdata, "\x00\x00\x00\x00", 4) == 0) {
          u8 buf_downstream_[BUFF_LEN];
          u8 *buf_downstream = buf_downstream_;

          DnsHeader header_downstream_error = get_error_header(NAME_ERROR);
          header_downstream_error.id = header->id;
          dump_header(&buf_downstream, header_downstream_error);
          dump_question(&buf_downstream, question);
          dump_error_authority(&buf_downstream, question);

          sendto(
              server_fd,
              buf_downstream_,
              buf_downstream - buf_downstream_,
              0,
              (struct sockaddr *)&arg->client_addr,
              sizeof(arg->client_addr)
          );
          debug(
              1,
              "Send error response to client %s",
              inet_ntoa(arg->client_addr.sin_addr)
          );
          debug(2, "Response header id %d", header_downstream_error.id);

          return 1;
        }
      }
    }

    // 缓存中有其他值（不为 0.0.0.0）
    if (header_downstream.ancount != 0 || header_downstream.nscount != 0
        || header_downstream.arcount != 0) {
      u8 buf_downstream_[BUFF_LEN];
      u8 *buf_downstream = buf_downstream_;
      dump_header(&buf_downstream, header_downstream);
      dump_question(&buf_downstream, question);
      for (ListNode *p = list->head; p != NULL; p = p->next) {
        if (p->data == NULL)
          continue;

        DnsResourceRecord *rr
            = (DnsResourceRecord *)malloc(sizeof(DnsResourceRecord));
        memcpy(rr, p->data, sizeof(DnsResourceRecord));
        rr->ttl = p->ttl - (time(NULL) - p->record_time);
        dump_resource_record(&buf_downstream, rr);
        free(rr);
      }

      sendto(
          server_fd,
          buf_downstream_,
          buf_downstream - buf_downstream_,
          0,
          (struct sockaddr *)&arg->client_addr,
          sizeof(arg->client_addr)
      );
      debug(
          1, "Send response to client %s", inet_ntoa(arg->client_addr.sin_addr)
      );
      debug(2, "Response header id %d", header_downstream.id);
      debug(2, "Response header ancount %d", header_downstream.ancount);
      debug(2, "Response header nscount %d", header_downstream.nscount);
      debug(2, "Response header arcount %d", header_downstream.arcount);

      return 1;
    }
  }

  return 0;
}

int handle_send_query(
    ThreadArg *arg, DnsHeader *header, DnsQuestion *question
) {
  /// 写转换表
  u16 req_id = header->id;
  convert_table[buf1_len].buf_req_id = req_id;
  convert_table[buf1_len].buf_sock = arg->client_addr;
  if (buf1_len >= MAP_LEN) {
    buf1_len = 1;
  }

  /// 写向上游的请求报文
  header->id = buf1_len++;
  u8 buf_relay_[BUFF_LEN];
  u8 *buf_relay = buf_relay_;
  dump_header(&buf_relay, *header);
  dump_question(&buf_relay, question);
  sendto(
      server_fd,
      buf_relay_,
      buf_relay - buf_relay_,
      0,
      (struct sockaddr *)&dns_addr,
      sizeof(dns_addr)
  );
  debug(1, "Send query to dns server %s", inet_ntoa(dns_addr.sin_addr));
  debug(2, "Query header id %d", header->id);
  debug(2, "Query qname %s", question->qname);
  debug(2, "Query qtype %d", question->qtype);

  /// 等待上级dns回复或者超时
  convert_table[header->id].valid = 1;

  /// 创建定时器 dispatch timer
  debug(1, "Start timer for query id %d", header->id);
  dispatch_queue_t queue
      = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_source_t timer
      = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
  if (timer == NULL) {
    perror("Exit failure: dispatch_source_create");
    exit(EXIT_FAILURE);
  }
  TimerContext *timerContext = (TimerContext *)malloc(sizeof(TimerContext));
  timerContext->buf_id = header->id;
  dispatch_set_context(timer, timerContext);
  dispatch_set_finalizer_f(timer, free);

  int64_t interval = 8 * NSEC_PER_SEC; // 8s, TODO: 写常量
  dispatch_time_t start_time = dispatch_time(DISPATCH_TIME_NOW, interval);
  dispatch_source_set_timer(timer, start_time, interval, 0);
  dispatch_source_set_event_handler_f(timer, timer_handler);
  dispatch_resume(timer);

  // 等待超时或者收到上级dns的回复
#ifdef __APPLE__
  dispatch_semaphore_wait(convert_table[header->id].sem, DISPATCH_TIME_FOREVER);
#else
  sem_wait(&convert_table[header->id].sem);
#endif
  debug(1, "Stop timer for query id %d", header->id);
  convert_table[header->id].valid = 0;
  dispatch_source_cancel(timer);

  return 1;
}

int handle_receive_query(ThreadArg *arg, u8 *buf, DnsHeader *header) {
  /// 解析下游的请求报文
  DnsQuestion question;
  // TODO: 写个返回报错包的函数，如果 qdcount != 1 就返回报错包
  for (int i = 0; i < header->qdcount; i++) {
    parse_question(&buf, &question);

    debug(
        1,
        "qname: %s, qtype: %d, qclass: %d",
        question.qname,
        question.qtype,
        question.qclass
    );
  }

  if (handle_read_cache(arg, header, &question) == 1)
    return 1;

  handle_send_query(arg, header, &question);
  return 1;
}

int handle_send_response(
    u16 req_id, DnsHeader *header, DnsQuestion *question, DnsResourceRecord **answer,
    DnsResourceRecord **authority, DnsResourceRecord **additional
) {
  /// 写向下游的响应报文
  uint8_t buf_relay_[BUFF_LEN];
  u8 *buf_relay = buf_relay_;

  // 写 header
  header->id = convert_table[req_id].buf_req_id;
  // TODO: 一些字段需要修改，如 aa
  dump_header(&buf_relay, *header);
  dump_question(&buf_relay, question);

  // 写 answer body
  for (int i = 0; i < header->ancount; ++i)
    dump_resource_record(&buf_relay, answer[i]);
  for (int i = 0; i < header->nscount; ++i)
    dump_resource_record(&buf_relay, authority[i]);
  for (int i = 0; i < header->arcount; ++i)
    dump_resource_record(&buf_relay, additional[i]);

  sendto(
      server_fd,
      buf_relay_,
      buf_relay - buf_relay_,
      0,
      (struct sockaddr *)&(convert_table[req_id].buf_sock),
      sizeof(convert_table[req_id].buf_sock)
  );

  return 1;
}

int handle_receive_response(
    ThreadArg *arg, u8 *buf, DnsHeader *header
) {
  // 解析响应报文中的 Question
  DnsQuestion question;
  parse_question(&buf, &question);

  // 解析响应报文
  DnsResourceRecord **answer = NULL;
  DnsResourceRecord **authority = NULL;
  DnsResourceRecord **additional = NULL;

  if (header->ancount > 0) {
    answer = (DnsResourceRecord **)malloc(
        sizeof(DnsResourceRecord *) * header->ancount
    );
    for (int i = 0; i < header->ancount; i++) {
      answer[i] = (DnsResourceRecord *)malloc(sizeof(DnsResourceRecord));
      parse_resource_record(&buf, arg->buf, answer[i]);

      debug(
          2,
          "Receive Answer RR name: %s, type: %d, class: %d, ttl: %d, rdlength: %d",
          answer[i]->name,
          answer[i]->type,
          answer[i]->class,
          answer[i]->ttl,
          answer[i]->rdlength
      );
    }
  }

  if (header->nscount > 0) {
    authority = (DnsResourceRecord **)malloc(
        sizeof(DnsResourceRecord *) * header->nscount
    );
    for (int i = 0; i < header->nscount; i++) {
      authority[i] = (DnsResourceRecord *)malloc(sizeof(DnsResourceRecord));
      parse_resource_record(&buf, arg->buf, authority[i]);
    }
  }

  if (header->arcount > 0) {
    additional = (DnsResourceRecord **)malloc(
        sizeof(DnsResourceRecord *) * header->arcount
    );
    for (int i = 0; i < header->arcount; i++) {
      additional[i] = (DnsResourceRecord *)malloc(sizeof(DnsResourceRecord));
      parse_resource_record(&buf, arg->buf, additional[i]);
    }
  }

  /// 写 cache
  if (header->rcode == NO_ERROR) {
    time_t cur_time = time(NULL);

    for (int i = 0; i < header->ancount; i++) {
      ListNode *node = list_node_ctor_with_info(
          answer[i], ANSWER, cur_time, min_u32(180, answer[i]->ttl)
      );

      if (answer[i]->type == 1)
        cache_insert(CACHE_TYPE_IPV4, question.qname, node);
      else if (answer[i]->type == 28)
        cache_insert(CACHE_TYPE_IPV6, question.qname, node);
    }

    for (int i = 0; i < header->nscount; i++) {
      ListNode *node = list_node_ctor_with_info(
          authority[i], AUTHORITY, cur_time, min_u32(180, authority[i]->ttl)
      );

      if (authority[i]->type == 1)
        cache_insert(CACHE_TYPE_IPV4, question.qname, node);
      else if (authority[i]->type == 28)
        cache_insert(CACHE_TYPE_IPV6, question.qname, node);
    }

    for (int i = 0; i < header->arcount; i++) {
      ListNode *node = list_node_ctor_with_info(
          additional[i], ADDITIONAL, cur_time, min_u32(180, additional[i]->ttl)
      );

      if (additional[i]->type == 1)
        cache_insert(CACHE_TYPE_IPV4, question.qname, node);
      else if (additional[i]->type == 28)
        cache_insert(CACHE_TYPE_IPV6, question.qname, node);
    }
  }

  /// 获得 id 与检查有效性（未超时）
  u16 req_id = header->id;
  if (convert_table[req_id].valid == 0) {
    print_log(FAILURE, "Invalid response id: %d", req_id); // TODO: 疑似应当返回 FORMERR 而不是直接丢弃
    return 0;
  }

  handle_send_response(req_id, header, &question, answer, authority, additional);

  if (answer)
    free(answer);
  if (authority)
    free(authority);
  if (additional)
    free(additional);

  // 释放转发表
#ifdef __APPLE__
  dispatch_semaphore_signal(convert_table[req_id].sem);
#else
  sem_post(&convert_table[req_id].sem);
#endif

  return 1;
}

void *pthread_func(void *arg_) {
  ThreadArg *arg = (ThreadArg *)arg_;

  DnsHeader header;
  u8 *buf = arg->buf;
  parse_header(&buf, &header);

  if (header.qr == 0)
    handle_receive_query(arg, buf, &header);
  else if (header.qr == 1)
    handle_receive_response(arg, buf, &header);

  free(arg);
  return NULL;
}

void exit_handler(int sig) {
  if (!sig || sig == SIGHUP || sig == SIGINT || sig == SIGTERM) {
    if (sig != 0)
      debug(0, "Exit with signal %d", sig);
    close(server_fd);
    destroy_log_file();
    cache_dtor(CACHE_TYPE_IPV4);
    cache_dtor(CACHE_TYPE_IPV6);
    exit(0);
  }
}

// void crash_handler(int sig) {
//   if (sig == SIGSEGV) {
//     debug(0, "Crash with signal %d\n", sig);
//     close(server_fd);
//     cache_dtor(CACHE_TYPE_IPV4);
//     cache_dtor(CACHE_TYPE_IPV6);
//
//     cache_ctor(CACHE_TYPE_IPV6);
//     cache_ctor(CACHE_TYPE_IPV4);
//     server_fd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (server_fd < 0) {
//       print_log(FAILURE, "Failed to create socket\n");
//       exit(1);
//     }
//
//   }
// }

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

    // if domain don't ends with '.', then add it
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

void event_loop() {
  u32 sock_len = sizeof(struct sockaddr_in);
  while (1) {
    ThreadArg *arg = (ThreadArg *)calloc(1, sizeof(ThreadArg));

    ssize_t recv_len = recvfrom(
        server_fd,
        arg->buf,
        BUFF_LEN,
        0,
        (struct sockaddr *)&(arg->client_addr),
        &sock_len
    );

    if (recv_len < 0) {
      print_log(FAILURE, "recvfrom failed!");
      break;
    }

    debug(
        1,
        "Received from client: %s:%d",
        inet_ntoa(arg->client_addr.sin_addr),
        ntohs(arg->client_addr.sin_port)
    );

    arg->recv_len = recv_len;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, pthread_func, (void *)arg);
  }
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

  event_loop();

  exit_handler(0);

  return 0;
}
