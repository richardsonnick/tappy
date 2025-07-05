#include "StateMachine.h"
#include "types.h"

TCP_STATE handle_event(tcp_connection_t* conn, TCP_EVENT event, const tcp_packet_t* packet);