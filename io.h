#ifndef IO_H
#define IO_H

#include "utils.h"
int create_raw_socket();
void send_packet_raw(int sockfd, uint8_t* buf, size_t buf_len,
                    const char* dest_ip);
void write_to_dev(int netdev_fd, uint8_t* buf, size_t buf_len);
void read_from_dev(int netdev_fd, uint8_t* buf, size_t buf_len);
#endif