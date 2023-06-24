#include "protocol.h"

#include "util/type.h"

#include <stdlib.h>
#include <string.h>

size_t get_qname_length(const u8 *buf) {
  size_t len = 0;
  while (*buf != 0) {
    len += *buf + 1;
    buf += *buf + 1;
  }
  return len + 2;
}

void parse_qname(u8 **buf, u8 **output) {
  size_t len = get_qname_length(*buf);
  *output = malloc(len);
  u8 *p = *output;
  u8 l;

  while ((l = **buf) != 0) {
    (*buf)++;
    for (int i = 0; i < l; i++) {
      *p++ = **buf;
      (*buf)++;
    }
    *p++ = '.';
  }
  *p = '\0';
  (*buf)++;
}

uint8_t* parse_header(uint8_t *buf, DnsHeader *header) {
  memcpy(header, buf, sizeof(DnsHeader));
  header->id = ntohs(header->id);
  header->qdcount = ntohs(header->qdcount);
  header->ancount = ntohs(header->ancount);
  header->nscount = ntohs(header->nscount);
  header->arcount = ntohs(header->arcount);
  *buf += sizeof(DnsHeader);
  return buf + sizeof(DnsHeader);
}

uint8_t* dump_header(uint8_t *buf, DnsHeader header) {
  header.id = htons(header.id);
  header.qdcount = htons(header.qdcount);
  header.ancount = htons(header.ancount);
  header.nscount = htons(header.nscount);
  header.arcount = htons(header.arcount);
  memcpy(buf, &header, sizeof(DnsHeader));
  return buf + sizeof(DnsHeader);
}

void parse_question(uint8_t **buf, DnsQuestion *question) {
    parse_qname(buf, &question->qname);
    memcpy(&question->qtype, *buf, 2);
    question->qtype = ntohs(question->qtype);
    *buf += 2;
    memcpy(&question->qclass, *buf, 2);
    question->qclass = ntohs(question->qclass);
    *buf += 2;
}
