#ifndef SERVER_STATE_MACHINE_H
#define SERVER_STATE_MACHINE_H

#include "state_machine.h"
#include "types.h"

TCP_STATE server_handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_ip_t* tcp_ip);

#endif