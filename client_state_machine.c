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

// TODO: There are parts here where i should check for ack of something rather than just ack.
TCP_STATE client_handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_ip_t* tcp_ip) {
    // TODO Add actual error handling.
    tcp_packet_t* packet = NULL;
    if (tcp_ip != NULL) {
      packet = tcp_ip->tcp_packet;
    }

    if (packet != NULL) {
        printf("TCP Packet Flags: ");
        if (packet->flags & TCP_FLAG_FIN) printf("FIN ");
        if (packet->flags & TCP_FLAG_SYN) printf("SYN ");
        if (packet->flags & TCP_FLAG_RST) printf("RST ");
        if (packet->flags & TCP_FLAG_PSH) printf("PSH ");
        if (packet->flags & TCP_FLAG_ACK) printf("ACK ");
        if (packet->flags & TCP_FLAG_URG) printf("URG ");
        printf("(0x%02x)\n", packet->flags);
    }

    switch(conn->state) {
        case(CLOSED):
            if (event == OPEN) {
                printf("Got OPEN from CLOSED state!!\n");
                // Passive OPEN (listen for syn)
                simple_send_flag(conn, TCP_FLAG_SYN);
                return SYN_SENT;
            }
            break;
        case(LISTEN):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) { 
                printf("Got RECEIVE from LISTEN state!!\n");
                // TODO: send syn, ack
                return SYN_RECEIVED;
            }
            break;
        case(SYN_SENT):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) {
                return SYN_RECEIVED;
            } else if (event == RECEIVE && packet->flags == TCP_FLAG_SYN | TCP_FLAG_ACK) {
              printf("Got SYN,ACK from SYN_SENT state!\n");
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
