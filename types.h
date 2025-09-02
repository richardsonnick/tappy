#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

#define MIN_IP4_HEADER_SIZE 20
#define MIN_IP6_HEADER_SIZE 40

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
} ip_header_t;

#define MIN_TCP_PACKET_SIZE 20

// https://datatracker.ietf.org/doc/html/rfc9293#section-3.1
typedef struct {
    uint16_t source_port; // 16 bits
    uint16_t destination_port; // 16 bits
    uint32_t sequence_number; // 32 bits
    uint32_t acknowledgment_number; // 32 bits
    uint8_t data_offset; // 4 bits
    uint8_t reserved; // 4 bits
    uint8_t flags; // aka "control bits". 8 bits
    uint16_t window; // 16 bits
    uint16_t checksum; // 16 bits
    uint16_t urgent_pointer; // 16 bits
    // options...
    uint8_t* data;
    size_t data_len;
} tcp_packet_t;

typedef struct {
    ip_header_t* ip_header;
    tcp_packet_t* tcp_packet;
} tcp_ip_t;

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
} ip_addr_t;

typedef enum {
    TCP_FLAG_NONE = 0x00,
    TCP_FLAG_FIN = 0x01,
    TCP_FLAG_SYN = 0x02,
    TCP_FLAG_RST = 0x04,
    TCP_FLAG_PSH = 0x08,
    TCP_FLAG_ACK = 0x10,
    TCP_FLAG_URG = 0x20,
    TCP_FLAG_ECE = 0x40,
    TCP_FLAG_CWR = 0x80
} tcp_flag_bits_t;

#endif