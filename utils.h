#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <stdbool.h>

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

uint16_t compute_checksum(const ip_packet_t* ip_addr);
size_t to_buf(const ip_packet_t* packet, uint8_t* buf, size_t buf_len);
bool to_packet(const uint8_t* buf, size_t len, ip_packet_t* out_packet);

#endif // UTILS_H