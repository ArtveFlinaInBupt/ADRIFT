#pragma once

#ifndef ADRIFT_PROTOCOL_H
#  define ADRIFT_PROTOCOL_H

#  include "util/type.h"

typedef enum ReplyCode {
  NO_ERROR = 0,
  FORMAT_ERROR,
  SERVER_FAILURE,
  NAME_ERROR,
  NOT_IMPLEMENTED,
  REFUSED,
} ReplyCode;

typedef enum RrType {
  ANSWER = 1,
  AUTHORITY,
  ADDITIONAL,
} RrType;

typedef struct __attribute__((packed)) DnsHeader {
  u16 id; // identification number

  u8 rd : 1;     // recursion desired
  u8 tc : 1;     // truncated message
  u8 aa : 1;     // authoritive answer
  u8 opcode : 4; // purpose of message
  u8 qr : 1;     // query/response flag

  u8 rcode : 4; // response code
  u8 cd : 1;    // checking disabled
  u8 ad : 1;    // authenticated data
  u8 z : 1;     // its z! reserved
  u8 ra : 1;    // recursion available

  u16 qdcount; // number of question entries
  u16 ancount; // number of answer entries
  u16 nscount; // number of authority entries
  u16 arcount; // number of resource entries
} DnsHeader;

typedef struct DnsQuestion { // TODO: need it be packed?
  u8 *qname;                 // domain name
  u16 qtype;                 // type of record
  u16 qclass;                // class record
} DnsQuestion;

typedef struct DnsResourceRecord { // TODO: need it be packed?
  u8 *name;                        // domain name
  u16 type;                        // type of record
  u16 class;                       // class record
  u32 ttl;                         // time to live
  u16 rdlength;                    // length of rdata
  u8 *rdata;                       // resource data
} DnsResourceRecord;

void parse_header(u8 **buf, DnsHeader *header);

void dump_header(u8 **buf, DnsHeader header);

DnsHeader get_error_header(int rcode);

DnsHeader get_default_header(void);

/// @brief Get the length of a qname.
/// @param buf The buffer to get the length from.
/// @return The length of the qname.
size_t get_qname_length(const u8 *buf);

/// @brief Parse a qname from a buffer.
/// @param buf The buffer to parse from.
/// @param output The output buffer.
/// @details The output buffer must be freed by the caller.
void parse_qname(u8 **buf, u8 **output);

void parse_question(u8 **buf, DnsQuestion *question);

void dump_question(u8 **buf, DnsQuestion *question);

void destroy_question(DnsQuestion *question);

void parse_resource_record(u8 **buf, u8 *name_buf, DnsResourceRecord *record);

void dump_resource_record(u8 **buf, DnsResourceRecord *record);

void dump_error_authority(u8 **buf, DnsQuestion *question);

void destroy_resource_record(DnsResourceRecord *record);

#endif // ADRIFT_PROTOCOL_H
