#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
// https://datatracker.ietf.org/doc/html/rfc791#section-3.1
typedef struct {
    uint8_t version;  // 4 bits
    uint8_t ihl;  // 4 bits
    uint8_t type_of_service; // 8 bits
    uint16_t total_length; // 16 bits
    uint16_t identification; // 16 bits
    uint8_t flags; // 3 bits
    uint16_t fragment_offset; // 13 bits
    uint8_t time_to_live; // 8 bits
    uint8_t protocol; // 8 bits
    uint16_t header_checksum; // 16 bits
    uint32_t source_address; // 32 bits
    uint32_t destination_address; // 32 bits
    // ...options
    // ...padding
} ip_packet_t;

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} ip_addr_t;

#endif