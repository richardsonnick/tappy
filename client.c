#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"

#include "client.h"

void client_loop(int netdev_fd) {
    printf("Running client...\n");
    ip_header_t ip_header = {
        .version = 4,
        .ihl = 5, // header length in 32 bit (4 bytes) words i.e. ihl == 5 implies (5 * 4) 20 byte header
        .type_of_service = 0x66,
        .total_length = 40, // total length of the ip header + payload (tcp packets etc).
        .identification = 0x7777,
        .flags = 0x00,
        .fragment_offset = 0x0000,
        .time_to_live = 0xFF,
        .protocol = 0x06, // TCP
        .header_checksum = 0x00,
        .source_address = to_ip_encoding_decomposed(127,0,0,1),
        .destination_address = to_ip_encoding_decomposed(127,0,0,1),
    };
    ip_header.header_checksum = compute_checksum(&ip_header);
    const uint8_t buf_len = MIN_IP4_HEADER_SIZE + MIN_TCP_PACKET_SIZE;
    uint8_t buf[buf_len];
    ip_header_to_buf(&ip_header, buf, MIN_IP4_HEADER_SIZE);

    tcp_packet_t tcp_packet = {
        .source_port = 0x04,
        .destination_port = 0x05,
        .sequence_number = 0x00,
        .acknowledgment_number = 0x00,
        .data_offset = 0x05,
        .reserved = 0x00,
        .flags = 0x00,
        .window = 0x00,
        .checksum = 0x00,
        .urgent_pointer = 0x00
    };
    tcp_packet_to_buf(&tcp_packet, buf + MIN_IP4_HEADER_SIZE, MIN_TCP_PACKET_SIZE);
    while (1) {
        print_raw_buf(buf, buf_len);
        ssize_t written = write(netdev_fd, buf, buf_len);
        if (written != buf_len) {
            perror("write");
        }
        sleep(2);
    }
}