#pragma once

#ifndef ADRIFT_HANDLE_H
#  define ADRIFT_HANDLE_H

#  include "ds/list.h"
#  include "handle.h"
#  include "protocol/cache.h"
#  include "util/log.h"

#  include <arpa/inet.h>
#  include <dispatch/dispatch.h>
#  include <pthread.h>
#include "thread/threadpool.h"


#  define BUFFER_LEN 2048
#  define MAP_LEN    (0xffff + 2) // 0x10001

int server_fd;
struct sockaddr_in server_addr;
extern struct sockaddr_in dns_addr;

typedef struct Convert {
  uint16_t buf_req_id;
  struct sockaddr_in buf_sock;
#ifdef __APPLE__
  dispatch_semaphore_t sem;
#else // elseifdef __linux__
  sem_t sem;
#endif
  uint8_t valid;
} Convert;

Convert convert_table[MAP_LEN];

typedef struct ThreadArg {
  uint8_t buf[BUFFER_LEN];
  struct sockaddr_in client_addr;
  ssize_t recv_len;
} ThreadArg;

int handle_read_cache(ThreadArg *arg, DnsHeader *header, DnsQuestion *question);

int handle_send_query(ThreadArg *arg, DnsHeader *header, DnsQuestion *question);

int handle_receive_query(ThreadArg *arg, u8 *buf, DnsHeader *header);

int handle_send_response(
    u16 req_id, DnsHeader *header, DnsQuestion *question,
    DnsResourceRecord **answer, DnsResourceRecord **authority,
    DnsResourceRecord **additional
);

int handle_receive_response(ThreadArg *arg, u8 *buf, DnsHeader *header);

void *handle_stream(void *arg_);

void event_loop();

#endif // ADRIFT_HANDLE_H
