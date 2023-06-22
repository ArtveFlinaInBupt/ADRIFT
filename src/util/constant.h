#pragma once

#ifndef ADRIFT_UTIL_CONSTANT_H
#  define ADRIFT_UTIL_CONSTANT_H

#define ADRIFT_VERSION "0.0.1"
#define ADRIFT_NAME "adrift"

#define DEFAULT_HOSTS_FILE  "./dnsrelay.txt"
#define DEFAULT_DNS_SERVER "192.168.0.1"
#define DEFAULT_DEBUG_LEVEL 0

#define PORT 53
#define IPV4_ADDR_BUF_SIZE 16 // equals to INET_ADDRSTRLEN
#define IPV6_ADDR_BUF_SIZE 46 // equals to INET6_ADDRSTRLEN
#define URL_BUF_SIZE (128 | 1)

#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME 0x100000001b3

#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define GREY "\033[90m"
#define RESET "\033[0m"

#endif // ADRIFT_UTIL_CONSTANT_H
