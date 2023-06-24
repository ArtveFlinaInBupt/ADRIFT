#include "cli/parse.h"
#include "util/constant.h"
#include "util/ipv4addr.h"
#include "util/log.h"
#include "protocol/protocol.h"
#include <sys/socket.h>
#include <sys/types.h>  
#include <unistd.h> // for close
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFF_LEN 1024
#define SERVER_PORT 53


void *pthread_func(void *arg) {
  struct sockaddr_in* client_addr = (struct sockaddr_in*)arg;
  printf("client port: %d\n", ntohs((*client_addr).sin_port));
  printf("client addr: %s\n", inet_ntoa((*client_addr).sin_addr));
  printf("123");
  return NULL;
}

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




  // 建立socket
  int server_fd, ret;
  struct sockaddr_in ser_addr;

  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd < 0) {
    printf( "socket() failed");
    return -1;
  }

  memset(&ser_addr, 0, sizeof(ser_addr));
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  ser_addr.sin_port = htons(SERVER_PORT);

  ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));

  if (ret < 0) {
    printf("bind() failed");
    return -1;
  }


// 获取一个udp请求,如果是来自客户端的请求,则进行解析,得到请求的网址
// 如果网址在cache里面,就将结果返回
// 如果网站不在,就向上级服务器请求,维护一个表
// 如果udp来自上级服务器,就查表将结果返回给客户端

// 处理过程,这里等待改成多线程
  while (1) {
    char buf[BUFF_LEN];
    int socketLen;
    struct sockaddr_in client_addr;
    socketLen = sizeof(client_addr);
    // 阻塞式响应
    socketLen = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &socketLen);
    if (socketLen < 0) {
      printf("recvfrom() failed");
      return -1;
    }
    printf("receive from client: %s\n", buf);
    if (strcmp(buf, "exit") == 0) {
      break;
    }
    uint8_t *p = (uint8_t *)buf;
    printf("client port: %d\n", ntohs(client_addr.sin_port));
    printf("client addr: %s\n", inet_ntoa(client_addr.sin_addr));


    DnsHeader header;
    p = parse_header(buf, &header);
    // 拿到请求的类型，是查询报文还是响应报文
    uint8_t qr = header.qr;
    printf("qr: %d\n", qr);
    if(qr == 0){
      // query
      printf("query\n");
      // 查缓存操作

      
    }


    DnsQuestion question;
    // parse_question(p, &question);


    char response[1024];
    header.qr = 1;
    header.ancount = 1;
    memcpy(response, &header, sizeof(DnsHeader));
    DnsResourceRecord record;
    record.name = question.qname;
    record.type = 1;
    record.class = 1;
    record.ttl = 512;
    record.rdlength = 4;
    memcpy(response + sizeof(DnsHeader), &record, sizeof(DnsResourceRecord));

    char ip[4] = {'.','.','B','.'};
    record.RData = ip;


    pthread_t tid;
    ret = pthread_create(&tid, NULL, pthread_func, &client_addr);

    sendto(server_fd, response, socketLen, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

  }

// 不关闭好像也是没问题的,但是还是关闭一下.注意科学上网工具可能占用53劫持
  close(server_fd);
  return 0;
}
