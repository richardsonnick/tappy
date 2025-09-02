#include <stdbool.h>

void client_loop(ip_addr_t* source_ip, ip_addr_t* destination_ip, 
    int src_port,
    int dst_port, bool pipe_mode);
