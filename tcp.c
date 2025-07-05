#include "types.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "utils.h"

#include "tcp.h"

uint8_t* data_to_tcp_buf(const uint8_t* data, const size_t data_len, 
    const ip_addr_t* source_ip,
    const ip_addr_t* destination_ip,
    const uint16_t source_port,
    const uint16_t destination_port,
    size_t* out_total_packet_len) {
        const size_t total_tcp_segment_len = MIN_TCP_PACKET_SIZE + data_len;
        const size_t total_packet_len = MIN_IP4_HEADER_SIZE + total_tcp_segment_len;

        uint8_t* packet_buf = (uint8_t*)malloc(total_packet_len);
        if (packet_buf == NULL) {
            perror("Failed to allocate packet_buf");
            return NULL;
        }

        uint32_t src_ip_encoded = to_ip_encoding(source_ip);
        uint32_t dst_ip_encoded = to_ip_encoding(destination_ip);

        ip_header_t ip_header = {
            .version = 4,
            .ihl = 5, // header length in 32 bit (4 bytes) words i.e. ihl == 5 implies (5 * 4) 20 byte header
            .type_of_service = 0x66,
            .total_length = (uint16_t)total_packet_len, // total length of the ip header + payload (tcp packets etc).
            .identification = 0x7777,
            .flags = 0x00,
            .fragment_offset = 0x0000,
            .time_to_live = 69,
            .protocol = 0x06, // TCP
            .header_checksum = 0x00,
            .source_address = src_ip_encoded,
            .destination_address = dst_ip_encoded,
        };

        ip_header.header_checksum = compute_ip_checksum(&ip_header);
        ip_header_to_buf(&ip_header, packet_buf, MIN_IP4_HEADER_SIZE);

        tcp_packet_t tcp_packet = {
            .source_port = source_port,
            .destination_port = destination_port,
            .sequence_number = 0x00,
            .acknowledgment_number = 0x00,
            .data_offset = (MIN_TCP_PACKET_SIZE / 4), // data offset in 4 byte words (words == 32 bits)
            .reserved = 0x00,
            .flags = 0x00,
            .window = 0x00,
            .checksum = 0x00,
            .urgent_pointer = 0x00,
            .data = data,
            .data_len = data_len
        };
        tcp_packet.checksum = compute_tcp_packet_checksum(&tcp_packet, 
            ip_header.source_address, 
            ip_header.destination_address, 
            (uint16_t)total_tcp_segment_len);
        
        size_t bytes_written_tcp = tcp_packet_to_buf(
            &tcp_packet,
            packet_buf + MIN_IP4_HEADER_SIZE,
            total_tcp_segment_len
        );

        if (bytes_written_tcp == 0) {
            free(packet_buf);
            return NULL;
        }

        if (out_total_packet_len) {
            *out_total_packet_len = total_packet_len;
        }

        return packet_buf;
    }

/***
 * The checksum field is the 16-bit ones' complement of the ones' complement sum of all 16-bit words in the header and text.
 * The checksum computation needs to ensure the 16-bit alignment of the data being summed.
 * If a segment contains an odd number of header and text octets, alignment can be achieved by padding the last octet with zeros on its right to form a 16-bit word for checksum purposes. 
 * The pad is not transmitted as part of the segment. While computing the checksum, the checksum field itself is replaced with zeros.
 * 
 * The checksum also covers a pseudo-header (Figure 2) conceptually prefixed to the TCP header.
 * The pseudo-header is 96 bits for IPv4 and 320 bits for IPv6.
 * Including the pseudo-header in the checksum gives the TCP connection protection against misrouted segments.
 * This information is carried in IP headers and is transferred across the TCP/network interface in the arguments or results of calls by the TCP implementation on the IP layer.
 * https://datatracker.ietf.org/doc/html/rfc9293#section-3.1
 */
