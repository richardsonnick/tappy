#include <stdlib.h>
#include "state_machine.h"
#include "client_state_machine.h"
#include "tcp.h"
#include "io.h"
#include <stdio.h>
#include <unistd.h>

void client_teardown(tcp_connection_t* conn) {
    free(conn->tcb);
    conn->tcb = NULL;
}

void do_write(const tcp_ip_t* tcp_ip, int netdev_fd) {
    const size_t total_tcp_segment_len = MIN_TCP_PACKET_SIZE;
    const size_t total_packet_len = MIN_IP4_HEADER_SIZE + total_tcp_segment_len;
    uint8_t* buf = (uint8_t*)malloc(total_packet_len);
    size_t buf_len = tcp_ip_to_buf(tcp_ip, buf);
    write_to_dev(netdev_fd, buf, total_packet_len);
}

void syn(const tcp_connection_t* conn) {
    tcp_ip_t* tcp_ip = make_packet(conn->tcb, TCP_FLAG_SYN);
    // TODO: track a single socket for entire process.
    int sockfd = create_raw_socket();
    const size_t total_packet_len = MIN_IP4_HEADER_SIZE + MIN_TCP_PACKET_SIZE;
    uint8_t* buf = malloc(total_packet_len);
    tcp_ip_to_buf(tcp_ip, buf);
    print_raw_buf(buf, total_packet_len);
    send_packet_raw(sockfd, buf, total_packet_len, "192.168.1.130");
    close(sockfd);
    free(buf);
    // do_write(tcp_ip, conn->netdev_fd);
}

// TODO: There are parts here where i should check for ack of something rather than just ack.
TCP_STATE client_handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_packet_t* packet) {
    switch(conn->state) {
        case(CLOSED):
            if (event == OPEN) {
                printf("Got OPEN from CLOSED state!!\n");
                // Passive OPEN (listen for syn)
                syn(conn);
                return SYN_SENT;
            }
            break;
        case(LISTEN):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) { 
                // TODO: send syn, ack
                return SYN_RECEIVED;
            }
            break;
        case(SYN_SENT):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) {
                return SYN_RECEIVED;
            } else if (event == RECEIVE && packet->flags == TCP_FLAG_SYN | TCP_FLAG_ACK) {
                // TODO send ACK
                return ESTABLISHED;
            }
            break;
        case(SYN_RECEIVED):
            if (event == RECEIVE && packet->flags == TCP_FLAG_RST) {
                return LISTEN;
            } else if (event == RECEIVE && packet->flags == TCP_FLAG_ACK) {
                return ESTABLISHED;
            }
            else if (event == CLOSE) {
                // send FIN
                return FIN_WAIT_1;
            } 
            break;
        case(ESTABLISHED):
            if (event == RECEIVE && packet->flags == TCP_FLAG_FIN) {
                return CLOSE_WAIT;
            } else if (event == CLOSE) {
                // SEND FIN
                return FIN_WAIT_1;
            }
            break;
        case(FIN_WAIT_1):
            if (event == RECEIVE && packet->flags == TCP_FLAG_ACK) {
                return FIN_WAIT_2;
            } else if (event == RECEIVE && packet->flags == TCP_FLAG_FIN) {
                return CLOSING;
            }
            break;
        case(FIN_WAIT_2):
            if (event == RECEIVE && packet->flags == TCP_FLAG_FIN) {
                // SEND ack
                return TIME_WAIT;
            }
            break;
        case(CLOSE_WAIT):
            if (event == CLOSE && packet->flags == TCP_FLAG_FIN) {
                return LAST_ACK;
            }
            break;
        case(CLOSING):
            if (event == RECEIVE && packet->flags == TCP_FLAG_FIN) {
                return TIME_WAIT;
            }
            break;
        case(LAST_ACK):
            if (event == RECEIVE && packet->flags == TCP_FLAG_ACK) {
                return CLOSED;
            }
            break;
        case(TIME_WAIT):
            // Check timeout
            client_teardown(conn);
            break;
    };
    return CLOSED; // maybe remove
}
