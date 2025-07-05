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

#endif // UTILS_H