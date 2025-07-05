#ifndef IO_H
#define IO_H

#include "utils.h"
void write_to_dev(int netdev_fd, uint8_t* buf, size_t buf_len);
void read_from_dev(int netdev_fd, uint8_t* buf, size_t buf_len);
#endif