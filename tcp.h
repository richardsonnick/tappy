#ifndef TCP_H
#define TCP_H

#include "types.h"

uint8_t* data_to_tcp_buf(const uint8_t* data, const size_t data_len, 
    const ip_addr_t* source_ip,
    const ip_addr_t* destination_ip,
    const uint16_t source_port,
    const uint16_t destination_port,
    size_t* out_total_packet_len);

uint16_t compute_ip_checksum(const ip_header_t* header);
uint16_t compute_tcp_packet_checksum(const tcp_packet_t* packet,
                                    uint32_t src_ip, uint32_t dst_ip,
                                    uint16_t tcp_length);
size_t ip_header_to_buf(const ip_header_t* packet, uint8_t* buf, size_t buf_len);
size_t tcp_packet_to_buf(const tcp_packet_t* packet, uint8_t* buf, size_t buf_len);
bool ip_buf_to_packet(const uint8_t* buf, size_t len, ip_header_t* out_packet);

#endif