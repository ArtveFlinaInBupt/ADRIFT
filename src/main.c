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

#define BUFF_LEN 1024
#define SERVER_PORT 53
#define MAP_LEN 0xffff

// 获取一个udp请求,如果是来自客户端的请求,则进行解析,得到请求的网址
// 如果网址在cache里面,就将结果返回
// 如果网站不在,就向上级服务器请求,维护一个转发表
// 如果udp来自上级服务器,就查表将结果返回给客户端

int server_fd;
struct sockaddr_in server_addr;

const struct sockaddr_in dns_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(53),
    .sin_addr.s_addr = 0x2c09030a
};

typedef struct {
    uint8_t buf[BUFF_LEN];
    struct sockaddr_in client_addr;
    int recv_len;
}thread_arg;

// TODO: 改掉临时的转发表,加锁.
uint16_t buf_req_id[MAP_LEN];
struct sockaddr_in buf_sock[MAP_LEN];
int buf1_len = 1;

void *pthread_func(void *a){
    thread_arg* arg = (thread_arg*)a;

    // 解析dns包头
    DnsHeader header;
    u8 *buf = arg->buf;
    u8 *data = arg->buf;
    parse_header(&buf, &header);

    // 如果是请求报文
    if(header.qr == 0){
        // 解析查询
        // TODO: 解析查询的内容
        DnsQuestion *questions = malloc(sizeof(DnsQuestion) * header.qdcount);
        for (int i = 0; i < header.qdcount; i++) {
            parse_question(&buf, &questions[i]);
            debug(0, "qname: %s, qtype: %d, qclass: %d\n", questions[i].qname, questions[i].qtype, questions[i].qclass);
        }

        // TODO: 对网址查找缓存,或者host文件,此处先假装查了没查到

        // 获得id
        uint16_t req_id = header.id;
        // 缓存req,这里的缓存都是指的转发表
        // TODO: 别用这个弱智缓存
        buf_req_id[buf1_len] = req_id;
        buf_sock[buf1_len] = arg->client_addr;
        if(buf1_len >= MAP_LEN){
            buf1_len = 1;
        }

        // 查询上级DNS
        header.id = buf1_len++;
        u8 buf_relay_[BUFF_LEN];
        u8 *buf_relay = buf_relay_;
        dump_header(&buf_relay, header);
        for (int i = 0; i < header.qdcount; i++) {
            dump_question(&buf_relay, &questions[i]);
        }
        // TODO: 解析获得包长度
//        memcpy(relay_data, data, BUFF_LEN - sizeof(DnsHeader));
        sendto(server_fd, buf_relay_, buf_relay - buf_relay_, 0, (struct sockaddr *)&dns_addr, sizeof(dns_addr));

        free(questions);
    }
    else if(header.qr == 1){
        // TODO: 解析响应,加入cache

        // 获得id
        uint16_t req_id = header.id;

       // TODO: 检查ttl和有效性,这里默认转发表有效
        uint8_t buf_relay_[BUFF_LEN];
        u8 *buf_relay = buf_relay_;
        header.id = buf_req_id[req_id];
        dump_header(&buf_relay, header);
        memcpy(buf_relay, data + sizeof(DnsHeader), BUFF_LEN - sizeof(DnsHeader));
        sendto(server_fd, buf_relay_, arg->recv_len, 0, (struct sockaddr*)&(buf_sock[req_id]), sizeof(buf_sock[req_id]));

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


    unsigned sock_len = sizeof(struct sockaddr_in);
    while(1){

        // 注意free
        thread_arg* arg = (thread_arg*)malloc(sizeof(thread_arg));
        memset(arg, 0, sizeof(thread_arg));
        int recv_len = recvfrom(server_fd, arg->buf, BUFF_LEN, 0, (struct sockaddr*)&(arg->client_addr), &sock_len);
        if(recv_len < 0){
            printf("recvfrom failed!\n");
            return -1;
        }
        arg->recv_len = recv_len;
        printf("recvfrom client port: %d\n", ntohs(arg->client_addr.sin_port));
        printf("recvfrom client addr: %s\n", inet_ntoa(arg->client_addr.sin_addr));
        // printf("recvfrom client data: %s\n", buf);
        pthread_t tid;
        pthread_create(&tid, NULL, pthread_func, (void*)arg);
    }

    return 0;
}
