void client_loop(ip_addr_t* source_ip, ip_addr_t* destination_ip, 
    int src_port,
    int dst_port);
void send_syn(int netdev_fd, ip_addr_t* source_ip, ip_addr_t* destination_ip);
