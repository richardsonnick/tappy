#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"
#include "tcp.h"
#include "string.h"

#include "client.h"

void client_loop(int netdev_fd) {
    printf("Running client...\n");
    const char* data = "hello world";
    size_t data_len = strlen(data);
    ip_addr_t source_ip = {127, 0, 0, 1};
    ip_addr_t destination_ip = {127, 0, 0, 1};
    uint8_t* tcp_buf = NULL;
    size_t buf_len = 0;

    tcp_buf = data_to_tcp_buf(data, strlen(data), 
                                    &source_ip, &destination_ip,
                                    4, 5, &buf_len);
    if (tcp_buf == NULL) {
        fprintf(stderr, "Failed to build TCP packet.\n");
        return;
    }

    while (1) {
        print_raw_buf(tcp_buf, buf_len);
        ssize_t written = write(netdev_fd, tcp_buf, buf_len);
        if (written != buf_len) {
            perror("write");
        }
        sleep(2);
    }
}