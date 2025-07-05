#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "utils.h"
#include "tcp.h"
#include "string.h"

#include "client.h"

#include "utils.h"
#include "io.h"
void write_to_dev(int netdev_fd, uint8_t* buf, size_t buf_len) {
    // TODO: gate this with debug
    print_raw_buf(buf, buf_len);
    ssize_t written = write(netdev_fd, buf, buf_len);
    if (written != buf_len) {
        perror("write");
    }
}