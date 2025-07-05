#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <stdbool.h>

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

uint16_t compute_ip_checksum(const ip_header_t* header);
uint16_t compute_tcp_packet_checksum(const tcp_packet_t* packet,
                                    uint32_t src_ip, uint32_t dst_ip,
                                    uint16_t tcp_length);
size_t ip_header_to_buf(const ip_header_t* packet, uint8_t* buf, size_t buf_len);
size_t tcp_packet_to_buf(const tcp_packet_t* packet, uint8_t* buf, size_t buf_len);
bool ip_buf_to_packet(const uint8_t* buf, size_t len, ip_header_t* out_packet);

#endif // UTILS_H