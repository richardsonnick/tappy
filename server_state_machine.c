#include <stdlib.h>
#include "state_machine.h"
#include "server_state_machine.h"

tcb_t* init_tcp_stack() {
    tcb_t* tcb = malloc(sizeof(tcb_t));
    if (!tcb) return NULL;

    tcb->recv_buf.data = calloc(TCP_BUF_SIZE, sizeof(uint8_t));
    tcb->recv_buf.length = TCP_BUF_SIZE;
    tcb->send_buf.data = calloc(TCP_BUF_SIZE, sizeof(uint8_t));
    tcb->send_buf.length = TCP_BUF_SIZE;

    return tcb;
}

void teardown(tcp_connection_t* conn) {
    free(conn->tcb);
    conn->tcb = NULL;
}

// TODO: There are parts here where i should check for ack of something rather than just ack.
TCP_STATE server_handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_packet_t* packet) {
    switch(conn->state) {
        case(CLOSED):
            if (event == OPEN) {
                // Active OPEN (send syn)
                conn->tcb = init_tcp_stack();
                // TODO: Send syn
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
            teardown(conn);
            break;
    };
    return CLOSED; // maybe remove
}