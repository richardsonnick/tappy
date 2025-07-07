#ifndef SERVER_H
#define SERVER_H

#include "types.h"
void server_loop(
    ip_addr_t* source_ip,
    ip_addr_t* destination_ip, 
    int src_port,
    int dst_port);

#endif