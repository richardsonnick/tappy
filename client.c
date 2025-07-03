#include <linux/if.h>
#include <stdio.h>

#include "utils.h"
#include "client.h"

void client_loop() {
    // The kernel will give u a free tun device name automatically.
    // hence the format string.
    char dev[IFNAMSIZ] = "tun%d";
    int fd;
    if ( (fd = tun_alloc(dev)) < 0) {
        perror("Failed to open tun device");
    }

    printf("Created tun fd:%d\n", fd);
    close(fd);
}