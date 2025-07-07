#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"
#include "tcp.h"
#include "string.h"
#include "io.h"
#include "client_state_machine.h"

#include "client.h"

void send_syn(int netdev_fd) {
    printf("Sending syn... ");
    ip_addr_t source_ip = {192, 168, 1, 246};
    ip_addr_t destination_ip = {192, 168, 1, 246};
    tcp_connection_t* conn = init_tcp_stack(&source_ip, &destination_ip,
                8080,  //source port
                8081, // destination port
                CLOSED);
    TCP_STATE state = client_handle_event(conn, OPEN, NULL);
}

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
        write_to_dev(netdev_fd, tcp_buf, buf_len);
        sleep(2);
    }
}