#include <linux/if.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Invalid arg len\n");
        return -1;
    }
    // The kernel will give u a free tun device name automatically.
    // hence the format string.
    char dev[IFNAMSIZ] = "tun%d";
    int fd;
    if ( (fd = tun_alloc(dev)) < 0) {
        printf("Failed to open tun device\n");
        return -1;
    }

    printf("Created tun fd:%d\n", fd);
    if (strcmp(argv[1], "client") == 0) {
        client_loop(fd);
    }
    else if (strcmp(argv[1], "server") == 0) {
        server_loop(fd);
    }

    close(fd);
}