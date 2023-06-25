#include "cli/parse.h"
#include "util/constant.h"
#include "util/ipv4addr.h"
#include "util/log.h"
#include "protocol/protocol.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <unistd.h> // for close
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <dispatch/dispatch.h>
#include <stdio.h> 



#define BUFF_LEN 1024
#define SERVER_PORT 53
#define MAP_LEN (0xffff + 2) // 0x10001

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
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
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
    printf("超时: %d\n", timerContext->buf_id);
#ifdef __APPLE__
    dispatch_semaphore_signal(convert_table[timerContext->buf_id].sem);
#else
    sem_post(&convert_table[req_id].sem);
#endif
    printf("定时器响应了一次\n");
}

struct sockaddr_in dns_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(53),
    .sin_addr.s_addr = 0x2d09030a
};

typedef struct ThreadArg {
    uint8_t buf[BUFF_LEN];
    struct sockaddr_in client_addr;
    ssize_t recv_len;
} ThreadArg;


void *pthread_func(void *arg_){
    ThreadArg *arg = (ThreadArg *)arg_;

    // 解析dns包头
    DnsHeader header;
    u8 *buf = arg->buf;
    u8 *data = arg->buf;
    parse_header(&buf, &header);

    // 如果是请求报文
    if (header.qr == 0) {
        /// 解析下游的请求报文
        DnsQuestion question;
        // TODO: 写个返回报错包的函数，如果 qdcount != 1 就返回报错包
        for (int i = 0; i < header.qdcount; i++) {
            parse_question(&buf, &question);
            debug(0, "qname: %s, qtype: %d, qclass: %d\n", question.qname, question.qtype, question.qclass);
        }

        /// TODO: 查找 cache，向下游发包

        /// 写转换表
        // 获得 id
        uint16_t req_id = header.id;
        convert_table[buf1_len].buf_req_id = req_id;
        convert_table[buf1_len].buf_sock = arg->client_addr;
        if (buf1_len >= MAP_LEN) {
            buf1_len = 1;
        }

        /// TODO？: 写 cache

        /// 写向上游的请求报文
        header.id = buf1_len++;
        u8 buf_relay_[BUFF_LEN];
        u8 *buf_relay = buf_relay_;
        // memcpy(relay_data, data, BUFF_LEN - sizeof(DnsHeader));
        dump_header(&buf_relay, header);
        dump_question(&buf_relay, &question);
        // TODO？: 解析获得包长度
        sendto(server_fd, buf_relay_, buf_relay - buf_relay_, 0, (struct sockaddr *)&dns_addr, sizeof(dns_addr));

        /// 等待上级dns回复或者超时
        convert_table[header.id].valid = 1;

        /// 创建定时器 dispatch timer
        printf("创建定时器\n");
        dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
        if (timer == NULL) {
            perror("dispatch_source_create");
            exit(1);
        }
        TimerContext *timerContext = (TimerContext *)malloc(sizeof(TimerContext));
        timerContext->buf_id = header.id;
        dispatch_set_context(timer, timerContext);
        dispatch_set_finalizer_f(timer, free);

        int64_t interval = 8 * NSEC_PER_SEC; // 8s, TODO: 写常量
        dispatch_time_t start_time = dispatch_time(DISPATCH_TIME_NOW, interval);
        dispatch_source_set_timer(timer, start_time, interval, 0);
        dispatch_source_set_event_handler_f(timer, timer_handler);
        dispatch_resume(timer);

        //等待超时或者收到上级dns的回复
#ifdef __APPLE__
        dispatch_semaphore_wait(convert_table[header.id].sem, DISPATCH_TIME_FOREVER);
#else
        sem_wait(&convert_table[header.id].sem);
#endif
        printf("释放\n");
        convert_table[header.id].valid = 0;
        dispatch_source_cancel(timer);
    } else if (header.qr == 1) {
        // TODO: 解析响应

        /// TODO: 写 cache

        /// TODO: 写向下游的响应报文
        /// 获得 id 与检查有效性（未超时）
        uint16_t req_id = header.id;
        if (convert_table[req_id].valid == 0) {
            printf("invalid response!\n");
            free(arg);
            return NULL;
        }

        // TODO: 检查 ttl 和有效性,这里默认转发表有效
        uint8_t buf_relay_[BUFF_LEN];
        u8 *buf_relay = buf_relay_;
        header.id = convert_table[req_id].buf_req_id;
        dump_header(&buf_relay, header);
        memcpy(buf_relay, data + sizeof(DnsHeader), BUFF_LEN - sizeof(DnsHeader));
        sendto(server_fd, buf_relay_, arg->recv_len, 0, (struct sockaddr *)&(convert_table[req_id].buf_sock), sizeof(convert_table[req_id].buf_sock));

        // 释放转发表
#ifdef __APPLE__
        dispatch_semaphore_signal(convert_table[req_id].sem);
#else
        sem_post(&convert_table[req_id].sem);
#endif
    }

    free(arg);
    return NULL;
}

int main(int argc, char const *argv[]){
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_fd < 0){
        printf("create socket failed!\n");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ret;
    ret = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0){
        printf("bind failed!\n");
        return -1;
    }

    
#ifdef __APPLE__
    for(int i = 0;i < MAP_LEN;i++){
        dispatch_semaphore_t *sem = &(convert_table[i].sem);
        *sem = dispatch_semaphore_create(0);

        sem_init(&(convert_table[i].sem), 0, 0);
    }

#else
    for(int i = 0;i < MAP_LEN;i++){
        sem_init(&(convert_table[i].sem), 0, 0);
    }
#endif


    unsigned sock_len = sizeof(struct sockaddr_in);
    while(1){

        // 注意free
        ThreadArg * arg = (ThreadArg *)malloc(sizeof(ThreadArg));
        memset(arg, 0, sizeof(ThreadArg));
        int recv_len = recvfrom(server_fd, arg->buf, BUFF_LEN, 0, (struct sockaddr*)&(arg->client_addr), &sock_len);

        if(recv_len < 0){
            printf("recvfrom failed!\n");
            return -1;
        }
//        for (int i = 0; i < recv_len; i++){
//            printf("%02x ", arg->buf[i]);
//        }
        arg->recv_len = recv_len;
        printf("recvfrom client port: %d\n", ntohs(arg->client_addr.sin_port));
        printf("recvfrom client addr: %s\n", inet_ntoa(arg->client_addr.sin_addr));
        // printf("recvfrom client data: %s\n", buf);
        pthread_t tid;
        pthread_create(&tid, NULL, pthread_func, (void*)arg);
    }

    return 0;
}
