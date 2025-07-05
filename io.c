#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"
#include "tcp.h"
#include "string.h"

#include "client.h"

#include "utils.h"
#include "io.h"

void write_to_dev(int netdev_fd, uint8_t* buf, size_t buf_len) {
    // TODO: gate this with debug
    print_raw_buf(buf, buf_len);
    ssize_t written = write(netdev_fd, buf, buf_len);
    if (written != buf_len) {
        perror("write");
    }
}

void read_from_dev(int netdev_fd, uint8_t* buf, size_t buf_len) {
    ssize_t read_bytes = read(netdev_fd, buf, buf_len);
    if (read_bytes < 0) {
        perror("read");
        return;
    }
    // TODO check if tcp packets
    if (read_bytes >= 20) {
        uint8_t version = (buf[0] >> 4) & 0x0F;
        if (version == 4) {
            uint8_t protocol = buf[9];
            if (protocol == 0x06) { // TCP
                printf("Got TCP packet...\n");
                print_raw_buf(buf, (size_t) read_bytes);
            } else {
                printf("Non-TCP IPv4 protocol: %u\n", protocol);
            }
        } else {
            printf("Got IPv6. Skipping\n");
        }
    }
}