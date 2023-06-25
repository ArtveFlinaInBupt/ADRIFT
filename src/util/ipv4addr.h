#pragma once

#ifndef ADRIFT_UTIL_IPV4ADDR_H
#  define ADRIFT_UTIL_IPV4ADDR_H

#  include "util/log.h"
#  include "util/type.h"

#  include <arpa/inet.h>

typedef u32 Ipv4Addr;

static inline Ipv4Addr ipv4_addr_from_str(const char *str) {
  Ipv4Addr addr;
  if (inet_pton(AF_INET, str, &addr) != 1) {
    debug(2, "Invalid IPv4 address occurred: %s", str);
    addr = -1;
  }
  return addr;
}

/// @brief Get the octet of an IPv4 address.
/// @param addr The IPv4 address.
/// @param octet The octet to get.
/// @return The octet of the IPv4 address.
/// @details The octet is zero-indexed, possible values are 0~3.
/// @example ipv4_addr_to_octet(0x01020304, 0) == 0x04
/// @example ipv4_addr_to_octet(0x01020304, 3) == 0x01
static inline u8 ipv4_addr_to_octet(Ipv4Addr addr, u8 octet) {
  return (addr >> (octet << 3)) & 0xff;
}

/// @brief Convert an IPv4 address to a string.
/// @param addr The IPv4 address.
/// @param str The string to store the result.
/// @details The string must be at least INET_ADDRSTRLEN bytes long.
/// @example ipv4_addr_to_str(0x01020304, str) == "4.3.2.1"
static inline void ipv4_addr_to_str(Ipv4Addr addr, char *str) {
  inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN);
}

#endif // ADRIFT_UTIL_IPV4ADDR_H
