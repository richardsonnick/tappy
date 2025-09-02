#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "state_machine.h"
#include <stdbool.h>

void print_ip_header(const ip_header_t* ip_header);
void print_raw_buf(const uint8_t* buf, const size_t buf_len);

/**
 * Allocates a tun driver
 * https://www.kernel.org/doc/Documentation/networking/tuntap.txt
 */
int tun_alloc(char *dev);

uint32_t to_ip_encoding_decomposed(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d);
/**
 * Returns ip_addr as a uint32_t in big-endian (network endianness)
 */
uint32_t to_ip_encoding(const ip_addr_t* ip_addr);

ip_addr_t from_ip_encoding(const uint32_t ip);
void print_tcp_packet(const tcp_packet_t* packet);
const char* tcp_state_to_string(TCP_STATE state);
void print_conn(const tcp_connection_t* conn, const char* context);

void write_u32_be(uint8_t* buf, uint32_t val);
void write_u16_be(uint8_t* buf, uint32_t val);
uint32_t read_u32_be(const uint8_t* buf);
uint16_t read_u16_be(const uint8_t* buf);

#endif // UTILS_H