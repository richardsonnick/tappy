#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"

#include "client.h"

void client_loop(int netdev_fd) {
    printf("Running client...\n");
    ip_packet_t packet = {
        .version = 4,
        .ihl = 5,
        .type_of_service = 0x66,
        .total_length = 20,
        .identification = 0x7777,
        .flags = 0x00,
        .fragment_offset = 0x0000,
        .time_to_live = 0xFF,
        .protocol = 0x88,
        .header_checksum = 0x9898,
        .source_address = to_ip_encoding_decomposed(127,0,0,1),
        .destination_address = to_ip_encoding_decomposed(127,0,0,1),
    };
    const uint8_t buf_len = 20;
    uint8_t buf[buf_len];
    to_buf(&packet, buf, buf_len);
    for (int j = 0; j < buf_len; ++j) {
        printf("%02x ", buf[j]);
    }
    printf("\n");
    // return;
    while (1) {
    for (int j = 0; j < buf_len; ++j) {
        printf("%02x ", buf[j]);
    }
    printf("\n");
        ssize_t written = write(netdev_fd, buf, buf_len);
        if (written != buf_len) {
            perror("write");
        }
    }
}