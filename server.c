#include <stdio.h>
#include <unistd.h>
#include "server.h"
#include "io.h"

void server_loop(int netdev_fd) {
    printf("Running server...\n");
    uint8_t tcp_buf[64];
    while (1) {
        read_from_dev(netdev_fd, tcp_buf, 64);
        sleep(2);
    }
}