uint16_t compute_tcp_packet_checksum(const tcp_packet_t* packet,
                                    uint32_t src_ip, uint32_t dst_ip,
                                    uint16_t tcp_length) {
    tcp_packet_t temp_packet = *packet; // ensure checksum is set to zero 
    temp_packet.checksum = 0; 

    const uint8_t tcp_header_len = packet->data_offset * 4;
    const uint8_t total_tcp_len = tcp_header_len + packet->data_len;

    size_t checksum_buf_len = total_tcp_len;
    if (checksum_buf_len % 2 != 0) {
        checksum_buf_len++; // ensure checksum buf is even
    }
    uint8_t* checksum_buf = (uint8_t*)calloc(1, checksum_buf_len);
    size_t bytes_written = tcp_packet_to_buf(&temp_packet, checksum_buf, checksum_buf_len);
    if (bytes_written == 0) {
        free(checksum_buf);
        return 0;
    }

    uint32_t sum = 0;

    sum += (src_ip >> 16) & 0xFFFF;
    sum += (src_ip) & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += (dst_ip) & 0xFFFF;
    sum += 6; // TCP protocol
    sum += tcp_length;

    for (int i = 0; i < total_tcp_len; i += 2) {
        uint16_t word;
        if (i + 1 < total_tcp_len) {
            word = (checksum_buf[i] << 8) | checksum_buf[i + 1];
        } else {
            word = checksum_buf[i] << 8;
        }
        sum += word;
    }

    // carry bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    free(checksum_buf);
    return (uint16_t)(~sum);
}

/**
 * "The checksum field is the 16 bit one's complement of the one's
 *    complement sum of all 16 bit words in the header.  For purposes of
 *    computing the checksum, the value of the checksum field is zero."
 *  https://datatracker.ietf.org/doc/html/rfc791#section-3.1
 */
