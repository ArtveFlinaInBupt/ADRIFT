#include "handle.h"

#include "ds/list.h"
#include "protocol/cache.h"
#include "util/constant.h"
#include "util/log.h"

#include <arpa/inet.h>
#include <dispatch/dispatch.h>
#include <pthread.h>

struct sockaddr_in dns_addr = {
  .sin_family = AF_INET,
  .sin_port = htons(53),
  .sin_addr.s_addr = 0x2c09030a
};

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

int handle_read_cache(
    ThreadArg *arg, DnsHeader *header, DnsQuestion *question
) {
  /// 查找 cache，删除过期 cache，向下游发包
  List *list = NULL;
  if (question->qtype == 1) {
    list = cache_find(CACHE_TYPE_IPV4, question->qname);
  } else if (question->qtype == 28) {
    list = cache_find(CACHE_TYPE_IPV6, question->qname);
  } else if (question->qtype == 5) {
    list = cache_find(CACHE_TYPE_CNAME, question->qname);
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
          u8 buf_downstream_[BUFFER_LEN];
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
      u8 buf_downstream_[BUFFER_LEN];
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
  u8 buf_relay_[BUFFER_LEN];
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

  i64 interval = DEFAULT_TIMEOUT;
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

  if (header->qdcount != 1) {
    print_log(FAILURE, "Error: qdcount != 1");
    u8 buf_downstream_[BUFFER_LEN];
    u8 *buf_downstream = buf_downstream_;

    DnsHeader header_nxdomain = get_error_header(1);
    dump_header(&buf_downstream, header_nxdomain);

    sendto(
        server_fd,
        buf_downstream_,
        buf_downstream - buf_downstream_,
        0,
        (struct sockaddr *)&arg->client_addr,
        sizeof(arg->client_addr)
    );

    return 1;
  }

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
    u16 req_id, DnsHeader *header, DnsQuestion *question,
    DnsResourceRecord **answer, DnsResourceRecord **authority,
    DnsResourceRecord **additional
) {
  /// 写向下游的响应报文
  uint8_t buf_relay_[BUFFER_LEN];
  u8 *buf_relay = buf_relay_;

  // 写 header
  header->id = convert_table[req_id].buf_req_id;
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

int handle_receive_response(ThreadArg *arg, u8 *buf, DnsHeader *header) {
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
          "Receive Answer RR name: %s, type: %d, class: %d, ttl: %d, rdlength: "
          "%d",
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
      else if (answer[i]->type == 5)
        cache_insert(CACHE_TYPE_CNAME, question.qname, node);
    }

    for (int i = 0; i < header->nscount; i++) {
      ListNode *node = list_node_ctor_with_info(
          authority[i], AUTHORITY, cur_time, min_u32(180, authority[i]->ttl)
      );

      if (authority[i]->type == 1)
        cache_insert(CACHE_TYPE_IPV4, question.qname, node);
      else if (authority[i]->type == 28)
        cache_insert(CACHE_TYPE_IPV6, question.qname, node);
      else if (authority[i]->type == 5)
        cache_insert(CACHE_TYPE_CNAME, question.qname, node);
    }

    for (int i = 0; i < header->arcount; i++) {
      ListNode *node = list_node_ctor_with_info(
          additional[i], ADDITIONAL, cur_time, min_u32(180, additional[i]->ttl)
      );

      if (additional[i]->type == 1)
        cache_insert(CACHE_TYPE_IPV4, question.qname, node);
      else if (additional[i]->type == 28)
        cache_insert(CACHE_TYPE_IPV6, question.qname, node);
      else if (additional[i]->type == 5)
        cache_insert(CACHE_TYPE_CNAME, question.qname, node);
    }
  }

  /// 获得 id 与检查有效性（未超时）
  u16 req_id = header->id;
  if (convert_table[req_id].valid == 0) {
    print_log(FAILURE, "Invalid response id: %d", req_id);
    return 0;
  }

  handle_send_response(
      req_id, header, &question, answer, authority, additional
  );

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

void event_loop() {
  u32 sock_len = sizeof(struct sockaddr_in);
  while (1) {
    ThreadArg *arg = (ThreadArg *)calloc(1, sizeof(ThreadArg));

    ssize_t recv_len = recvfrom(
        server_fd,
        arg->buf,
        BUFFER_LEN,
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
    // threadPoolAdd(pool, pthread_func, (void *)arg);
    pthread_create(&thread_id, NULL, pthread_func, (void *)arg);
  }
}
