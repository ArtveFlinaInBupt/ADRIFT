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
  *(p - 1) = '\0';
  (*buf)++;
}

void dump_qname(u8 **buf, u8 *qname) {
  u8 *p = *buf;
  u8 *q = qname;
  while (*qname != '\0') {
    u8 len = 0;
    while (*qname != '.' && *qname != '\0') {
      len++;
      qname++;
    }
    *p++ = len;
    memcpy(p, q, len);
    p += len;
    q += len + 1;
    if (*qname == '.') {
      qname++;
    }
  }
  *p++ = '\0';
  *buf = p;
}

void parse_header(u8 **buf, DnsHeader *header) {
  memcpy(header, *buf, sizeof(DnsHeader));
  header->id = ntohs(header->id);
  header->qdcount = ntohs(header->qdcount);
  header->ancount = ntohs(header->ancount);
  header->nscount = ntohs(header->nscount);
  header->arcount = ntohs(header->arcount);
  *buf += sizeof(DnsHeader);
}

void dump_header(u8 **buf, DnsHeader header) {
  header.id = htons(header.id);
  header.qdcount = htons(header.qdcount);
  header.ancount = htons(header.ancount);
  header.nscount = htons(header.nscount);
  header.arcount = htons(header.arcount);
  memcpy(*buf, &header, sizeof(DnsHeader));
  *buf += sizeof(DnsHeader);
}

void parse_question(u8 **buf, DnsQuestion *question) {
  parse_qname(buf, &question->qname);
  memcpy(&question->qtype, *buf, 2);
  question->qtype = ntohs(question->qtype);
  *buf += 2;
  memcpy(&question->qclass, *buf, 2);
  question->qclass = ntohs(question->qclass);
  *buf += 2;
}

void dump_question(u8 **buf, DnsQuestion *question) {
  dump_qname(buf, question->qname);
  question->qtype = htons(question->qtype);
  memcpy(*buf, &question->qtype, 2);
  *buf += 2;
  question->qclass = htons(question->qclass);
  memcpy(*buf, &question->qclass, 2);
  *buf += 2;
}

void destroy_question(DnsQuestion *question) {
  free(question->qname);
}

void parse_resource_record(u8 **buf, DnsResourceRecord *record) {
  parse_qname(buf, &record->name);
  memcpy(&record->type, *buf, 2);
  record->type = ntohs(record->type);
  *buf += 2;
  memcpy(&record->class, *buf, 2);
  record->class = ntohs(record->class);
  *buf += 2;
  memcpy(&record->ttl, *buf, 4);
  record->ttl = ntohl(record->ttl);
  *buf += 4;
  memcpy(&record->rdlength, *buf, 2);
  record->rdlength = ntohs(record->rdlength);
  *buf += 2;
  record->rdata = malloc(record->rdlength);
  memcpy(record->rdata, *buf, record->rdlength);
  *buf += record->rdlength;
}

void destroy_resource_record(DnsResourceRecord *record) {
  free(record->name);
  free(record->rdata);
}