uint16_t compute_ip_checksum(const ip_header_t* packet) {
    const uint8_t header_len = packet->ihl * 4;
    uint8_t header[header_len];

    ip_header_t temp_packet = *packet;
    temp_packet.header_checksum = 0; // ensure the checksum is set to zero

    ip_header_to_buf(&temp_packet, header, header_len);

    uint32_t sum = 0;

    // Process as 16 bit words
    for (int i = 0; i < header_len; i += 2) {
        uint16_t word = (header[i] << 8) | header[i + 1];
        sum += word;
    }

    // carry bits
    // Takes bits added that exceed the 16 bit final width
    // and adds them back to the lower 16 bits.
    // This is necessary for the "one's complement sum"
    // https://datatracker.ietf.org/doc/html/rfc1071#section-1
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

size_t ip_header_to_buf(const ip_header_t* packet, uint8_t* buf, size_t buf_len) {
    if (buf_len < MIN_IP4_HEADER_SIZE) {
        return -1;
    }

    uint8_t i = 0;
    buf[i++] = ((packet->version & 0x0f) << 4) | (packet->ihl & 0x0F);
    buf[i++] = packet->type_of_service;

    uint16_t total_length = packet->total_length;
    buf[i++] = (total_length >> 8);
    buf[i++] = (total_length & 0xFF);

    uint16_t identification = packet->identification;
    buf[i++] = (identification >> 8);
    buf[i++] = (identification & 0xFF); 

    uint16_t flags_fragment = ((packet-> flags & 0x7) << 13) | (packet->fragment_offset & 0x1FFF);
    flags_fragment = flags_fragment;
    buf[i++] = flags_fragment >> 8;
    buf[i++] = flags_fragment & 0xFF;

    buf[i++] = packet->time_to_live;
    buf[i++] = packet->protocol;

    uint16_t header_checksum = packet->header_checksum;
    buf[i++] = (header_checksum >> 8);
    buf[i++] = (header_checksum & 0xFF); 

    uint32_t source_address = packet->source_address;
    buf[i++] = (source_address >> 24);
    buf[i++] = (source_address >> 16) & 0xFF; 
    buf[i++] = (source_address >> 8) & 0xFF; 
    buf[i++] = (source_address & 0x00FF); 

    uint32_t destination_address = packet->destination_address;
    buf[i++] = (destination_address >> 24);
    buf[i++] = (destination_address >> 16) & 0xFF; 
    buf[i++] = (destination_address >> 8) & 0xFF; 
    buf[i++] = (destination_address & 0x00FF); 

    return i;
}

bool ip_buf_to_packet(const uint8_t* buf, size_t len, ip_header_t* out_packet) {
    if (len < MIN_IP4_HEADER_SIZE) {
        return false;
    }
    uint8_t i = 0;
    out_packet->version = (buf[i] >> 4); // version
    out_packet->ihl = (buf[i] & 0x0F); // ihl
    i++;
    out_packet->type_of_service = buf[i++];
    out_packet->total_length = ((uint16_t)buf[i++] << 8); 
    out_packet->total_length = (((uint16_t)buf[i++] & 0x00FF) | out_packet->total_length); 
    out_packet->identification = ((uint16_t)buf[i++] << 8); 
    out_packet->identification = (((uint16_t)buf[i++] & 0x00FF) | out_packet->identification); 

    uint8_t flag_fragment1 = buf[i++];
    uint8_t flag_fragment2 = buf[i++];
    out_packet->flags = (((uint8_t) flag_fragment1 >> 5));
    out_packet->fragment_offset = (((uint16_t) flag_fragment1 & 0x1F) << 8)
                                | ((uint16_t) flag_fragment2);

    out_packet->time_to_live = buf[i++];
    out_packet->protocol = buf[i++];
    out_packet->header_checksum = ((uint16_t)buf[i++] << 8); 
    out_packet->header_checksum = (((uint16_t)buf[i++] & 0x00FF) | out_packet->header_checksum); 
    out_packet->source_address = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    out_packet->destination_address = (((uint32_t) buf[i++]) << 24)
                                | ((uint32_t) buf[i++] << 16)
                                | ((uint32_t) buf[i++] << 8)
                                | ((uint32_t) buf[i++]);
    return true;
}

size_t tcp_packet_to_buf(const tcp_packet_t* packet, uint8_t* buf, size_t buf_len) {
    size_t required_len = (packet->data_offset * 4) + packet->data_len;
    if (buf_len < required_len) {
        fprintf(stderr, "Buffer too small for tcp packet");
        return 0;
    }

    uint8_t i = 0;
    uint16_t source_port = packet->source_port;
    buf[i++] = ((source_port >> 8) & 0xFF); 
    buf[i++] = (source_port & 0xFF);

    uint16_t destination_port = packet->destination_port;
    buf[i++] = ((destination_port >> 8) & 0xFF); 
    buf[i++] = (destination_port & 0xFF);

    uint32_t sequence_number = packet->sequence_number;
    buf[i++] = (sequence_number >> 24);
    buf[i++] = (sequence_number >> 16) & 0xFF; 
    buf[i++] = (sequence_number >> 8) & 0xFF; 
    buf[i++] = (sequence_number & 0x00FF); 

    uint32_t acknowledgment_number = packet->acknowledgment_number;
    buf[i++] = (acknowledgment_number >> 24);
    buf[i++] = (acknowledgment_number >> 16) & 0xFF; 
    buf[i++] = (acknowledgment_number >> 8) & 0xFF; 
    buf[i++] = (acknowledgment_number & 0x00FF); 

    buf[i++] = ((packet->data_offset & 0x0F) << 4) | (packet->reserved & 0x0F);

    buf[i++] = packet->flags;

    uint16_t window = packet->window;
    buf[i++] = (window >> 8);
    buf[i++] = (window & 0xFF); 

    uint16_t checksum = packet->checksum;
    buf[i++] = (checksum >> 8);
    buf[i++] = (checksum & 0xFF); 

    uint16_t urgent_pointer = packet->urgent_pointer;
    buf[i++] = (urgent_pointer >> 8);
    buf[i++] = (urgent_pointer & 0xFF); 

    //...options

    if (packet->data && packet->data_len > 0) {
        memcpy(buf + i, packet->data, packet->data_len);
        i += packet->data_len;
    }
    return i;
}