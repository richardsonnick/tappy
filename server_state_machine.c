#include <stdlib.h>
#include <stdio.h>
#include "state_machine.h"
#include "server_state_machine.h"
#include "tcp.h"
#include "io.h"
#include "types.h"
#include "utils.h"

void server_teardown(tcp_connection_t* conn) {
    free(conn->tcb);
    conn->tcb = NULL;
}


// TODO: There are parts here where i should check for ack of something rather than just ack.
TCP_STATE server_handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_ip_t* tcp_ip) {
    print_conn(conn, "server_handle_event");
    
    // TODO Add actual error handling.
    tcp_packet_t* packet = NULL;
    if (tcp_ip != NULL) {
      packet = tcp_ip->tcp_packet;
    }

    if (packet != NULL) {
        printf("RECEIVED TCP Packet: \n");
        print_tcp_packet(tcp_ip->tcp_packet);
    }

    switch(conn->state) {
        case(CLOSED):
            if (event == OPEN) {
                // Passive OPEN (listen for syn)
                printf("Transitioned to OPEN state. Initialized TCB. LISTENING...\n");
                return LISTEN;
            }
            break;
        case(LISTEN):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) { 
                printf("SYN_RECEIVED\n");
                simple_send_flag(conn, TCP_FLAG_SYN | TCP_FLAG_ACK);
                return SYN_RECEIVED;
            }
            break;
        case(SYN_SENT):
            if (event == RECEIVE && packet->flags == TCP_FLAG_SYN) {
                return SYN_RECEIVED;
            } else if (event == RECEIVE && ((packet->flags == TCP_FLAG_SYN) | TCP_FLAG_ACK)) {
                simple_send_flag(conn, TCP_FLAG_ACK);
                printf("Sent ACK to complete handshake\n");
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
                simple_send_flag(conn, TCP_FLAG_FIN);
                return FIN_WAIT_1;
            } 
            break;
        case(ESTABLISHED):
            if (event == RECEIVE && packet->flags == TCP_FLAG_FIN) {
                return CLOSE_WAIT;
            } else if (event == CLOSE) {
                // SEND FIN
                simple_send_flag(conn, TCP_FLAG_FIN);
                printf("Sent FIN\n");
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
                simple_send_flag(conn, TCP_FLAG_ACK);
                printf("Sent ACK");
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
            server_teardown(conn);
            break;
    };
    return CLOSED; // maybe remove
}
